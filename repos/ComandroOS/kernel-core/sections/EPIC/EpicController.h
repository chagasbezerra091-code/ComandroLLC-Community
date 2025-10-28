#ifndef COMANDRO_KERNEL_SECTIONS_EPIC_CONTROLLER_H
#define COMANDRO_KERNEL_SECTIONS_EPIC_CONTROLLER_H

#include "EpicTypes.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/spinlock.h>
#include <functional>

namespace comandro {
namespace kernel {
namespace epic {

// Funcao de callback para o handler de interrupcao
using IrqHandler = std::function<void()>;

/**
 * @brief O EpicController e o gerenciador central de interrupcoes no ComandroOS.
 * * Garante previsibilidade e latencia minima para interrupcoes em Tempo Real (RT).
 */
class EpicController {
public:
    static EpicController& instance();

    /**
     * @brief Inicializa o hardware do controlador de interrupcao e suas tabelas.
     * @return true se a inicializacao foi bem-sucedida.
     */
    bool initializeHardware();

    /**
     * @brief Registra um handler (ISR) para uma interrupcao especifica.
     * @param irq_id O ID do IRQ (e.g., 32 para Timer, 40 para NIC).
     * @param handler A funcao de callback a ser executada.
     * @param priority A prioridade de kernel a ser atribuida.
     */
    void registerIrqHandler(IrqId irq_id, IrqHandler handler, IrqPriority priority);

    /**
     * @brief Configura o hardware do IRQ (prioridade, modo de disparo e CPU target).
     */
    void configureIrq(const IrqConfigRegister& config);

    /**
     * @brief Habilita a interrupcao no controlador (hardware).
     * @param irq_id O ID do IRQ a ser habilitado.
     */
    void enableIrq(IrqId irq_id);

    /**
     * @brief Desabilita a interrupcao no controlador (hardware).
     * @param irq_id O ID do IRQ a ser desabilitado.
     */
    void disableIrq(IrqId irq_id);

    /**
     * @brief Funcao principal chamada pelo assembly de baixo nivel na ocorrencia de uma IRQ.
     * * Esta funcao orquestra a chamada do handler registrado.
     * @param irq_id O ID da interrupcao que disparou.
     */
    void handleIrqDispatch(IrqId irq_id);

private:
    EpicController();
    
    SpinLock m_lock;
    std::array<IrqHandler, 256> m_irq_handlers; // Tabela de handlers
    std::array<IrqPriority, 256> m_irq_priorities; // Tabela de prioridades

    /**
     * @brief Notifica o hardware (GIC/EPIC) que a IRQ foi processada.
     */
    void acknowledgeIrq(IrqId irq_id);
};

} // namespace epic
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_SECTIONS_EPIC_CONTROLLER_H
