#ifndef COMANDRO_ART_LITHIUM_SAFETY_MONITOR_H
#define COMANDRO_ART_LITHIUM_SAFETY_MONITOR_H

#include <comandro/kernel/core_io.h>
#include <comandro/kernel/log.h>
#include <string>

namespace comandro {
namespace art {
namespace lithium {

// Constantes de Limiar de Segurança
static constexpr float CRITICAL_LITHIUM_PPM = 5.0f; // Limiar de Partes Por Milhao (PPM)
static constexpr uint32_t SC_REGISTER_FLAG = 0x01; // Flag de Curto-Circuito no registrador de hardware

/**
 * @brief Gerencia o monitoramento de litio e a resposta a falhas catastroficas.
 */
class LithiumSafetyMonitor {
public:
    LithiumSafetyMonitor() = default;
    
    /**
     * @brief Chamado periodicamente pelo Watchdog Timer do kernel.
     */
    void runPeriodicCheck();

private:
    /**
     * @brief Le o valor quimico/eletrico do sensor de litio (I2C/ADC).
     * * Realiza uma leitura de baixo nivel de hardware.
     */
    float readLithiumSensorPPM();

    /**
     * @brief Verifica registradores de hardware que indicam falha eletrica/termica.
     * * Especificamente verifica curtos-circuitos graves.
     */
    bool isShortCircuitDetected();

    /**
     * @brief Executa a rotina de desligamento fatal e alerta.
     * * Esta funcao nao retorna.
     */
    __attribute__((noreturn)) void triggerCatastrophicShutdown(const std::string& reason);

    // FFI (Foreign Function Interface) para o User Space/Framework
    // Funcao que aciona o modulo de voz e o rendering de tela (Js/Ocaml)
    extern "C" void comandro_ffi_voice_alert(const char* message, bool is_critical);
    extern "C" void comandro_ffi_display_fatal_message(const char* message);
};

} // namespace lithium
} // namespace art
} // namespace comandro

#endif // COMANDRO_ART_LITHIUM_SAFETY_MONITOR_H
