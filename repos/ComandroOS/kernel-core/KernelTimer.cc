#include "KernelTimer.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/scheduler.h>
#include <list>

namespace comandro {
namespace kernel {

using kernel::Log;
using kernel::Scheduler;

static constexpr const char* TAG = "KernelTimer";
static constexpr Nanoseconds HW_TICK_RATE = Nanoseconds(1000000); // 1ms por tick

// Estrutura interna para um temporizador de software
struct SoftwareTimer {
    uint32_t id;
    Nanoseconds expiry_time;
    Nanoseconds duration;
    TimerCallback callback;
    void* context;
    bool periodic;

    bool operator<(const SoftwareTimer& other) const {
        return expiry_time < other.expiry_time;
    }
};

static std::list<SoftwareTimer> s_active_timers; // Lista ordenada por tempo de expiracao

KernelTimer& KernelTimer::instance() {
    static KernelTimer s_instance;
    return s_instance;
}

uint32_t KernelTimer::setTimer(Nanoseconds duration, TimerCallback callback, void* context, bool periodic) {
    SpinLock::Guard lock(m_lock);
    
    if (duration.count() == 0) {
        Log::error(TAG, "Duracao do temporizador invalida.");
        return 0;
    }
    
    // Calcula o tempo de expiracao a partir do tempo atual de alta resolucao
    Nanoseconds current_time = Scheduler::getKernelTime();
    uint32_t new_id = ++m_next_timer_id;
    
    SoftwareTimer new_timer = {
        .id = new_id,
        .expiry_time = current_time + duration,
        .duration = duration,
        .callback = callback,
        .context = context,
        .periodic = periodic
    };

    // Insere na lista ordenada (mantendo o proximo timer a expirar no inicio)
    s_active_timers.push_back(new_timer);
    s_active_timers.sort(); 

    Log::debug(TAG, "Temporizador setado. ID: " + std::to_string(new_id) + ", Expira em: " + std::to_string(duration.count()) + "ns");

    // O hardware do timer precisa ser re-agendado se este for o mais proximo.
    // Exemplo: Scheduler::rescheduleNextHwTick(s_active_timers.front().expiry_time);
    
    return new_id;
}

void KernelTimer::handleHwTimerIrq() {
    // Esta funcao e chamada em contexto de IRQ/SoftIRQ. Deve ser rapida.
    SpinLock::Guard lock(m_lock);
    
    Nanoseconds current_time = Scheduler::getKernelTime();
    
    while (!s_active_timers.empty() && s_active_timers.front().expiry_time <= current_time) {
        SoftwareTimer expired_timer = s_active_timers.front();
        s_active_timers.pop_front();
        
        Log::info(TAG, "Temporizador " + std::to_string(expired_timer.id) + " expirou.");
        
        // Dispara o callback (executado em contexto de kernel thread)
        // Isso deve ser delegado a uma thread de kernel para nao bloquear o IRQ.
        Scheduler::dispatchDeferredCall([=]() {
            expired_timer.callback(expired_timer.context);
        });

        if (expired_timer.periodic) {
            // Reagendar o temporizador periodico
            expired_timer.expiry_time += expired_timer.duration;
            s_active_timers.push_back(expired_timer);
            s_active_timers.sort();
        }
    }
    
    // Reagendar o proximo tick de hardware com base no proximo timer (se existir)
    // if (!s_active_timers.empty()) { ... }
}

bool KernelTimer::cancelTimer(uint32_t timer_id) {
    SpinLock::Guard lock(m_lock);

    for (auto it = s_active_timers.begin(); it != s_active_timers.end(); ++it) {
        if (it->id == timer_id) {
            s_active_timers.erase(it);
            Log::info(TAG, "Temporizador ID " + std::to_string(timer_id) + " cancelado.");
            // Verificar se o hardware precisa de reagendamento
            return true;
        }
    }
    Log::warn(TAG, "Tentativa de cancelar ID " + std::to_string(timer_id) + " nao encontrado.");
    return false;
}

} // namespace kernel
} // namespace comandro
