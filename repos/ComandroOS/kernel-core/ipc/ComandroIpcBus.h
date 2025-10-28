#ifndef COMANDRO_KERNEL_IPC_BUS_H
#define COMANDRO_KERNEL_IPC_BUS_H

#include <comandro/kernel/thread.h>
#include <comandro/kernel/semaphore.h>
#include <comandro/kernel/types.h>
#include <string>
#include <chrono>

namespace comandro {
namespace kernel {
namespace ipc {

// Constantes
static constexpr size_t RING_BUFFER_SIZE = 4096; // 4KB por fila (otimizado para cache L1/L2)
static constexpr uint32_t MAX_BUS_NODES = 256; 

// Tipos
typedef uint32_t BusNodeID;
typedef struct {
    uint32_t message_id;
    uint16_t sender_tid;
    uint16_t payload_size;
    uint8_t payload[RING_BUFFER_SIZE - 8]; // Payload - Header Size
} IpcMessage;

// Estrutura do Ring Buffer (Memoria Compartilhada)
struct RingBuffer {
    volatile uint32_t head;
    volatile uint32_t tail;
    uint8_t data[RING_BUFFER_SIZE];
};

/**
 * @brief O Comandro IPC Bus (C-Bus).
 * * Responsavel pela comunicacao assincrona e sincrona de baixa latencia entre processos/threads.
 * * Usa Ring Buffers baseados em memoria compartilhada (lock-free/minimal-lock).
 */
class ComandroIpcBus {
public:
    static ComandroIpcBus& instance();

    /**
     * @brief Registra um novo servico no C-Bus.
     * @param service_name Nome do servico (e.g., "AudioService").
     * @param tid A thread ID do servico receptor.
     * @return O ID unico do no (BusNodeID) ou 0 se falhar.
     */
    BusNodeID registerService(const std::string& service_name, kernel::Thread::TID tid);

    /**
     * @brief Envia uma mensagem assincrona (nao bloqueante) para um no.
     * @return true se a mensagem foi enfileirada com sucesso.
     */
    bool sendAsync(BusNodeID destination, const IpcMessage& message);

    /**
     * @brief Recebe uma mensagem (bloqueante com timeout).
     */
    bool receive(BusNodeID self_id, IpcMessage& out_message, std::chrono::milliseconds timeout);

private:
    ComandroIpcBus();
    
    // Estrutura de dados para cada no no barramento
    struct BusNode {
        std::string service_name;
        kernel::Thread::TID receiver_tid;
        RingBuffer rx_buffer; // Buffer de Recebimento
        kernel::Semaphore message_semaphore; // Para sinalizar mensagens (sleep/wake)
        bool is_active;
    };

    BusNode m_nodes[MAX_BUS_NODES];
    volatile BusNodeID m_next_node_id;
};

} // namespace ipc
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_IPC_BUS_H
