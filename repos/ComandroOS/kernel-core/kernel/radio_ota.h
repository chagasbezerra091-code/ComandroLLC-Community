#ifndef COMANDRO_KERNEL_RADIO_OTA_H
#define COMANDRO_KERNEL_RADIO_OTA_H

#include <comandro/kernel/types.h>
#include <comandro/kernel/spinlock.h>
#include <comandro/kernel/thread.h>
#include <string>

namespace comandro {
namespace kernel {

// =====================================================================
// RADIO_OTA_H - Gerenciamento de Atualizacao Over-The-Air (OTA)
// Responsavel por receber, validar e aplicar patches e imagens de kernel.
// =====================================================================

// Constantes de Particao e Tamanho de Bloco
static constexpr size_t OTA_BLOCK_SIZE = 4096; // Tamanho de bloco de escrita
static constexpr size_t OTA_MAX_IMAGE_SIZE = 512 * 1024 * 1024; // 512 MB max.

// Enumeração de Estados do Processo OTA
enum class OtaState {
    IDLE,               // Nenhuma operacao em andamento
    RECEIVING_METADATA, // Recebendo informacoes sobre o patch
    DOWNLOADING,        // Baixando blocos de dados
    VERIFYING,          // Verificando assinatura e hash (Integridade)
    FLASHING_CRITICAL,  // Escrevendo dados nas particoes criticas (Kernel, Bootloader)
    COMPLETE,           // Atualizacao bem-sucedida
    FAILED              // Falha na operacao
};

/**
 * @brief O OtaUpdateManager (OTAM) gerencia todo o ciclo de vida da atualizacao OTA.
 * * As operacoes criticas de I/O de Flash/eMMC sao executadas por uma thread de alta prioridade.
 */
class OtaUpdateManager {
public:
    static OtaUpdateManager& instance();

    /**
     * @brief Inicia o processo de escuta para uma nova atualizacao OTA.
     * @return true se o listener de rede foi ativado com sucesso.
     */
    bool startOtaListener();

    /**
     * @brief Ponto de entrada chamado pelo driver de rede quando metadados OTA sao recebidos.
     * @param metadata_json Dados que descrevem o patch (versao, hash, tamanho).
     * @return true se os metadados sao validos e o download pode comecar.
     */
    bool processOtaMetadata(const std::string& metadata_json);

    /**
     * @brief Processa um bloco de dados recebido do servidor OTA.
     * * Esta funcao enfileira os dados para a thread de escrita.
     * @param block_id O indice do bloco recebido.
     * @param data O buffer contendo os dados do bloco.
     * @param length O tamanho dos dados recebidos (deve ser OTA_BLOCK_SIZE).
     * @return true se o bloco foi aceito e enfileirado.
     */
    bool receiveOtaBlock(uint32_t block_id, const uint8_t* data, size_t length);

    /**
     * @brief Retorna o estado atual do processo de atualizacao.
     */
    OtaState getCurrentState() const { return m_state; }

    /**
     * @brief Fornece o progresso da atualizacao (0 a 100).
     */
    uint8_t getProgress() const;

private:
    OtaUpdateManager();

    // Variaveis de Estado
    volatile OtaState m_state;
    volatile uint32_t m_total_blocks;
    volatile uint32_t m_received_blocks;
    SpinLock m_state_lock;

    kernel::Thread::TID m_write_thread_tid;

    /**
     * @brief Thread de kernel responsavel pela escrita e verificacao (alta prioridade).
     */
    static void otaWriteAndVerifyLoop(void* arg);
    
    /**
     * @brief Verifica a assinatura criptografica da imagem baixada.
     */
    bool verifyImageSignature(const std::string& expected_hash);

    /**
     * @brief Escreve os blocos da imagem na particao de destino.
     */
    bool flashImageToPartition();

};

} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_RADIO_OTA_H
