#include <comandro/kernel/binder/server/cpu_target.h> // Para select_target_cpu
#include <comandro/kernel/binder/server/atomic_info.h> // Para dados atomicos
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/thread_info.h>

// =====================================================================
// optimize_cpu_binder_ip.cc - Otimizador de Afinidad de Thread Binder
// Utiliza todas as metricas em tempo real para setar a afinidade de CPU 
// de uma thread Binder Server no momento da transacao.
// =====================================================================

namespace comandro {
namespace kernel {
namespace binder {

/**
 * @brief Otimiza dinamicamente a afinidade de CPU para a thread Binder atual.
 * * Esta e a funcao critica chamada apos o kernel acordar a thread do servidor 
 * Binder para processar uma nova transacao.
 * * @param binder_thread_id ID da thread Binder Server que vai processar a transacao.
 * @param transaction_priority Prioridade da transacao (de SCHED_PRIORITY_LOW a SCHED_PRIORITY_CRITICAL).
 * @return O ID do nucleo de CPU que foi definido como alvo.
 */
int optimize_thread_affinity(int binder_thread_id, int transaction_priority) {
    // 1. Determinar o nucleo alvo usando a heuristica (do __server_device_cpu_target.cc)
    int target_cpu_id = select_target_cpu(transaction_priority);

    if (target_cpu_id < 0) {
        LOG_WARN("optimize_thread_affinity: Falha ao selecionar CPU alvo. Usando nucleo atual.");
        return scheduler::get_current_cpu_id();
    }
    
    // 2. Coleta de Metricas (Pre-Check para Otimizacao Fina)
    // A afinidade e determinada, mas verificamos as metricas atomicamente
    // antes de setar o scheduling.
    
    uint32_t current_freq = atomic_read_core_frequency(target_cpu_id);
    uint8_t current_load = atomic_read_core_load(target_cpu_id);

    // 3. Logica de Contingencia (Baseada em dados atomicos)
    // Se a carga for altissima (>95%) e o nucleo alvo for um nucleo LITTLE,
    // o sistema pode decidir promover a thread para um nucleo BIG disponivel.
    if (current_load > 95 && cpu::get_topology_info().is_little_core(target_cpu_id)) {
        int big_core_fallback = get_preferred_cpu_core(); 
        
        // Verifica se o nucleo BIG nao esta sobrecarregado.
        if (big_core_fallback != -1 && atomic_read_core_load(big_core_fallback) < 80) {
            target_cpu_id = big_core_fallback;
            LOG_DEBUG("Binder Thread %d promovida de LITTLE para BIG Core %d devido a sobrecarga.", 
                      binder_thread_id, big_core_fallback);
        }
    }

    // 4. Execucao da Mudanca de Afinidade
    // O kernel e instruido a agendar esta thread APENAS no nucleo alvo.
    int result = scheduler::set_thread_affinity(binder_thread_id, target_cpu_id);

    if (result == 0) {
        LOG_TRACE("Binder Thread %d afinidade setada para CPU %d (Freq: %uMHz, Load: %u%%)", 
                  binder_thread_id, target_cpu_id, current_freq, current_load);
    } else {
        LOG_ERROR("optimize_thread_affinity: Erro ao setar afinidade para CPU %d. Erro: %d", 
                  target_cpu_id, result);
    }

    return target_cpu_id;
}

/**
 * @brief Hook de otimizacao de baixa latencia.
 * E chamado em intervalos de tempo curtos para reajustar a afinidade de threads criticas.
 * (Omitted for brevity: Esta função reavalia threads de baixa prioridade que estao 
 * "roubando" tempo de CPU do nucleo preferido).
 */
void periodic_affinity_recheck() {
    // scheduler::iterate_critical_binder_threads([](int thread_id, int current_cpu) {
    //    // logica de reajuste se o load_average do nucleo preferido estiver acima do limite.
    // });
}

} // namespace binder
} // namespace kernel
} // namespace comandro
