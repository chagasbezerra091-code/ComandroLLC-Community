#ifndef COMANDRO_KERNEL_SHUTDOWN_MANAGER_H
#define COMANDRO_KERNEL_SHUTDOWN_MANAGER_H

#include <comandro/kernel/thread.h>
#include <comandro/kernel/timer.h>
#include <comandro/kernel/log.h>
#include <chrono>

namespace comandro {
namespace kernel {
namespace shutdown {

// Tipos de Acao de Shutdown
enum class ShutdownAction {
    HALT,           // Desligamento total do sistema
    REBOOT_NORMAL,  // Reinicializacao normal
    REBOOT_RECOVERY // Reinicializacao para o modo de recuperacao
};

// Codigos de Saida de Shutdown
enum class ShutdownReason {
    USER_REQUEST,
    CRITICAL_ERROR,
    LOW_BATTERY,
    SYSTEM_UPDATE
};

/**
 * @brief Gerencia e orquestra a sequencia de desligamento/reinicializacao do sistema.
 * * Garante a finalizacao graciosa de servicos e o sync de disco.
 */
class ShutdownManager {
public:
    static ShutdownManager& instance();

    /**
     * @brief Inicia o procedimento de desligamento ou reinicializacao.
     * @param action O tipo de acao (desligar ou reiniciar).
     * @param reason A razao pela qual o shutdown foi solicitado.
     */
    void initiateShutdown(ShutdownAction action, ShutdownReason reason);

private:
    ShutdownManager() = default;

    /**
     * @brief Executa a sequencia de finalizacao graciosa do sistema.
     * @param timeout Tempo maximo permitido para finalizacao.
     */
    void gracefulFinalizationSequence(std::chrono::milliseconds timeout);

    /**
     * @brief Notifica o User Space e espera pela finalizacao dos Apps.
     */
    bool notifyUserSpaceAndAwait(std::chrono::milliseconds timeout);

    /**
     * @brief Sincroniza todos os sistemas de arquivos.
     */
    void syncFilesystems();
    
    /**
     * @brief Acao final de hardware que nao retorna.
     */
    __attribute__((noreturn)) void finalHardwareAction(ShutdownAction action);
};

} // namespace shutdown
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_SHUTDOWN_MANAGER_H
