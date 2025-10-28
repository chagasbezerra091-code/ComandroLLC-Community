#ifndef COMANDRO_KERNEL_FASTBOOT_H
#define COMANDRO_KERNEL_FASTBOOT_H

#include <stddef.h>
#include <stdint.h>

// ====================================================================
// FASTBOOT.H - Definições e API Pública do Protocolo Fastboot
// ====================================================================

// --- 1. Tamanhos e Limites do Protocolo ---

// Tamanho maximo de um comando Fastboot (ex: "flash system <tamanho>")
#define FB_COMMAND_MAX_SIZE         64
// Tamanho maximo da resposta do dispositivo (ex: "OKAY[resposta]")
#define FB_RESPONSE_MAX_SIZE        64
// Tamanho maximo de um pacote de dados a ser transferido (por bloco)
#define FB_DATA_MAX_SIZE            (1024 * 1024) // 1MB por bloco


// --- 2. Códigos de Status (Prefixo de Resposta) ---

// Resposta bem-sucedida
#define FB_STATUS_OKAY              "OKAY"
// Resposta de erro (comando invalido, falha de flash)
#define FB_STATUS_FAIL              "FAIL"
// Resposta pendente (aguardando dados, ex: após comando 'download')
#define FB_STATUS_INFO              "INFO"


// --- 3. Comandos Suportados (Strings) ---

#define FB_CMD_GETVAR               "getvar"        // Obter variavel (ex: bootloader-version)
#define FB_CMD_FLASH                "flash"         // Flash em uma particao
#define FB_CMD_BOOT                 "boot"          // Bootar uma imagem temporaria
#define FB_CMD_DOWNLOAD             "download"      // Preparar para receber dados
#define FB_CMD_REBOOT               "reboot"        // Rebootar dispositivo
#define FB_CMD_REBOOT_FASTBOOT      "reboot-fastboot" // Rebootar de volta para o fastboot


// --- 4. API Pública (Para uso interno do Kernel) ---

namespace comandro {
namespace kernel {
namespace fastboot {

    /**
     * @brief Inicializa o subsistema Fastboot (USB/Serial I/O).
     * @return 0 em sucesso, -1 em falha.
     */
    int initialize_fastboot();

    /**
     * @brief Loop principal de Fastboot. Espera por comandos e envia respostas.
     * Deve ser executado em uma thread dedicada (PRIORITY_HIGH_IO).
     */
    void fastboot_main_loop();

    /**
     * @brief Envia uma resposta (OKAY, FAIL, INFO) para o host.
     * @param status_prefix FB_STATUS_OKAY, FB_STATUS_FAIL, etc.
     * @param message Mensagem de texto a ser anexada.
     */
    void send_response(const char* status_prefix, const char* message);

} // namespace fastboot
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_FASTBOOT_H
