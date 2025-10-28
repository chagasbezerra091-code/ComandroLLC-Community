#include <stdint.h>
#include <stdbool.h>
#include "hardware_init.h"
#include "boot_modes.h"
#include "log_display.h"

// =====================================================================
// boot_entry.c - Ponto de entrada do Bootloader (UEFI/U-Boot customizado)
// Responsavel por inicializar o hardware, ler flags e decidir o modo de boot.
// =====================================================================

// Endereços de memória para flags de boot (simulacao de registradores eMMC/NVRAM)
#define MEM_ADDR_BOOT_FLAG          (0x80000000)
#define MEM_ADDR_KERNEL_START       (0x80200000)
#define MEM_ADDR_RECOVERY_START     (0x80400000)

// Tipos de Modo de Boot
typedef enum {
    BOOT_MODE_NORMAL = 0x00,
    BOOT_MODE_RECOVERY = 0x01,
    BOOT_MODE_FASTBOOT = 0x02,
    BOOT_MODE_FACTORY = 0x03
} BootMode;

// Ponteiro para o endereço de memória da flag de boot
static volatile BootMode *g_boot_flag = (volatile BootMode *)MEM_ADDR_BOOT_FLAG;

/**
 * @brief Ponto de entrada principal do bootloader, chamado pela ROM.
 * (Analogia a um ambiente UEFI ou U-Boot customizado para o SoC ARM64).
 */
void main_boot_entry(void) {
    BootMode requested_mode;

    // 1. Inicializacao do Hardware Minimo (Assembly/Microcode)
    init_clocks();
    init_memory_controller();
    
    // 2. Inicializa o display basico para log (modo de texto)
    log_display_init();
    
    // 3. Le o modo de boot solicitado (por software ou combinacao de teclas/eMMC)
    requested_mode = *g_boot_flag;
    
    log_display_print("[BOOTLOADER]: Hardware Inicializado. Verificando Modo de Boot.\n");
    
    // Zera a flag de boot imediatamente apos a leitura para evitar loop de modo especial
    *g_boot_flag = BOOT_MODE_NORMAL; 
    
    // 4. Logica de Decisao do Boot
    switch (requested_mode) {
        case BOOT_MODE_RECOVERY:
            log_display_print("[BOOTLOADER]: Modo Recovery Detectado. Carregando imagem de recuperacao.\n");
            load_and_jump_to_image(MEM_ADDR_RECOVERY_START);
            break;

        case BOOT_MODE_FASTBOOT:
            log_display_print("[BOOTLOADER]: Modo Fastboot/Download Detectado. Entrando em modo de servico.\n");
            enter_fastboot_mode();
            break;
            
        case BOOT_MODE_FACTORY:
            log_display_print("[BOOTLOADER]: Modo de Teste de Fabrica. Executando testes de burn-in.\n");
            run_factory_tests();
            break;

        case BOOT_MODE_NORMAL:
        default:
            log_display_print("[BOOTLOADER]: Modo Normal. Carregando Kernel ComandroOS.\n");
            // 5. Carrega e Inicia o Kernel Principal
            load_and_jump_to_image(MEM_ADDR_KERNEL_START);
            break;
    }

    // Se a execucao chegar aqui, significa que a imagem falhou ao carregar
    log_display_error("[FATAL]: Falha ao carregar a imagem. Parando CPU.\n");
    halt_cpu(); 
}


// --- Funcoes Externas (Definidas em outros modulos .c) ---

/**
 * @brief Carrega a imagem do endereco de inicio (eMMC/Flash) e salta para ela.
 * Na pratica, isso envolve copiar a imagem para a RAM e desabilitar caches/MMU.
 */
void load_and_jump_to_image(uint32_t start_address) {
    // Simulacao: Inicia a imagem no endereco de memoria
    void (*kernel_entry)() = (void (*)())start_address;
    
    log_display_print("[BOOTLOADER]: Transferencia de controle para 0x%X\n", start_address);
    // Desabilita as interrupcoes antes do salto
    disable_interrupts();
    // Flush/invalida o cache
    invalidate_caches_and_tlb(); 
    
    kernel_entry(); // Salto para o kernel
    
    // O kernel_entry nunca deve retornar.
    halt_cpu(); 
}

// Funcoes dummy para completar o codigo
void init_clocks() { /* ... */ }
void init_memory_controller() { /* ... */ }
void enter_fastboot_mode() { /* Loop de comandos USB/Serial */ }
void run_factory_tests() { /* ... */ }
void log_display_init() { /* ... */ }
void log_display_print(const char* msg) { /* ... */ }
void log_display_error(const char* msg) { /* ... */ }
void halt_cpu() { while(1) {} }
void disable_interrupts() { /* ... */ }
void invalidate_caches_and_tlb() { /* ... */ }

