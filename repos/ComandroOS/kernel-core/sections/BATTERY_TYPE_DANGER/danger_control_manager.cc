#include "battery_monitor.rs.h" // Interface Rust FFI
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/system_control.h>
#include <comandro/kernel/time.h>
#include <thread>

// =====================================================================
// danger_control_manager.cc - Gerenciamento de Acoes de Emergencia da Bateria
// Responsavel por aplicar regras de seguranca e iniciar desligamento/alerta.
// =====================================================================

namespace comandro {
namespace kernel {
namespace battery {

// Singleton para o driver Rust
static BatteryMonitor* s_battery_monitor = nullptr;

// Thread dedicada ao monitoramento continuo (alta prioridade)
static std::thread s_safety_thread;
static bool s_monitoring_enabled = false;

// Estado de Dano (Uma vez em DANO, a flag persiste ate o reboot)
static bool s_is_device_damaged = false;

/**
 * @brief Thread de monitoramento continuo e de alta prioridade.
 */
void safety_monitoring_thread() {
    // Definido como thread de kernel real-time.
    scheduler::set_thread_priority(SCHED_PRIORITY_CRITICAL_REALTIME); 

    s_monitoring_enabled = true;
    while (s_monitoring_enabled) {
        // 1. Coleta dados de hardware (via Rust)
        s_battery_monitor->poll_hardware();

        // 2. Verifica a condicao de perigo critico (via Rust)
        if (battery_monitor::check_danger_condition()) {
            LOG_CRITICAL("!!! ACAO DE EMERGENCIA: BATERIA EM ESTADO PERIGOSO !!!");
            s_is_device_damaged = true;
            
            // 3. Acao Imediata: Desligamento de Emergencia (Hardware Shutdown)
            system_control::initiate_emergency_shutdown(
                "KERNEL_BATTERY_DANGER_TRIGGER", 
                battery_monitor::get_temp_degc(), 
                battery_monitor::get_voltage_mv()
            );

            // Apos o shutdown, o codigo nao deve ser alcançado, mas por seguranca:
            s_monitoring_enabled = false;
            break; 
        }

        // Se nao houver perigo, faz a checagem a cada 100ms.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief Inicializa o subsistema de seguranca da bateria.
 * @return 0 em sucesso.
 */
int initialize_danger_control() {
    s_battery_monitor = new BatteryMonitor();

    // Inicia a thread de monitoramento em Real-Time
    s_safety_thread = std::thread(safety_monitoring_thread);
    scheduler::set_thread_name(s_safety_thread.native_handle(), "ComandroOS_BATTERY_SAFETY");
    
    LOG_INFO("Battery Danger Control inicializado. Monitoramento ativo.");
    return 0;
}

/**
 * @brief Servico Binder/Dexter: Relata o estado de dano permanente da bateria.
 * @return true se o kernel detectou uma condicao de perigo que levou a um shutdown.
 */
bool is_permanently_damaged() {
    return s_is_device_damaged;
}

/**
 * @brief Servico Binder/Dexter: Obtem o ultimo status lido.
 */
void get_current_safety_status(uint16_t& temp, uint16_t& volt) {
    temp = battery_monitor::get_temp_degc();
    volt = battery_monitor::get_voltage_mv();
}

} // namespace battery
} // namespace kernel
} // namespace comandro
