#ifndef COMANDRO_KERNEL_SOS_POLICE_MANAGER_H
#define COMANDRO_KERNEL_SOS_POLICE_MANAGER_H

#include <comandro/kernel/thread.h>
#include <comandro/kernel/gpio.h>
#include <comandro/kernel/log.h>
#include <chrono>

namespace comandro {
namespace kernel {
namespace sos {

// Constantes de Emergencia
static constexpr int SOS_BUTTON_GPIO = 17; // Exemplo: Pino GPIO para botao de panico dedicado/multi-pressionamento
static constexpr int MAX_RESCUE_ATTEMPTS = 5;

// Estrutura de Dados de Localizacao
struct GpsLocation {
    double latitude;
    double longitude;
    float accuracy_meters;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief O SosPoliceManager (SPM) gerencia o modo de emergencia e o rastreamento critico.
 * * Opera com a mais alta prioridade de kernel (RT-Critical).
 */
class SosPoliceManager {
public:
    static SosPoliceManager& instance();

    /**
     * @brief Ponto de entrada chamado por um Handler de Interrupcao (IRQ).
     * * Disparado por um evento de hardware (p. ex., botao de panico).
     */
    void handleEmergencyIRQ();

    /**
     * @brief Tenta enviar o pacote de localizacao e dados para o servico de emergencia.
     * @return true se o pacote foi transmitido com sucesso.
     */
    bool sendEmergencyPackage(const GpsLocation& location);

    /**
     * @brief Consulta o status atual de emergencia.
     */
    bool isEmergencyActive() const { return m_is_emergency_active; }

private:
    SosPoliceManager();
    
    // Variaveis de Estado
    volatile bool m_is_emergency_active;
    volatile int m_attempt_counter;
    kernel::Thread::TID m_rt_tracking_tid; // TID da thread de rastreamento Real-Time

    /**
     * @brief Inicia a thread de rastreamento de alta prioridade.
     */
    void startRealTimeTrackingThread();
    
    /**
     * @brief Funcao executada pela thread de rastreamento (RT-Critical).
     */
    static void realTimeTrackingLoop(void* arg);

    /**
     * @brief Obtem a localizacao atual do hardware GPS/GNSS.
     */
    GpsLocation getGpsLocation();

    // FFI (Foreign Function Interface) para o modulo de comunicacao segura
    extern "C" bool comandro_ffi_secure_transmit(const uint8_t* data, size_t length);
};

} // namespace sos
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_SOS_POLICE_MANAGER_H
