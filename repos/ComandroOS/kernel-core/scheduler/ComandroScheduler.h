#ifndef COMANDRO_KERNEL_SCHEDULER_H
#define COMANDRO_KERNEL_SCHEDULER_H

#include <comandro/kernel/thread.h>
#include <comandro/kernel/list.h> // Simula uma lista ligada do kernel
#include <stdint.h>
#include <chrono>

namespace comandro {
namespace kernel {
namespace scheduler {

// Definicoes de Prioridade
enum Priority {
    PRIORITY_RT_EMERGENCY = 99,     // Critical Watchdogs, Hardware Abort
    PRIORITY_RT_AUDIO_STREAM = 90,  // Audio I/O (must run)
    PRIORITY_RT_DISPLAY_VSYNC = 85, // Display/GPU V-Sync
    PRIORITY_UI_INTERACTIVE = 70,   // Touch/Input Processing
    PRIORITY_CRAN_NORMAL = 50,      // Tarefas padrao, IPC
    PRIORITY_CRAN_BACKGROUND = 20,  // Background Services, Networking
    PRIORITY_VERY_LOW = 1,          // Garbage Collector, Log Uploads
};

// Estrutura do Descritor de Thread
struct ThreadDescriptor {
    uint32_t tid;                   // ID da thread
    Priority priority;              // Prioridade atual
    uint64_t vruntime_ns;           // Virtual Runtime (para agendamento CRAN)
    uint64_t exec_start_time_ns;    // Tempo de inicio da ultima execucao
    uint64_t total_runtime_ns;      // Tempo total de execucao
    
    // Ponteiros para listas do scheduler (RT ou CRAN)
    list_head list_node; 
};

class ComandroScheduler {
private:
    // Fila para threads de Tempo Real (RT): Listas para cada nivel de prioridade RT.
    list_head rt_runqueue[100]; // 99 -> 70
    
    // Fila para threads Cranberry (CRAN): Estrutura de arvore binaria ou lista ordenada.
    list_head cran_runqueue; 
    
    ThreadDescriptor* current_thread = nullptr;

    // Funcoes internas
    void enqueue_thread(ThreadDescriptor* td);
    void dequeue_thread(ThreadDescriptor* td);
    ThreadDescriptor* pick_next_thread();
    
    // Logica de Tempo Real
    ThreadDescriptor* pick_next_rt();
    
    // Logica Cranberry (fair, mas focado em baixa latencia)
    ThreadDescriptor* pick_next_cran();
    void update_vruntime(ThreadDescriptor* td, uint64_t actual_runtime_ns);
    
public:
    ComandroScheduler();
    
    /**
     * @brief Chamado pelo timer interrupt para agendar a proxima thread.
     */
    void schedule();

    /**
     * @brief Adiciona uma nova thread ao scheduler.
     */
    void add_thread(ThreadDescriptor* td);

    /**
     * @brief Define a prioridade de uma thread, movendo-a entre filas se necessario.
     */
    void set_thread_priority(ThreadDescriptor* td, Priority new_priority);
    
    /**
     * @brief O kernel cede o restante do seu quantum.
     */
    void yield();

    // Funcoes publicas para uso externo (simulacao)
    static void sleep(std::chrono::milliseconds duration);
};

} // namespace scheduler
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_SCHEDULER_H
