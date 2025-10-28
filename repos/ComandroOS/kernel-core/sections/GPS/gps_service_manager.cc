#include "gps_device.rs.h" // Interface Rust FFI
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/ipc/binder.h>
#include <comandro/kernel/time.h>
#include <mutex>
#include <thread>

// =====================================================================
// gps_service_manager.cc - Gerenciador do Servico GPS do Kernel
// Responsavel por manter o estado, injetar efemerides e entregar dados via Binder.
// =====================================================================

namespace comandro {
namespace kernel {
namespace gps {

// Estrutura de dados de Posicao e Tempo (EPT)
struct GpsEpochTimePosition {
    double latitude;
    double longitude;
    double altitude;
    uint64_t kernel_epoch_ms; // Tempo do kernel no momento do fix
    uint64_t time_to_first_fix_ms;
};

// Singleton para o driver Rust FFI
static GpsDevice* s_gps_driver = nullptr;
// Mutex para proteger o dado de posicao mais recente
static std::mutex s_position_mutex;
static GpsEpochTimePosition s_last_position;

// Thread dedicada a leitura e parsing do NMEA
static std::thread s_nmea_thread;
static bool s_running = false;
static uint64_t s_fix_start_time = 0;

/**
 * @brief Thread principal que le dados NMEA do driver Rust.
 */
void nmea_parser_thread() {
    scheduler::set_thread_priority(SCHED_PRIORITY_BACKGROUND); // Baixa prioridade

    s_running = true;
    while (s_running) {
        // 1. Le linha NMEA do driver Rust (via FFI)
        auto nmea_line_option = s_gps_driver->read_nmea_line();
        
        if (nmea_line_option.has_value()) {
            std::string nmea_line = nmea_line_option.value();
            
            // 2. Parsar a linha NMEA (simulacao)
            if (nmea_line.find("$GPGGA") != std::string::npos) {
                // [LOGICA DE PARSING DETALHADA OMITIDA]
                double lat = 34.0522; 
                double lon = -118.2437;
                
                std::lock_guard<std::mutex> lock(s_position_mutex);
                s_last_position.latitude = lat;
                s_last_position.longitude = lon;
                s_last_position.kernel_epoch_ms = time::get_uptime_ms();
                
                if (s_fix_start_time != 0 && s_last_position.time_to_first_fix_ms == 0) {
                     s_last_position.time_to_first_fix_ms = time::get_uptime_ms() - s_fix_start_time;
                     LOG_INFO("GPS: Primeiro fix em %lu ms.", s_last_position.time_to_first_fix_ms);
                }
                
                // 3. Notificar o Servico Binder (para que as apps recebam a localizacao)
                binder::notify_gps_location_update(s_last_position.latitude, s_last_position.longitude);
            }
        }
        
        // Pequena pausa para nao sobrecarregar o loop
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

/**
 * @brief Inicializa o Gerenciador de Servico GPS.
 * @return 0 em sucesso.
 */
int initialize_gps_service() {
    s_gps_driver = new GpsDevice();
    s_fix_start_time = time::get_uptime_ms();

    // Cria e inicia a thread do parser NMEA
    s_nmea_thread = std::thread(nmea_parser_thread);
    scheduler::set_thread_name(s_nmea_thread.native_handle(), "ComandroOS_NMEA_Parser");
    
    LOG_INFO("GPS Service Manager iniciado. Parser thread ID: %d", s_nmea_thread.get_id());
    return 0;
}

/**
 * @brief Servico Binder: Injeta dados de assistencia (AGPS/Efemérides).
 * @param data Buffer de dados de correcao.
 */
void binder_inject_assistance_data(const std::string& data) {
    LOG_DEBUG("GPS: Dados de assistencia injetados. Tamanho: %zu", data.size());
    // [LOGICA DE INJECAO NO DRIVER RUST OMITIDA]
    // O driver Rust (gps_device.rs) deve ter uma funcao FFI para receber estes dados e enviá-los ao hardware.
}

} // namespace gps
} // namespace kernel
} // namespace comandro
