#ifndef COMANDRO_KERNEL_BLUETOOTH_H
#define COMANDRO_KERNEL_BLUETOOTH_H

#include <comandro/kernel/types.h>
#include <comandro/kernel/thread.h>
#include <comandro/kernel/ipc/ComandroIpcBus.h>
#include <string>
#include <array>

namespace comandro {
namespace kernel {

// =====================================================================
// BLUETOOTH_H - Comandro Bluetooth Subsystem (CBSS)
// Protocolo HCI e Gerenciamento de Conexao de Baixa Latencia
// =====================================================================

// Tipos de Endereco Bluetooth
using BluetoothAddress = std::array<uint8_t, 6>; // BD_ADDR (e.g., [AA:BB:CC:DD:EE:FF])

// Protocolos de Servico
enum class BluetoothProfile {
    GENERIC_ACCESS,
    LE_AUDIO,           // Prioridade Real-Time
    A2DP_SINK,          // Streaming de Audio
    HID_GAMING,         // Human Interface Device (Baixa Latencia)
    LE_DATA             // Generic LE data
};

// Estrutura de Conexao Ativa
struct ConnectionHandle {
    uint16_t handle_id;
    BluetoothAddress remote_addr;
    BluetoothProfile active_profile;
    bool is_encrypted;
    Thread::TID data_processor_tid; // Thread RT responsavel pelo processamento de dados
};


/**
 * @brief O Comandro Bluetooth Subsystem (CBSS) gerencia o stack Bluetooth (HCI/L2CAP/RFCOMM).
 * * Focado em Garantias de Tempo Real (RT) para perfis de Audio e HID.
 */
class BluetoothManager {
public:
    static BluetoothManager& instance();

    /**
     * @brief Inicializa o chip Bluetooth (Modo HCI e firmware).
     * @return true se o hardware e o firmware foram carregados com sucesso.
     */
    bool initializeHardware();

    /**
     * @brief Inicia uma busca ativa por dispositivos nas proximidades.
     */
    void startScan();

    /**
     * @brief Inicia uma conexao assincrona a um endereco especifico.
     * @param addr Endereco do dispositivo a conectar.
     * @param profile Perfil de servico desejado.
     * @return O Handle da conexao ou 0 se falhar imediatamente.
     */
    uint16_t initiateConnection(const BluetoothAddress& addr, BluetoothProfile profile);

    /**
     * @brief Envia um pacote de dados de baixa latencia.
     * * Usado para perfis criticos como HID e LE Audio.
     * @param handle Handle da conexao.
     * @param data Payload a ser enviado.
     * @param length Tamanho do Payload.
     * @return Quantidade de bytes escritos.
     */
    size_t sendLowLatencyData(uint16_t handle, const uint8_t* data, size_t length);
    
    /**
     * @brief Desconecta um dispositivo e finaliza a thread RT associada.
     */
    void disconnect(uint16_t handle);

    /**
     * @brief Callback do kernel chamado pelo driver HCI quando novos dados sao recebidos.
     * * Deve ser executado em contexto de Interrupt/SoftIRQ.
     */
    void handleHciDataReceived(uint16_t handle, const uint8_t* data, size_t length);

private:
    BluetoothManager();

    SpinLock m_connection_lock;
    std::array<ConnectionHandle, 32> m_active_connections; // Limite de 32 conexoes ativas

    /**
     * @brief Mapeia um perfil para uma prioridade de thread do scheduler.
     */
    Thread::Priority mapProfileToPriority(BluetoothProfile profile);
    
    /**
     * @brief Envia um comando de controle HCI para o hardware.
     */
    bool sendHciCommand(uint16_t opcode, const uint8_t* params, size_t param_len);

    /**
     * @brief Rotina principal de processamento de dados da thread de tempo real.
     */
    static void connectionDataLoop(void* arg); 
};

} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_BLUETOOTH_H
