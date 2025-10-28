#include "fastboot.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// Stubs para funcoes nativas de I/O de baixo nivel
extern "C" {
    // Le dados do host (USB/Serial). Retorna o numero de bytes lidos.
    size_t native_fb_read_command(char* buffer, size_t max_size);
    // Escreve dados para o host.
    void native_fb_write_data(const char* data, size_t size);
    // Realiza o flash real na particao de memoria
    int native_vfs_flash_partition(const char* partition_name, const void* data, size_t size);
    // Obtem uma variavel do sistema (ex: versao)
    const char* native_get_system_variable(const char* var_name);
    // Funcao de reboot de hardware
    void native_system_reboot(int mode);
}

namespace comandro {
namespace kernel {
namespace fastboot {

// Variaveis de estado interno
static uint8_t* download_buffer = nullptr;
static size_t download_size = 0;
static size_t current_data_expected = 0;

int initialize_fastboot() {
    printf("[FASTBOOT] Inicializando subsistema USB/Serial...\n");
    // [HARDWARE-INIT] native_usb_init();
    
    // Aloca buffer temporario para download (Ex: 16MB)
    // download_buffer = native_alloc_memory(16 * 1024 * 1024);
    if (!download_buffer) {
        printf("[FASTBOOT] ERRO: Falha ao alocar buffer de download.\n");
        return -1;
    }
    printf("[FASTBOOT] Buffer de download alocado.\n");
    return 0;
}

void send_response(const char* status_prefix, const char* message) {
    char response[FB_RESPONSE_MAX_SIZE];
    snprintf(response, FB_RESPONSE_MAX_SIZE, "%s%s", status_prefix, message ? message : "");
    
    // Garante que a resposta termine com \n ou \0 para envio seguro
    response[FB_RESPONSE_MAX_SIZE - 1] = '\0'; 
    native_fb_write_data(response, strlen(response));
}

// Manipulador principal de comandos
static void handle_command(char* command, size_t length) {
    char cmd_name[FB_COMMAND_MAX_SIZE];
    char arg1[FB_COMMAND_MAX_SIZE] = {0};
    
    // Tenta parsear o comando no formato "CMD ARG1"
    sscanf(command, "%s %s", cmd_name, arg1);
    
    // --- Comandos de Informacao (getvar) ---
    if (strcmp(cmd_name, FB_CMD_GETVAR) == 0) {
        const char* value = native_get_system_variable(arg1);
        if (value) {
            send_response(FB_STATUS_OKAY, value);
        } else {
            send_response(FB_STATUS_FAIL, "Variavel nao encontrada.");
        }
        return;
    }
    
    // --- Comandos de Transferencia (download) ---
    if (strcmp(cmd_name, FB_CMD_DOWNLOAD) == 0) {
        // Formato esperado: download:<tamanho_hex>
        if (arg1[0] != ':') {
             send_response(FB_STATUS_FAIL, "Formato DOWNLOAD incorreto.");
             return;
        }
        
        // Le o tamanho em hexadecimal apos o ':'
        current_data_expected = strtoul(arg1 + 1, NULL, 16);
        
        if (current_data_expected == 0 || current_data_expected > 16 * 1024 * 1024) {
            send_response(FB_STATUS_FAIL, "Tamanho de download invalido.");
            return;
        }
        
        download_size = 0;
        
        // Envia o prompt para o host para iniciar a transferencia de dados
        char msg[64];
        snprintf(msg, sizeof(msg), "%08lX", (unsigned long)current_data_expected); // Ex: 00800000
        send_response(FB_STATUS_INFO, msg); 
        
        // Loop de recebimento de dados (bloqueante)
        while (download_size < current_data_expected) {
            size_t bytes_to_read = std::min(current_data_expected - download_size, (size_t)FB_DATA_MAX_SIZE);
            
            // native_fb_read_data deve ler bytes_to_read e escrever em download_buffer + download_size
            // size_t bytes_read = native_fb_read_data(download_buffer + download_size, bytes_to_read);
            
            // Simulacao de sucesso de leitura
            size_t bytes_read = bytes_to_read; 
            
            if (bytes_read == 0) {
                // Erro de I/O ou desconexao
                send_response(FB_STATUS_FAIL, "I/O timeout.");
                return;
            }
            download_size += bytes_read;
        }
        
        send_response(FB_STATUS_OKAY, "Download concluido.");
        return;
    }
    
    // --- Comandos de Flash ---
    if (strcmp(cmd_name, FB_CMD_FLASH) == 0) {
        if (download_size == 0) {
            send_response(FB_STATUS_FAIL, "Nenhum dado baixado para flash.");
            return;
        }
        
        // arg1 contem o nome da particao (ex: system, boot, recovery)
        if (native_vfs_flash_partition(arg1, download_buffer, download_size) == 0) {
             send_response(FB_STATUS_OKAY, "Particao flashed com sucesso.");
        } else {
             send_response(FB_STATUS_FAIL, "Falha ao gravar na particao.");
        }
        // Opcional: Limpar download_size e buffer aqui
        return;
    }
    
    // --- Comandos de Controle (reboot) ---
    if (strcmp(cmd_name, FB_CMD_REBOOT) == 0) {
        send_response(FB_STATUS_OKAY, "Reiniciando o sistema...");
        native_system_reboot(0); // 0 = Modo normal
        return;
    }
    
    // --- Comando Desconhecido ---
    send_response(FB_STATUS_FAIL, "Comando desconhecido ou malformado.");
}


void fastboot_main_loop() {
    char command_buffer[FB_COMMAND_MAX_SIZE];
    
    while (1) {
        // Aguarda um novo comando do host
        size_t bytes_read = native_fb_read_command(command_buffer, FB_COMMAND_MAX_SIZE);
        
        if (bytes_read > 0) {
            // Garante que o buffer seja null-terminated para seguranca do C++
            command_buffer[bytes_read] = '\0';
            handle_command(command_buffer, bytes_read);
        } else {
            // Em caso de I/O timeout ou desconexao, pode-se reiniciar ou dar reboot.
            // native_system_reboot(1); // Ex: Rebootar em modo Fastboot
        }
    }
}

} // namespace fastboot
} // namespace kernel
} // namespace comandro
