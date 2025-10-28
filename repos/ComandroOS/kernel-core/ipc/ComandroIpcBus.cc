#include "ComandroIpcBus.h"
#include <comandro/kernel/spinlock.h>
#include <comandro/kernel/log.h>
#include <cstring>

namespace comandro {
namespace kernel {
namespace ipc {

using kernel::Log;
using kernel::SpinLock;
using kernel::Thread;

static constexpr const char* TAG = "ComandroIpcBus";
static SpinLock s_registration_lock;

ComandroIpcBus& ComandroIpcBus::instance() {
    static ComandroIpcBus s_instance;
    return s_instance;
}

ComandroIpcBus::ComandroIpcBus() : m_next_node_id(1) {
    // Inicializa a lista de nos e semaforos
    for (uint32_t i = 0; i < MAX_BUS_NODES; ++i) {
        m_nodes[i].is_active = false;
        // Inicializa o semaforo com contagem 0 (bloqueado ate receber mensagem)
        m_nodes[i].message_semaphore.init(0); 
        m_nodes[i].rx_buffer.head = 0;
        m_nodes[i].rx_buffer.tail = 0;
    }
    Log::info(TAG, "Comandro IPC Bus (C-Bus) inicializado. Max nos: " + std::to_string(MAX_BUS_NODES));
}

BusNodeID ComandroIpcBus::registerService(const std::string& service_name, Thread::TID tid) {
    s_registration_lock.lock();
    
    if (m_next_node_id >= MAX_BUS_NODES) {
        s_registration_lock.unlock();
        Log::error(TAG, "Falha ao registrar servico. Limite de nos alcancado.");
        return 0;
    }

    BusNodeID new_id = m_next_node_id++;
    BusNode& node = m_nodes[new_id];
    
    node.service_name = service_name;
    node.receiver_tid = tid;
    node.is_active = true;
    
    s_registration_lock.unlock();
    Log::info(TAG, "Servico " + service_name + " registrado no C-Bus com ID: " + std::to_string(new_id));
    return new_id;
}

// --- Funcoes de Ring Buffer Lock-Minimized ---

/**
 * @brief Funcao auxiliar: Enfileira dados no Ring Buffer.
 * (Note: A logica do ring buffer é O(1) e pode ser quase lock-free se a arquitetura suportar atomicos)
 */
static bool ring_buffer_enqueue(RingBuffer& buffer, const uint8_t* data, uint32_t len) {
    uint32_t head = buffer.head;
    uint32_t tail = buffer.tail;
    
    // Calcula o espaco livre
    uint32_t free_space = (head <= tail) 
                          ? (RING_BUFFER_SIZE - tail + head) % RING_BUFFER_SIZE 
                          : head - tail;
    
    if (free_space < len + sizeof(uint32_t)) { // + tamanho para o header do comprimento
        return false; // Buffer cheio
    }

    // Escreve o comprimento do payload primeiro (para leitura mais facil)
    uint32_t current_tail = tail;
    // Buffer::write_atomic(&buffer.data[current_tail], &len, sizeof(uint32_t)); // Lock-free write
    current_tail = (current_tail + sizeof(uint32_t)) % RING_BUFFER_SIZE;
    
    // Escreve o payload
    // Buffer::write_data_chunk(buffer.data, current_tail, data, len); // Lock-free write
    // Simplificacao de escrita:
    if (current_tail + len > RING_BUFFER_SIZE) {
        // Envolve (wrap around)
        uint32_t part1_len = RING_BUFFER_SIZE - current_tail;
        std::memcpy(&buffer.data[current_tail], data, part1_len);
        std::memcpy(&buffer.data[0], data + part1_len, len - part1_len);
    } else {
        std::memcpy(&buffer.data[current_tail], data, len);
    }
    
    // Atualiza o tail (deve ser um write atomico para ser lock-free real)
    buffer.tail = (current_tail + len) % RING_BUFFER_SIZE; 
    return true;
}

// Implementacao de envio assincrono
bool ComandroIpcBus::sendAsync(BusNodeID destination, const IpcMessage& message) {
    if (destination == 0 || destination >= MAX_BUS_NODES || !m_nodes[destination].is_active) {
        Log::warn(TAG, "Tentativa de enviar mensagem para no inativo/invalido: " + std::to_string(destination));
        return false;
    }
    
    BusNode& node = m_nodes[destination];
    
    // A logica real usaria um semaforo de escrita ou uma primitiva de lock-free
    // para proteger a escrita do tail. Aqui, a funcao ring_buffer_enqueue simula isso.
    if (ring_buffer_enqueue(node.rx_buffer, (const uint8_t*)&message, sizeof(IpcMessage))) {
        // Sinaliza o semaforo para acordar a thread receptora
        node.message_semaphore.signal(); 
        return true;
    }
    
    Log::error(TAG, "Falha ao enviar mensagem: buffer cheio.");
    return false;
}

// Implementacao de recebimento
bool ComandroIpcBus::receive(BusNodeID self_id, IpcMessage& out_message, std::chrono::milliseconds timeout) {
    if (self_id == 0 || self_id >= MAX_BUS_NODES || !m_nodes[self_id].is_active) {
        return false;
    }

    BusNode& node = m_nodes[self_id];

    // 1. Espera pelo semaforo (a thread fica bloqueada pelo kernel scheduler)
    if (!node.message_semaphore.wait(timeout)) {
        return false; // Timeout
    }

    // 2. Leitura Lock-free/Atomic da Fila
    uint32_t head = node.rx_buffer.head;
    uint32_t tail = node.rx_buffer.tail;
    
    if (head == tail) {
        // Logica de falha: Acordou, mas o buffer esta vazio. (Spurious Wakeup)
        return false;
    }

    // 3. Le o comprimento do payload (omitido na simplificacao)

    // 4. Le o payload
    // Buffer::read_data_chunk(buffer.data, head, (uint8_t*)&out_message, sizeof(IpcMessage)); // Lock-free read
    // Simplificacao de leitura:
    if (head + sizeof(IpcMessage) > RING_BUFFER_SIZE) {
        // Envolve (wrap around)
        uint32_t part1_len = RING_BUFFER_SIZE - head;
        std::memcpy((uint8_t*)&out_message, &node.rx_buffer.data[head], part1_len);
        std::memcpy((uint8_t*)&out_message + part1_len, &node.rx_buffer.data[0], sizeof(IpcMessage) - part1_len);
    } else {
        std::memcpy((uint8_t*)&out_message, &node.rx_buffer.data[head], sizeof(IpcMessage));
    }

    // 5. Atualiza o head (deve ser um write atomico)
    node.rx_buffer.head = (head + sizeof(IpcMessage)) % RING_BUFFER_SIZE;
    
    return true;
}

} // namespace ipc
} // namespace kernel
} // namespace comandro
