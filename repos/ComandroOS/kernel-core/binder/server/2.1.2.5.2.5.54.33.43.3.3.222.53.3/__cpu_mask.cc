#include "__cpu_mask.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/util.h> // Para funcoes de bitwise

namespace comandro {
namespace kernel {
namespace binder {
namespace server {

using kernel::Log;
using kernel::Scheduler;

static constexpr const char* TAG = "BinderCpuMask";

CpuMaskManager& CpuMaskManager::instance() {
    static CpuMaskManager s_instance;
    return s_instance;
}

CpuMaskArray CpuMaskManager::createEmptyMask() {
    return 0ULL;
}

CpuMaskArray CpuMaskManager::setCpu(CpuMaskArray mask, uint8_t cpu_id) {
    if (cpu_id >= MAX_CPUS) {
        Log::warn(TAG, "Tentativa de setar CPU invalida: " + std::to_string(cpu_id));
        return mask;
    }
    // Operacao de OR (ativa o bit)
    return mask | (1ULL << cpu_id);
}

CpuMaskArray CpuMaskManager::clearCpu(CpuMaskArray mask, uint8_t cpu_id) {
    if (cpu_id >= MAX_CPUS) {
        return mask;
    }
    // Operacao de AND com o complemento (desativa o bit)
    return mask & (~(1ULL << cpu_id));
}

bool CpuMaskManager::isCpuSet(CpuMaskArray mask, uint8_t cpu_id) {
    if (cpu_id >= MAX_CPUS) {
        return false;
    }
    // Verifica se o bit correspondente esta ativo
    return (mask & (1ULL << cpu_id)) != 0;
}

int CpuMaskManager::getFirstCpu(CpuMaskArray mask) {
    if (mask == 0) {
        return -1;
    }
    // Utiliza uma funcao intrinseca de kernel (ctz/ffs - Count Trailing Zeros/Find First Set)
    // Supondo que kernel::util::countTrailingZeros seja a funcao padrao.
    return kernel::util::countTrailingZeros(mask);
}

/**
 * @brief Aplica a afinidade de CPU a uma thread, integrando-se ao scheduler.
 */
bool CpuMaskManager::setThreadAffinity(kernel::Thread::TID tid, CpuMaskArray mask) {
    if (mask == 0) {
        Log::error(TAG, "Tentativa de setar afinidade com mascara vazia (nao permitido).");
        return false;
    }
    
    Log::info(TAG, "Aplicando cpumask " + std::to_string(mask) + " a thread Binder TID: " + std::to_string(tid));
    
    // Chamada critica para o agendador do kernel
    // O Scheduler e responsavel por mover a thread para um dos cores setados.
    if (!Scheduler::setAffinity(tid, mask)) {
        Log::critical(TAG, "Falha ao definir a afinidade da thread Binder no Scheduler.");
        return false;
    }
    
    return true;
}

} // namespace server
} // namespace binder
} // namespace kernel
} // namespace comandro
