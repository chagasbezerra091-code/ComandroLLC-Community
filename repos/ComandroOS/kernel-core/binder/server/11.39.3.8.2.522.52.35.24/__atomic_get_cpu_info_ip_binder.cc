#include <comandro/kernel/binder/server/atomic_info.h>
#include <comandro/kernel/cpu_monitor.h>
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/time.h>
#include <atomic>

// =====================================================================
// __atomic_get_cpu_info_ip_binder.cc - Acesso Atômico a Infos da CPU e IP da Thread
// Proposito: Fornecer dados de baixa latencia para o seletor de nucleo de Binder.
// =====================================================================

namespace comandro {
namespace kernel {
namespace binder {

// Estrutura interna de cache de informacoes da CPU por nucleo.
struct CpuAtomicCache {
    // Frequencia da CPU (em MHz) no momento da ultima atualizacao.
    std::atomic<uint32_t> current_frequency_mhz;
    
    // Nivel de carga (0-100%) no momento da ultima atualizacao.
    std::atomic<uint8_t> current_load_percent;
};

// Array de caches para cada nucleo do sistema.
static CpuAtomicCache s_cpu_caches[cpu::MAX_CPU_CORES];


/**
 * @brief Inicializa a cache de informacoes da CPU.
 */
void initialize_atomic_cpu_cache() {
    for (int i = 0; i < cpu::MAX_CPU_CORES; ++i) {
        s_cpu_caches[i].current_frequency_mhz.store(0);
        s_cpu_caches[i].current_load_percent.store(0);
    }
    LOG_INFO("Binder Atomic CPU Cache inicializada para %d nucleos.", cpu::MAX_CPU_CORES);
}

/**
 * @brief Atualiza de forma atomica a cache de informacoes da CPU.
 * Esta funcao e chamada periodicamente pelo Daemon do Scheduler.
 * @param core_id ID do nucleo a ser atualizado.
 */
void atomic_update_cpu_cache(int core_id) {
    if (core_id < 0 || core_id >= cpu::MAX_CPU_CORES) {
        return;
    }

    // 1. Obtem os dados reais (simulando chamada ao driver de hardware)
    uint32_t freq = cpu_monitor::get_core_frequency(core_id);
    uint8_t load = scheduler::get_core_load_percentage(core_id);

    // 2. Armazena na cache de forma atomica e lock-free
    s_cpu_caches[core_id].current_frequency_mhz.store(freq, std::memory_order_release);
    s_cpu_caches[core_id].current_load_percent.store(load, std::memory_order_release);
}


// -------------------------------------------------------------------
// Funcoes de Leitura Atomica para o Selecionador de Nucleo (select_target_cpu)
// -------------------------------------------------------------------

/**
 * @brief Leitura atomica da frequencia atual de um nucleo.
 * Usado pelo Binder para priorizar nucleos mais rapidos.
 * @param core_id ID do nucleo.
 * @return Frequencia em MHz.
 */
uint32_t atomic_read_core_frequency(int core_id) {
    if (core_id < 0 || core_id >= cpu::MAX_CPU_CORES) {
        return 0;
    }
    // Leitura atomica com memory_order_acquire garante que le o ultimo valor 'release'
    return s_cpu_caches[core_id].current_frequency_mhz.load(std::memory_order_acquire);
}

/**
 * @brief Leitura atomica do percentual de carga de um nucleo.
 * Usado pelo Binder para desviar transacoes para nucleos menos ocupados.
 * @param core_id ID do nucleo.
 * @return Carga em percentual (0-100).
 */
uint8_t atomic_read_core_load(int core_id) {
    if (core_id < 0 || core_id >= cpu::MAX_CPU_CORES) {
        return 100; // Assume alta carga para nucleos invalidos
    }
    return s_cpu_caches[core_id].current_load_percent.load(std::memory_order_acquire);
}


/**
 * @brief Obtem de forma atomica o Instruction Pointer (IP) de uma thread Binder.
 * Utilitario de debugging de baixa latencia.
 * @param binder_thread_id ID da thread alvo.
 * @return Endereco do IP (program counter) da thread.
 */
uintptr_t atomic_get_thread_ip(int binder_thread_id) {
    // 1. Localiza a estrutura de controle da thread (Thread Control Block - TCB)
    ThreadControlBlock* tcb = scheduler::get_tcb_by_id(binder_thread_id);
    
    if (tcb == nullptr) {
        return 0;
    }

    // 2. Leitura atômica do IP (Simulacao de acesso ao registrador salvo na TCB)
    // O IP e um dado critico, seu acesso deve ser rapido e livre de locks.
    // Em um kernel real, isso seria um acesso direto a um campo volatile/atomic na TCB.
    uintptr_t ip_address = tcb->atomic_saved_instruction_pointer.load(std::memory_order_acquire);
    
    // [HEURÍSTICA DE SEGURANÇA BINDER]
    // Se o IP estiver em uma area critica do kernel (ex: rotina de lock), 
    // o Binder pode decidir interromper a transacao ou tentar outro nucleo.
    if (ip_address >= KERNEL_LOCK_REGION_START && ip_address <= KERNEL_LOCK_REGION_END) {
        LOG_WARN("IP de Thread Binder %d em regiao critica.", binder_thread_id);
    }

    return ip_address;
}

} // namespace binder
} // namespace kernel
} // namespace comandro
