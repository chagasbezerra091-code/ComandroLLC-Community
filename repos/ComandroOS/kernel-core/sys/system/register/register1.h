#ifndef COMANDRO_SYS_REGISTER1_H
#define COMANDRO_SYS_REGISTER1_H

// ====================================================================
// ComandroOS System Register 1 (REGISTER1.H)
// Arquivo de definições de baixo nível para endereços de hardware e flags.
// Usado por drivers (C/C++/Rust) e o kernel-core.
// ====================================================================

// --- 1. Endereços de Registradores de Hardware Críticos (Base) ---

// Endereço base para o Subsistema de Clock e Energia (CPM - Clock/Power Management)
#define REGISTER_BASE_CPM           0x01A00000UL

// Endereço base para o Controlador de Interrupção GIC (Global Interrupt Controller)
#define REGISTER_BASE_GIC           0x01A01000UL

// Endereço base para o Controlador DMA (Direct Memory Access)
#define REGISTER_BASE_DMA           0x01A02000UL

// Endereço base para o Controlador de Tela de Toque (usado pelo touchscreen.rs)
#define REGISTER_BASE_TOUCH_CTRL    0x01A03000UL


// --- 2. Deslocamentos (Offsets) de Registradores Comuns ---

// Deslocamento para o Registrador de Status do Sistema
#define REGISTER_OFFSET_STATUS      0x00000004UL

// Deslocamento para o Registrador de Controle de Energia (Power Control)
#define REGISTER_OFFSET_POWER_CTRL  0x00000008UL

// Deslocamento para o Registrador de Inicialização de Interrupt (IRQ Start)
#define REGISTER_OFFSET_IRQ_INIT    0x00000010UL


// --- 3. Constantes de Máscara (Flags) para Registradores ---

// Máscara para indicar o estado de "Modo de Debug Ativo" (usado no REG_STATUS)
#define REGISTER_FLAG_DEBUG_MODE    (1UL << 0)

// Máscara para indicar o estado de "Memória Crítica (OOM Killer Ativo)"
#define REGISTER_FLAG_OOM_ACTIVE    (1UL << 1)

// Máscara para desabilitar as interrupções globais (usado no REG_POWER_CTRL)
#define REGISTER_FLAG_IRQ_DISABLE   (1UL << 8)

// Máscara para indicar que a CPU deve ser Throttled (usado no REG_POWER_CTRL)
#define REGISTER_FLAG_CPU_THROTTLE  (1UL << 9)

// Máscara para indicar que o Slot B está marcado como a próxima partição de boot
#define REGISTER_FLAG_BOOT_SLOT_B   (1UL << 16)


// --- 4. Constantes de Tempo Críticas (Microsegundos) ---

// Latência máxima aceitável para I/O (1 milissegundo = 1000 microsegundos)
#define MAX_IO_LATENCY_US           1000UL

// Tempo limite para inicialização de drivers (50 milissegundos)
#define DRIVER_INIT_TIMEOUT_US      50000UL


// --- 5. Convenções de Nomes e Versões ---

// O nome do sistema operacional (para ser usado por drivers C/C++)
#define OS_NAME_SHORT               "COMANDRO_OS"

// Versão principal da ABI do kernel
#define KERNEL_ABI_VERSION          210 // 2.1.0

#endif // COMANDRO_SYS_REGISTER1_H
