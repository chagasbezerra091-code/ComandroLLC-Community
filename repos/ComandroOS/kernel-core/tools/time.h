#ifndef COMANDRO_KERNEL_TOOLS_TIME_H
#define COMANDRO_KERNEL_TOOLS_TIME_H

#include <comandro/kernel/types.h>
#include <chrono>

namespace comandro {
namespace kernel {
namespace time {

// =====================================================================
// TIME_H - Kernel Time and Scheduling Utilities
// =====================================================================

// Tipos de Alta Precisao (Hardware Clock)
using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;

// Constantes de Tempo
static constexpr Nanoseconds NANOS_PER_SECOND = Nanoseconds(1000000000);
static constexpr Nanoseconds NANOS_PER_MS = Nanoseconds(1000000);

/**
 * @brief Estrutura que representa o tempo do sistema (Epoch) de forma detalhada.
 */
struct SystemTime {
    uint64_t seconds;       // Segundos desde a Epoch (1970-01-01)
    uint32_t nanoseconds;   // Nanosegundos dentro do segundo atual
};

/**
 * @brief O TimeUtils fornece acesso ao relógio de hardware (H/W Clock)
 * e servicos de temporizacao de kernel de alta precisao.
 */
class TimeUtils {
public:

    /**
     * @brief Obtem a contagem atual do relogio do kernel de alta resolucao.
     * * Esta e a fonte mais precisa de tempo no kernel.
     * @return O tempo atual em nanosegundos (desde o boot ou outra referencia).
     */
    static Nanoseconds getHighResTime();

    /**
     * @brief Obtem o tempo do sistema (wall clock time) atual.
     * * Este tempo e ajustado via NTP e pode ter jitter.
     */
    static SystemTime getSystemTime();

    /**
     * @brief Converte um tempo absoluto de Nanoseconds para a estrutura SystemTime.
     */
    static SystemTime toSystemTime(Nanoseconds absolute_time);

    /**
     * @brief Adiciona um temporizador one-shot ao agendador do kernel.
     * @param duration Duracao para o disparo.
     * @param callback A funcao a ser executada no vencimento.
     * @param is_real_time Se o temporizador deve ter prioridade de RT.
     * @return Um ID do temporizador para cancelamento, ou 0 em caso de falha.
     */
    using TimerCallback = void (*)(void* context);
    static uint32_t setKernelTimer(Nanoseconds duration, TimerCallback callback, void* context, bool is_real_time);

    /**
     * @brief Cancela um temporizador de kernel ativo.
     * @param timer_id O ID retornado por setKernelTimer.
     * @return true se o temporizador foi encontrado e cancelado.
     */
    static bool cancelKernelTimer(uint32_t timer_id);
    
private:
    // Nao instanciável (classe estática de utilidade)
    TimeUtils() = delete;
};

// =====================================================================
// Helpers para Manipulacao de Tempo (Inline/Template)
// =====================================================================

/**
 * @brief Converte qualquer duracao do std::chrono para Nanoseconds.
 */
template <typename Rep, typename Period>
Nanoseconds toNanos(const std::chrono::duration<Rep, Period>& duration) {
    return std::chrono::duration_cast<Nanoseconds>(duration);
}

/**
 * @brief Funcao utilitaria para realizar um busy-wait (nao usar em threads de aplicacao).
 */
static inline void busyWaitMicroseconds(uint32_t us) {
    // Implementacao de calibracao de loop de kernel
    // (Acesso direto ao contador de ciclo da CPU ou timer loop)
    TimeUtils::getHighResTime(); // Leitura inicial
    // ...
}


} // namespace time
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_TOOLS_TIME_H
