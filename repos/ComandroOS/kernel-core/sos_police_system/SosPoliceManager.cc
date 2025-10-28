#include "SosPoliceManager.h"
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/gps_driver.h>
#include <comandro/kernel/secure_comm_driver.h>
#include <comandro/kernel/memory.h>
#include <comandro/kernel/util.h>

namespace comandro {
namespace kernel {
namespace sos {

using kernel::Log;
using kernel::Scheduler;
using kernel::GpsDriver;
using kernel::SecureCommDriver;

static constexpr const char* TAG = "SosPoliceManager";

SosPoliceManager& SosPoliceManager::instance() {
    static SosPoliceManager s_instance;
    return s_instance;
}

SosPoliceManager::SosPoliceManager() 
    : m_is_emergency_active(false), 
      m_attempt_counter(0),
      m_rt_tracking_tid(0) 
{
    // Inicializa o GPIO para o botao/trigger SOS (Associa IRQ)
    kernel::Gpio::set_irq_handler(SOS_BUTTON_GPIO, [this](){ this->handleEmergencyIRQ(); });
    Log::info(TAG, "SPM inicializado. IRQ do botao SOS pronto no GPIO " + std::to_string(SOS_BUTTON_GPIO));
}

/**
 * @brief Ponto de entrada chamado por um Handler de Interrupcao (IRQ).
 */
void SosPoliceManager::handleEmergencyIRQ() {
    // Esta funcao deve ser rapida e executada em contexto de IRQ (nao pode bloquear)
    
    if (m_is_emergency_active) {
        // Ignora presses adicionais se ja estiver ativo
        return;
    }
    
    Log::alert(TAG, "INTERRUPCAO DE EMERGENCIA (SOS) RECEBIDA!");
    
    m_is_emergency_active = true;
    
    // Inicia a thread de rastreamento imediatamente
    startRealTimeTrackingThread();

    // Notifica o User Space para desabilitar telas e microfone/camera (privacidade/seguranca)
    // ComandroIpcBus::instance().sendAsync(SYSTEM_SERVER_NODE, SOS_ACTIVATED_MSG); 
}

/**
 * @brief Inicia a thread de rastreamento de alta prioridade.
 */
void SosPoliceManager::startRealTimeTrackingThread() {
    // 1. Cria a thread com a maior prioridade de tempo real (RT-Critical)
    // Isso garante que o scheduler nunca a fara esperar por outras tarefas.
    kernel::ThreadAttributes attrs;
    attrs.priority = Scheduler::RT_PRIORITY_CRITICAL;
    attrs.name = "SOS_RT_Track";

    kernel::Thread::create(realTimeTrackingLoop, this, attrs, m_rt_tracking_tid);
    
    Log::critical(TAG, "Thread de rastreamento RT-Critical iniciada. TID: " + std::to_string(m_rt_tracking_tid));
}

/**
 * @brief Funcao executada pela thread de rastreamento (RT-Critical).
 */
void SosPoliceManager::realTimeTrackingLoop(void* arg) {
    SosPoliceManager* self = static_cast<SosPoliceManager*>(arg);
    
    // Ciclo de transmissao: 500ms (baixa latencia)
    std::chrono::milliseconds interval = std::chrono::milliseconds(500); 

    while (self->m_is_emergency_active && self->m_attempt_counter < MAX_RESCUE_ATTEMPTS) {
        
        // 1. Obtem a localizacao atual
        GpsLocation current_location = self->getGpsLocation();
        
        // 2. Tenta transmitir
        if (self->sendEmergencyPackage(current_location)) {
            Log::info(TAG, "Pacote SOS #" + std::to_string(self->m_attempt_counter) + 
                          " transmitido com sucesso. Lat: " + std::to_string(current_location.latitude));
            
            // Se o pacote for enviado com sucesso, o sistema espera por um ACK externo.
            // Aqui, apenas incrementamos o contador.
            self->m_attempt_counter++;
        } else {
            Log::error(TAG, "Falha na transmissao SOS. Re-tentando...");
            // Nao incrementamos o contador em caso de falha de transmissao (re-tentar)
        }
        
        // Dorme pelo intervalo de tempo real
        kernel::Thread::sleep_rt(interval);
    }
    
    // Se o loop terminar (por limite de tentativas ou desativacao externa)
    Log::warn(TAG, "Thread de rastreamento SOS finalizada.");
    self->m_is_emergency_active = false;
}


GpsLocation SosPoliceManager::getGpsLocation() {
    // Chamada real ao driver de kernel do GPS/GNSS
    GpsDriver::LocationData raw_data = GpsDriver::getHighRateLocation();
    
    GpsLocation loc;
    loc.latitude = raw_data.lat;
    loc.longitude = raw_data.lon;
    loc.accuracy_meters = raw_data.hdop * 1.5f; // Exemplo de conversao
    loc.timestamp = std::chrono::system_clock::now();
    
    return loc;
}

bool SosPoliceManager::sendEmergencyPackage(const GpsLocation& location) {
    // 1. Serializacao dos dados (Formato binario seguro)
    uint8_t buffer[128];
    size_t payload_size = 0;
    
    // Serializa coordenadas, timestamp e device ID
    payload_size += kernel::memory::serialize_double(buffer + payload_size, location.latitude);
    payload_size += kernel::memory::serialize_double(buffer + payload_size, location.longitude);
    // ... outros dados (ex: ID do dispositivo, nivel de bateria)
    
    // 2. Transmissao Segura (FFI para o modulo de modem/antena de baixa potencia)
    // O comandro_ffi_secure_transmit usaria um canal de radio dedicado (se disponivel) ou SMS/LTE
    if (comandro_ffi_secure_transmit(buffer, payload_size)) {
        return true;
    }
    return false;
}


// --- FFI STUB (Conexao para o Driver de Comunicacao Segura) ---
// Na implementacao real, este seria o driver de kernel de radio de baixa potencia
extern "C" bool comandro_ffi_secure_transmit(const uint8_t* data, size_t length) {
    // Simulacao de latencia de transmissao
    // SecureCommDriver::sendEncryptedPacket(data, length, SecureCommDriver::POLICE_ENDPOINT);
    
    // Retorna sucesso para simular o envio.
    return true; 
}


} // namespace sos
} // namespace kernel
} // namespace comandro
