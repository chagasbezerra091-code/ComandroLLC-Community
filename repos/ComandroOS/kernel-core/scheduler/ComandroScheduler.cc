#include "ComandroScheduler.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/lock.h> // Simula um spinlock
#include <comandro/kernel/system_time.h> // Para SystemTime::get_current_ns()

namespace comandro {
namespace kernel {
namespace scheduler {

// O Spinlock protege as filas de execucao (runqueues)
static SpinLock runqueue_lock; 
static constexpr const char* TAG = "ComandroScheduler";

ComandroScheduler::ComandroScheduler() {
    // Inicializa as listas do RT runqueue (uma lista para cada prioridade)
    for (int i = 0; i < 100; ++i) {
        INIT_LIST_HEAD(&rt_runqueue[i]);
    }
    // Inicializa a lista do CRAN runqueue
    INIT_LIST_HEAD(&cran_runqueue); 
    Log::info(TAG, "ComandroScheduler inicializado. Modo hibrido RT/CRAN ativo.");
}

// =====================================================================
// Funcoes de Agendamento (Scheduler)
// =====================================================================

/**
 * @brief Funcao principal chamada pelo timer interrupt.
 */
void ComandroScheduler::schedule() {
    // 1. Desabilita interrupcoes e adquire o lock
    runqueue_lock.lock(); 
    
    // 2. Conta o tempo de execucao da thread atual
    if (current_thread) {
        uint64_t current_time = SystemTime::get_current_ns();
        uint64_t actual_runtime = current_time - current_thread->exec_start_time_ns;
        
        // Atualiza vruntime se for uma thread CRAN
        if (current_thread->priority <= PRIORITY_UI_INTERACTIVE) {
            update_vruntime(current_thread, actual_runtime);
        }
        current_thread->total_runtime_ns += actual_runtime;
        
        // Coloca a thread atual de volta na fila se nao estiver bloqueada
        if (current_thread->list_node.prev == nullptr) { // Simplificacao: verifica se a thread foi removida (ex: por sleep)
             enqueue_thread(current_thread);
        }
    }
    
    // 3. Escolhe a proxima thread
    ThreadDescriptor* next_td = pick_next_thread();
    
    if (next_td != nullptr && next_td != current_thread) {
        dequeue_thread(next_td); // Remove da fila antes da troca de contexto
        
        // Marca o tempo de inicio de execucao
        next_td->exec_start_time_ns = SystemTime::get_current_ns();
        
        // 4. Troca de Contexto (Simulacao)
        Log::debug(TAG, "Troca de contexto: TID " + std::to_string(current_thread ? current_thread->tid : 0) + 
                         " -> TID " + std::to_string(next_td->tid) + 
                         " Prio: " + std::to_string(next_td->priority));
        
        // context_switch(current_thread, next_td); // Chamada ASM/hardware
        current_thread = next_td;
    }

    // 5. Libera o lock e reabilita interrupcoes
    runqueue_lock.unlock(); 
}

/**
 * @brief Escolhe a thread com maior prioridade para rodar.
 */
ThreadDescriptor* ComandroScheduler::pick_next_thread() {
    // Prioridade 1: RT (Real-Time)
    ThreadDescriptor* rt_thread = pick_next_rt();
    if (rt_thread) {
        return rt_thread;
    }
    
    // Prioridade 2: CRAN (Cranberry / Fair)
    return pick_next_cran();
}

/**
 * @brief Selecao para Fila de Tempo Real (RT) - Lista simples por prioridade.
 */
ThreadDescriptor* ComandroScheduler::pick_next_rt() {
    // Percorre as filas RT de maior prioridade (99) para menor (70)
    for (int i = PRIORITY_RT_EMERGENCY; i >= PRIORITY_UI_INTERACTIVE; --i) {
        if (!list_empty(&rt_runqueue[i])) {
            // Pega o primeiro da lista (FIFO dentro do mesmo nivel de RT)
            return list_entry(rt_runqueue[i].next, ThreadDescriptor, list_node);
        }
    }
    return nullptr;
}

/**
 * @brief Selecao para Fila Cranberry (CRAN) - Baseada em vruntime.
 * Esta lista deve ser idealmente uma estrutura de arvore (e.g., RB-Tree)
 * para encontrar o menor vruntime rapidamente. Aqui, simplificamos com uma lista.
 */
ThreadDescriptor* ComandroScheduler::pick_next_cran() {
    if (list_empty(&cran_runqueue)) {
        return nullptr;
    }
    
    ThreadDescriptor* best_td = nullptr;
    uint64_t min_vruntime = (uint64_t)-1; // Valor maximo

    // Itera sobre a lista CRAN para encontrar o menor vruntime (O(n) - lento)
    list_head *pos;
    list_for_each(pos, &cran_runqueue) {
        ThreadDescriptor *td = list_entry(pos, ThreadDescriptor, list_node);
        
        if (td->vruntime_ns < min_vruntime) {
            min_vruntime = td->vruntime_ns;
            best_td = td;
        }
    }
    
    return best_td; // A thread com o menor vruntime e a mais "faminta"
}

// =====================================================================
// Funcoes de Gerenciamento
// =====================================================================

void ComandroScheduler::enqueue_thread(ThreadDescriptor* td) {
    if (td->priority >= PRIORITY_RT_DISPLAY_VSYNC) {
        // Enfileira na lista RT correspondente a prioridade
        list_add_tail(&td->list_node, &rt_runqueue[td->priority]);
    } else {
        // Enfileira na lista CRAN (sera ordenada por vruntime na selecao)
        list_add_tail(&td->list_node, &cran_runqueue);
    }
}

void ComandroScheduler::dequeue_thread(ThreadDescriptor* td) {
    list_del_init(&td->list_node);
}

void ComandroScheduler::update_vruntime(ThreadDescriptor* td, uint64_t actual_runtime_ns) {
    // A logica real envolveria pesos baseados na prioridade (PRIORITY_UI_INTERACTIVE a PRIORITY_VERY_LOW)
    // Threads de prioridade mais alta CRAN (ex: UI) terao um fator de peso menor (vruntime cresce mais devagar).
    
    uint64_t weighted_runtime = actual_runtime_ns;
    // Simplificacao: Threads de baixa prioridade (muito_low) tem vruntime 4x mais rapido
    if (td->priority == PRIORITY_VERY_LOW) {
        weighted_runtime *= 4; 
    }
    
    td->vruntime_ns += weighted_runtime;
}

void ComandroScheduler::add_thread(ThreadDescriptor* td) {
    runqueue_lock.lock();
    td->vruntime_ns = 0; // Inicia com vruntime zero
    td->exec_start_time_ns = 0;
    td->total_runtime_ns = 0;
    enqueue_thread(td);
    Log::debug(TAG, "Thread TID " + std::to_string(td->tid) + " adicionada.");
    runqueue_lock.unlock();
}

void ComandroScheduler::set_thread_priority(ThreadDescriptor* td, Priority new_priority) {
    runqueue_lock.lock();
    
    bool needs_reschedule = (new_priority > td->priority);
    
    dequeue_thread(td);
    td->priority = new_priority;
    enqueue_thread(td);
    
    // Se a prioridade foi elevada, forca um reschedule imediato
    if (needs_reschedule) {
        Log::info(TAG, "Prioridade de TID " + std::to_string(td->tid) + " elevada. Reschedule forcado.");
        // comandro_trigger_reschedule_interrupt(); // Simula uma interrupcao de reschedule
    }
    
    runqueue_lock.unlock();
}

void ComandroScheduler::yield() {
    runqueue_lock.lock();
    
    // Atualiza o vruntime para threads CRAN antes de ceder
    if (current_thread && current_thread->priority <= PRIORITY_UI_INTERACTIVE) {
        uint64_t current_time = SystemTime::get_current_ns();
        uint64_t actual_runtime = current_time - current_thread->exec_start_time_ns;
        update_vruntime(current_thread, actual_runtime);
    }
    
    // Forca o scheduler a rodar
    // comandro_trigger_reschedule_interrupt(); 
    
    runqueue_lock.unlock();
}

// Simples simulacao de sleep (para ser usado pelo user space/framework)
void ComandroScheduler::sleep(std::chrono::milliseconds duration) {
    // Na implementacao real:
    // 1. current_thread->state = THREAD_BLOCKED;
    // 2. Add current_thread a uma lista de timers;
    // 3. dequeue_thread(current_thread);
    // 4. schedule();
    Log::debug(TAG, "Thread chamou sleep por " + std::to_string(duration.count()) + "ms.");
    // A thread bloqueia, e o scheduler a acordara quando o timer estourar.
}

} // namespace scheduler
} // namespace kernel
} // namespace comandro
