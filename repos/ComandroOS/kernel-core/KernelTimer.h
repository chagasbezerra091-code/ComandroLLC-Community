#ifndef COMANDRO_KERNEL_CORE_KERNEL_TIMER_H
#define COMANDRO_KERNEL_CORE_KERNEL_TIMER_H

#include <comandro/kernel/types.h>
#include <comandro/kernel/spinlock.h>
#include <chrono>

namespace comandro {
namespace kernel {

using Nanoseconds = std::chrono::nanoseconds;

using TimerCallback = void (*)(void* context);

/**
 * @brief Gerencia os temporizadores de hardware e software do kernel.
 */
class KernelTimer {
public:
    static KernelTimer& instance();
    
    // Funcao critica chamada pelo IRQ do timer de hardware.
    void handleHwTimerIrq();

    // Registra um temporizador de software one-shot ou periodico.
    uint32_t setTimer(Nanoseconds duration, TimerCallback callback, void* context, bool periodic);
    
    bool cancelTimer(uint32_t timer_id);

private:
    KernelTimer() = default;
    SpinLock m_lock;
    uint32_t m_next_timer_id;
};

} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_CORE_KERNEL_TIMER_H
