#ifndef COMANDRO_KERNEL_BINDER_CPU_MASK_H
#define COMANDRO_KERNEL_BINDER_CPU_MASK_H

#include <comandro/kernel/types.h>
#include <comandro/kernel/thread.h>
#include <comandro/kernel/cpuset.h> // Dependencia do kernel

namespace comandro {
namespace kernel {
namespace binder {
namespace server {

// O tamanho da máscara de CPU e fixo, definido pelo hardware maximo do ComandroOS (32/64/128)
static constexpr size_t MAX_CPUS = 64; 

/**
 * @brief Tipo que representa um conjunto de CPUs ativas, baseado na funcionalidade nativa do kernel.
 * * Usamos um array de 64 bits para suportar ate 64 cores.
 */
using CpuMaskArray = uint64_t;

/**
 * @brief Classe utilitaria para manipular conjuntos de CPUs (cpumasks) no contexto do Binder Server.
 * * O Binder pode precisar fixar threads em CPUs especificas para latencia (affine binding).
 */
class CpuMaskManager {
public:
    static CpuMaskManager& instance();

    /**
     * @brief Cria uma mascara vazia (nenhuma CPU setada).
     */
    static CpuMaskArray createEmptyMask();

    /**
     * @brief Adiciona uma CPU especifica ao conjunto.
     * @param mask O conjunto de CPUs.
     * @param cpu_id O ID da CPU (0 a MAX_CPUS-1).
     * @return A mascara atualizada.
     */
    static CpuMaskArray setCpu(CpuMaskArray mask, uint8_t cpu_id);

    /**
     * @brief Remove uma CPU especifica do conjunto.
     * @param mask O conjunto de CPUs.
     * @param cpu_id O ID da CPU.
     * @return A mascara atualizada.
     */
    static CpuMaskArray clearCpu(CpuMaskArray mask, uint8_t cpu_id);

    /**
     * @brief Verifica se uma CPU esta contida na mascara.
     */
    static bool isCpuSet(CpuMaskArray mask, uint8_t cpu_id);

    /**
     * @brief Retorna o numero da primeira CPU setada na mascara (para iteracao).
     * @param mask O conjunto de CPUs.
     * @return O ID da CPU (ou -1 se vazio).
     */
    static int getFirstCpu(CpuMaskArray mask);

    /**
     * @brief Aplica a mascara de CPU a uma thread especifica.
     * * Esta e a funcao critica para o agendamento (scheduler).
     * @param tid O ID da thread.
     * @param mask A mascara de CPU a ser aplicada.
     * @return true se a afinidade foi aplicada com sucesso.
     */
    bool setThreadAffinity(kernel::Thread::TID tid, CpuMaskArray mask);

private:
    CpuMaskManager() = default;
};

} // namespace server
} // namespace binder
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_BINDER_CPU_MASK_H
