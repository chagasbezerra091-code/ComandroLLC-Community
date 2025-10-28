#ifndef COMANDRO_KERNEL_SECTIONS_EPIC_TYPES_H
#define COMANDRO_KERNEL_SECTIONS_EPIC_TYPES_H

#include <comandro/kernel/types.h>

namespace comandro {
namespace kernel {
namespace epic {

// =====================================================================
// EPIC_TYPES_H - Tipos para o Controlador de Interrupcao Previsivel
// =====================================================================

// ID de Interrupcao (IRQ)
using IrqId = uint16_t;

// Prioridades de Interrupcao (de 0 a 255, 0 = Mais Alta)
using IrqPriority = uint8_t;
static constexpr IrqPriority IRQ_PRIORITY_CRITICAL = 0;
static constexpr IrqPriority IRQ_PRIORITY_LOW = 255;

// Estados do IRQ
enum class IrqState {
    DISABLED,
    PENDING,
    ACTIVE
};

// Modos de Disparo (Triggering)
enum class IrqTriggerMode {
    LEVEL_HIGH,     // Disparo por nivel alto (Level-Triggered)
    EDGE_RISING,    // Disparo por borda de subida (Edge-Triggered)
    MSI             // Message Signaled Interrupts
};

/**
 * @brief Estrutura que representa o registrador de configuracao de um IRQ.
 */
struct IrqConfigRegister {
    IrqId id;
    IrqPriority priority;
    IrqTriggerMode mode;
    uint32_t target_cpu_mask; // Mascara de CPU para roteamento de interrupcao
};

} // namespace epic
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_SECTIONS_EPIC_TYPES_H
