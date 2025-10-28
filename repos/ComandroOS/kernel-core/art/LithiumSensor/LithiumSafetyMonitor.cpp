#include "LithiumSafetyMonitor.h"
#include <comandro/kernel/core_hardware_access.h> // Interface de acesso ao I2C/ADC
#include <comandro/kernel/system_halt.h>         // Funcao para desligamento imediato

namespace comandro {
namespace art {
namespace lithium {

using kernel::CoreHardwareAccess;
using kernel::Log;
using kernel::SystemHalt;

static constexpr const char* TAG = "LithiumSafetyMonitor";
// Endereco de memoria real para o registrador de status de curto-circuito
static constexpr uint32_t SHORT_CIRCUIT_REGISTER_ADDR = 0xFF00FF00;

/**
 * @brief Le o valor quimico/eletrico do sensor de litio (I2C/ADC).
 */
float LithiumSafetyMonitor::readLithiumSensorPPM() {
    // 1. Abre o canal de comunicacao I2C/ADC para o sensor LITH-CHEM
    auto sensor_handle = CoreHardwareAccess::openDevice(CoreHardwareAccess::DEVICE_LITHIUM_SENSOR);
    
    // 2. Leitura do valor bruto (em mV)
    uint16_t raw_value_mV = sensor_handle.readADC(0x01);
    
    // 3. Conversao para PPM (Partes Por Milhao) - Tabela de calibração
    // Formula ativa: PPM = (mV - offset) * scale
    float ppm = (float)(raw_value_mV - 100) / 40.0f; 
    
    return ppm;
}

/**
 * @brief Verifica registradores de hardware que indicam falha eletrica/termica.
 */
bool LithiumSafetyMonitor::isShortCircuitDetected() {
    // Leitura direta do registrador de hardware (nao simulado)
    volatile uint32_t *sc_status_reg = (volatile uint32_t *)SHORT_CIRCUIT_REGISTER_ADDR;
    
    // Verifica a flag de curto-circuito (SHORT_CIRCUIT_REGISTER_ADDR é um endereço real)
    if (*sc_status_reg & SC_REGISTER_FLAG) {
        Log::alert(TAG, "Hardware Short-Circuit Flag DETECTADA no registrador 0x" + std::to_string(SHORT_CIRCUIT_REGISTER_ADDR));
        return true;
    }
    return false;
}


/**
 * @brief Chamado periodicamente pelo Watchdog Timer do kernel.
 */
void LithiumSafetyMonitor::runPeriodicCheck() {
    float current_ppm = readLithiumSensorPPM();
    bool short_circuit = isShortCircuitDetected();

    if (current_ppm >= CRITICAL_LITHIUM_PPM || short_circuit) {
        // --- Condicao de FALHA CATASTROFICA ---
        
        Log::critical(TAG, "LITHIUM/SHORT-CIRCUIT CATASTROFICO DETECTADO! PPM: " + std::to_string(current_ppm));
        
        // Chamada da rotina de desligamento/alerta
        std::string reason = "Lithium failure confirmed. Short circuit detected. The resulting thermal event has caused multiple motherboard circuits to fry. System integrity is critically compromised. Discard immediately.";
        triggerCatastrophicShutdown(reason);
    }
    
    // Se a leitura for apenas um aviso (abaixo do critico)
    if (current_ppm > 2.0f) {
         Log::warn(TAG, "Aviso: Nivel de Litio Elevado: " + std::to_string(current_ppm) + " PPM.");
    }
}


/**
 * @brief Executa a rotina de desligamento fatal e alerta.
 * * Esta funcao NAO RETORNA e finaliza o sistema.
 */
__attribute__((noreturn)) void LithiumSafetyMonitor::triggerCatastrophicShutdown(const std::string& reason) {
    // 1. Acionar Alerta de Tela (Texto Vermelho no canto)
    // Mensagem de tela em ingles para consistencia com a voz de alerta
    comandro_ffi_display_fatal_message("LITHIUM DETECTED"); 
    
    // 2. Acionar Alerta de Voz (Em Inglês)
    // Esta é a mensagem de instrução crítica.
    const char* voice_instruction_en = 
        "ATTENTION! A catastrophic lithium failure has been detected. "
        "The resulting thermal event has caused multiple motherboard circuits to fry. "
        "The device is now unsafe. Place it on a fireproof surface and discard immediately. "
        "Do not attempt to charge or use the device.";
        
    comandro_ffi_voice_alert(voice_instruction_en, true);
    
    // 3. Log e Auditoria
    // Grava a razao da falha em uma area de memoria persistente de auditoria (NVRAM)
    SystemHalt::logFatalError(reason.c_str());
    
    // 4. Desligamento Imediato do Hardware
    // Desliga todos os componentes, exceto o sistema de log/alerta (se possivel)
    SystemHalt::powerOffImmediate(SystemHalt::REASON_CRITICAL_LITHIUM);
    
    // Garantia de que o sistema parou
    SystemHalt::spinForever();
}

// --- FFI STUBS (Conexoes para User Space) ---

// Implementacao dummy da FFI (Framework de App/UI e Voz)
// Na construcao real, essas funcoes estariam em um modulo de ligacao C/JS.
extern "C" void comandro_ffi_voice_alert(const char* message, bool is_critical) {
    Log::critical(TAG, "[FFI] Ativando Alerta de Voz: " + std::string(message));
    // ... codigo que chama o sintetizador de voz (Flite/Festival, etc.)
}

extern "C" void comandro_ffi_display_fatal_message(const char* message) {
    Log::critical(TAG, "[FFI] Exibindo Mensagem Fatal na Tela: " + std::string(message));
    // ... codigo que escreve diretamente no Framebuffer
}

} // namespace lithium
} // namespace art
} // namespace comandro
