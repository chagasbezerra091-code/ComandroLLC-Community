//! oem_security_token.rs
//! Modulo Rust para validacao do token de desbloqueio OEM (eFuse/Hardware).

use comandro_kernel::hw_registers;
use comandro_kernel::sync::SpinLock;
use comandro_kernel::log;
use core::sync::atomic::{AtomicBool, Ordering};

// Endereco simulado do registrador de status de desbloqueio de hardware (eFuse)
const OEM_UNLOCK_REGISTER_ADDR: usize = 0xFF00_00A0;
const OEM_UNLOCK_MAGIC_VALUE: u32 = 0xDEADC0DE; 

// Estado de Desbloqueio, cacheado após a inicializacao.
static IS_OEM_UNLOCKED: AtomicBool = AtomicBool::new(false);

// Contador de tentativas falhas de acesso a ZONA DANGER
static FAILED_ACCESS_COUNT: SpinLock<u32> = SpinLock::new(0);

pub struct OemSecurityToken;

impl OemSecurityToken {
    /// @brief Verifica o estado de desbloqueio no hardware e cacheia o resultado.
    /// Esta funcao deve ser chamada apenas uma vez durante a inicializacao do kernel (bootloader).
    pub fn initial_token_check() {
        log::info!("DANGER_ZONE: Iniciando verificacao do token OEM...");
        
        // 1. Le o valor do registrador de hardware (eFuse simulada)
        let hw_value = unsafe {
            // Acesso direto ao endereço de memoria mapeado, operacao insegura!
            let reg_ptr = OEM_UNLOCK_REGISTER_ADDR as *const u32;
            reg_ptr.read_volatile()
        };

        if hw_value == OEM_UNLOCK_MAGIC_VALUE {
            IS_OEM_UNLOCKED.store(true, Ordering::Release);
            log::critical!("DANGER_ZONE: !!! MODO DE DESBLOQUEIO OEM ATIVO !!!");
            log::critical!("DANGER_ZONE: A seguranca do sistema esta COMPROMETIDA.");
        } else {
            IS_OEM_UNLOCKED.store(false, Ordering::Release);
            log::info!("DANGER_ZONE: Dispositivo bloqueado (Production Mode).");
        }
    }
    
    /// @brief Leitura atomica do estado de desbloqueio. 
    /// Usado por subsistemas do kernel (e.g., Flashing, Debug).
    pub fn is_oem_unlocked() -> bool {
        IS_OEM_UNLOCKED.load(Ordering::Acquire)
    }

    /// @brief Incrementa o contador de tentativas falhas de acesso a DANGER ZONE.
    pub fn increment_failed_access() {
        let mut count = FAILED_ACCESS_COUNT.lock();
        *count += 1;
        log::alert!("DANGER_ZONE: Tentativa de acesso nao autorizado. Contagem: {}", *count);

        if *count > 5 {
             // Acao de seguranca: Se houver muitas tentativas falhas, forcar um reboot de seguranca.
             // [system_control::initiate_security_reboot("TOO_MANY_FAILED_UNLOCK_ATTEMPTS")]
        }
    }

    /// @brief Reseta o contador apos um acesso bem-sucedido.
    pub fn reset_failed_access() {
        let mut count = FAILED_ACCESS_COUNT.lock();
        *count = 0;
    }
}
