#include "power_governor.rs.h" // Interface Rust FFI
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/gpu_scheduler.h>
#include <comandro/kernel/cpu_monitor.h>
#include <comandro/kernel/ipc/binder.h>

// =====================================================================
// performance_api.cc - APIs de Gerenciamento de Performance (CPU e GPU)
// Interface para o Governor Rust e agendamento de tarefas criticas.
// =====================================================================

namespace comandro {
namespace kernel {
namespace perf {

// Variavel global para o loop do Governor
static std::thread s_governor_thread;
static bool s_governor_running = false;

/**
 * @brief Thread dedicada a rodar o loop do Power Governor periodicamente.
 */
void governor_loop_thread() {
    scheduler::set_thread_priority(SCHED_PRIORITY_GOVERNOR); // Prioridade alta

    s_governor_running = true;
    while (s_governor_running) {
        // 1. Coleta a carga media atual dos nucleos 'Big'
        uint8_t load_avg = cpu_monitor::get_big_core_load_avg();

        // 2. Chama o algoritmo de decisao em Rust
        power_governor::run_governor_cycle(load_avg);
        
        // Dorme por 5ms (Ciclo ultra-rapido para baixa latencia)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

/**
 * @brief Inicializa o subsistema de controle de performance.
 * @return 0 em sucesso.
 */
int initialize_performance_api() {
    power_governor::init();
    
    // Inicia a thread do Governor
    s_governor_thread = std::thread(governor_loop_thread);
    scheduler::set_thread_name(s_governor_thread.native_handle(), "ComandroOS_Gov_Loop");
    
    LOG_INFO("Performance API: Governor thread iniciado. Freq CPU/GPU iniciais setadas.");
    return 0;
}

// -------------------------------------------------------------------
// Funcoes de Interface (Binder/PDK)
// -------------------------------------------------------------------

/**
 * @brief Servico Binder: Requisita uma explosao de clock temporaria da GPU.
 * Usado por tarefas de UI criticas (e.g., animacoes de toque).
 * @param duration_ms Duracao do burst de clock.
 */
void binder_request_gpu_burst(uint32_t duration_ms) {
    // 1. Informa o Scheduler de GPU para escalar imediatamente.
    gpu_scheduler::boost_gpu_clock(duration_ms);

    // 2. Notifica o Governor para manter a CPU alta durante este periodo.
    cpu_monitor::set_boost_hint(duration_ms); 

    LOG_DEBUG("GPU/CPU Boost de performance solicitado por %u ms.", duration_ms);
}

/**
 * @brief Retorna as frequencias atuais para diagnostico (Nucleum/Dexter).
 */
void get_current_frequencies(uint32_t* cpu_freq, uint32_t* gpu_freq) {
    *cpu_freq = power_governor::get_current_cpu_freq();
    *gpu_freq = power_governor::get_current_gpu_freq();
}

} // namespace perf
} // namespace kernel
} // namespace comandro
