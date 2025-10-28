#include "ShutdownManager.h"
#include <comandro/kernel/ipc/ComandroIpcBus.h>
#include <comandro/kernel/process_manager.h>
#include <comandro/kernel/power_controller.h>
#include <comandro/kernel/filesystem_sync.h>

namespace comandro {
namespace kernel {
namespace shutdown {

using kernel::Log;
using kernel::ipc::ComandroIpcBus;
using kernel::ProcessManager;
using kernel::PowerController;
using kernel::FilesystemSync;

static constexpr const char* TAG = "ShutdownManager";
static constexpr std::chrono::milliseconds GRACE_PERIOD = std::chrono::seconds(5);

ShutdownManager& ShutdownManager::instance() {
    static ShutdownManager s_instance;
    return s_instance;
}

void ShutdownManager::initiateShutdown(ShutdownAction action, ShutdownReason reason) {
    Log::alert(TAG, "Shutdown Iniciado. Acao: " + std::to_string(static_cast<int>(action)) + 
                      ", Razao: " + std::to_string(static_cast<int>(reason)));
    
    // 1. Sequencia de Finalizacao (Limite de 5 segundos)
    gracefulFinalizationSequence(GRACE_PERIOD);

    // 2. Acao Final de Hardware
    finalHardwareAction(action);
}

/**
 * @brief Executa a sequencia de finalizacao graciosa do sistema.
 */
void ShutdownManager::gracefulFinalizationSequence(std::chrono::milliseconds timeout) {
    Log::info(TAG, "Iniciando sequencia de finalizacao graciosa...");
    auto start_time = std::chrono::steady_clock::now();

    // 1. Notificar User Space (Apps, Services)
    if (!notifyUserSpaceAndAwait(timeout)) {
        Log::warn(TAG, "User Space nao finalizou a tempo. Forcando termino de processos remanescentes.");
        // Forca o termino de qualquer processo de usuario restante.
        ProcessManager::killAllUserProcesses();
    }

    // 2. Sincronizar todos os sistemas de arquivos (crucial para integridade)
    syncFilesystems();
    
    // 3. Finalizar Threads e Drivers de Kernel (menos criticos)
    ProcessManager::haltKernelThreads();
    
    auto elapsed_time = std::chrono::steady_clock::now() - start_time;
    Log::info(TAG, "Finalizacao graciosa concluida em " + 
                  std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count()) + "ms.");
}

/**
 * @brief Notifica o User Space (System Server) via C-Bus e espera pela finalizacao dos Apps.
 */
bool ShutdownManager::notifyUserSpaceAndAwait(std::chrono::milliseconds timeout) {
    // 1. Cria a mensagem de shutdown
    ipc::IpcMessage shutdown_msg;
    shutdown_msg.message_id = 0xDE01; // SHUTDOWN_REQUEST
    shutdown_msg.payload_size = 0;
    
    // 2. Envia para o no central do User Space (ID 1, tipicamente o SystemServer)
    ComandroIpcBus& bus = ComandroIpcBus::instance();
    if (!bus.sendAsync(1 /* SystemServer Node ID */, shutdown_msg)) {
        Log::error(TAG, "Falha ao notificar o SystemServer via C-Bus.");
        return false;
    }
    
    // 3. Espera pelo sinal de confirmacao ou timeout
    // O SystemServer enviaria uma mensagem de ACK ou mudaria um flag de estado
    // Simulacao de espera:
    return PowerController::waitForUserSpaceHalt(timeout);
}

/**
 * @brief Sincroniza todos os sistemas de arquivos.
 */
void ShutdownManager::syncFilesystems() {
    Log::alert(TAG, "Sincronizando todos os sistemas de arquivos... (SYNC)");
    
    // Chamada real para o kernel I/O
    FilesystemSync::syncAllData();
    
    Log::alert(TAG, "Sincronizacao concluida.");
}

/**
 * @brief Acao final de hardware que nao retorna.
 */
__attribute__((noreturn)) void ShutdownManager::finalHardwareAction(ShutdownAction action) {
    // Desmonta particoes e finaliza I/O
    FilesystemSync::unmountAllFilesystems();
    
    switch (action) {
        case ShutdownAction::HALT:
            Log::critical(TAG, "Desligando energia (HALT)...");
            PowerController::powerOff();
            break;
            
        case ShutdownAction::REBOOT_NORMAL:
            Log::critical(TAG, "Reiniciando o sistema (REBOOT NORMAL)...");
            PowerController::reboot(PowerController::REBOOT_MODE_NORMAL);
            break;
            
        case ShutdownAction::REBOOT_RECOVERY:
            Log::critical(TAG, "Reiniciando para Recovery (REBOOT RECOVERY)...");
            PowerController::reboot(PowerController::REBOOT_MODE_RECOVERY);
            break;
    }
    
    // Em caso de falha no PowerController, halt_cpu deve ser o ultimo recurso
    Log::fatal(TAG, "Acao de hardware falhou. CPU Halt forçado.");
    SystemHalt::spinForever(); // Ponto final, nao retorna
}

} // namespace shutdown
} // namespace kernel
} // namespace comandro
