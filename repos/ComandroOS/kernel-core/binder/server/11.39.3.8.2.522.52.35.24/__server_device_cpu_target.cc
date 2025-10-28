#include <comandro/kernel/binder/server/cpu_target.h>
#include <comandro/kernel/cpu_topology.h>
#include <comandro/kernel/scheduler.h>
#include <atomic>

// =====================================================================
// __server_device_cpu_target.cc - Otimizador de Afinidad de CPU para Binder
// Implementa as heurísticas para escolher o nucleo de CPU para threads Binder Server.
// =====================================================================

namespace comandro {
namespace kernel {
namespace binder {

// A ID do nucleo de CPU mais adequado para processar transacoes criticas.
// Deve ser uma variavel atomica para evitar condicoes de corrida.
static std::atomic<int> s_preferred_cpu_core(-1);

/**
 * @brief Inicializa o subsistema de segmentacao de CPU do Binder.
 * Escolhe um nucleo 'prime' (ou o mais eficiente) para transacoes criticas.
 */
int initialize_cpu_target_strategy() {
    // 1. Consulta a topologia do kernel para identificar nucleos Big/LITTLE.
    const auto& topo = cpu::get_topology_info();
    int best_core = -1;

    // 2. Heuristica de Selecao: Prioriza o maior nucleo disponivel (Big/Prime Core)
    //    para lidar com a carga de trabalho do Binder, se o sistema estiver sob baixa carga.
    if (topo.has_big_cores) {
        best_core = topo.highest_performance_core_id;
    } else {
        // Se nao houver Big Cores, escolhe o Core 0 por padrao.
        best_core = 0; 
    }

    if (best_core >= 0 && best_core < topo.total_core_count) {
        s_preferred_cpu_core.store(best_core);
        scheduler::set_cpu_target_affinity(BINDER_SERVER_TASK_ID, best_core);
        
        LOG_INFO("Binder CPU Target inicializado: Nucleo Preferido = %d", best_core);
        return 0;
    }

    LOG_ERROR("Falha ao inicializar o Binder CPU Target. Topologia invalida.");
    s_preferred_cpu_core.store(-1);
    return -1;
}

/**
 * @brief Retorna o ID do nucleo de CPU atualmente preferido para threads do Binder Server.
 * @return ID do nucleo (0 a N-1) ou -1 se nao inicializado.
 */
int get_preferred_cpu_core() {
    return s_preferred_cpu_core.load();
}

/**
 * @brief Heurística dinamica para escolher o nucleo alvo para uma nova transacao Binder.
 * @param transaction_priority Prioridade da transacao (de 0 a 100).
 * @return O ID do nucleo de CPU mais adequado para executar a thread do servidor.
 */
int select_target_cpu(int transaction_priority) {
    int preferred_core = s_preferred_cpu_core.load();

    // 1. Caso de alta prioridade (Latency-Critical)
    if (transaction_priority >= SCHED_PRIORITY_CRITICAL) {
        // Transacoes criticas SEMPRE vao para o nucleo mais rapido/reservado.
        if (preferred_core != -1) {
            return preferred_core;
        }
    }

    // 2. Caso de baixa prioridade (Background)
    if (transaction_priority < SCHED_PRIORITY_NORMAL) {
        // Transacoes em segundo plano sao desviadas para nucleos LITTLE (eficiencia energetica),
        // se disponiveis e se o nucleo preferido estiver ocupado.
        const auto& topo = cpu::get_topology_info();
        if (topo.has_little_cores) {
             // Heuristica: Pega um nucleo aleatorio de eficiencia (LITTLE)
             // (native_random_core_id(LITTLE_GROUP) seria a implementacao real)
             return topo.first_little_core_id; 
        }
    }
    
    // 3. Caso padrao ou fallback
    // Retorna o nucleo que esta atualmente menos ocupado (load balancing basico).
    return scheduler::get_least_busy_cpu();
}

/**
 * @brief Chamado quando uma transacao critica termina, reavaliando a carga.
 * @param core_id O nucleo em que a transacao foi processada.
 */
void notify_transaction_complete(int core_id) {
    // [LOGICA DE REBALANCEAMENTO]
    // Se o nucleo preferido estiver sob alta carga persistente (> 90%),
    // esta funcao pode acionar uma mudanca em s_preferred_cpu_core para o proximo
    // nucleo BIG menos ocupado.
    
    // (A complexa logica de rebalanceamento dinamico e omitida por brevidade)
    scheduler::report_work_finished(core_id);
}

} // namespace binder
} // namespace kernel
} // namespace comandro
