#include "oem_security_token.rs.h" // Interface Rust FFI
#include <comandro/kernel/security/audit_log.h>
#include <comandro/kernel/ipc/binder.h>
#include <mutex>
#include <string>

// =====================================================================
// danger_zone_access.cc - Gerenciamento de Acesso e Auditoria da DANGER ZONE
// Implementa as funcoes restritas e o controle de acesso por senha/token.
// =====================================================================

namespace comandro {
namespace kernel {
namespace security {

// Senha mestra simulada (Em um sistema real, seria um hash complexo/chave PKI)
const std::string OEM_MASTER_PASSWORD = "Comandro_OS_PDK_Security_Key_0xDEADBEEF";

/**
 * @brief Inicializa o subsistema de seguranca da DANGER ZONE.
 * Deve ser chamado antes de qualquer outra funcao do kernel.
 */
void initialize_danger_zone() {
    // A funcao Rust verifica o estado de hardware (eFuse) uma unica vez no boot.
    oem_security_token::initial_token_check();
}


// -------------------------------------------------------------------
// Funcoes de Interface (Binder/Dexter)
// -------------------------------------------------------------------

/**
 * @brief Servico Binder: Tenta desbloquear a DANGER ZONE com uma senha de software.
 * NOTA: O desbloqueio de software so funciona se o OEM Unlock de hardware ja estiver ativo.
 * @param password Senha de tentativa.
 * @return true se o desbloqueio de software foi bem-sucedido.
 */
bool binder_unlock_danger_zone(const std::string& password) {
    // 1. Verifica se o desbloqueio de hardware esta ativo (via Rust)
    if (!oem_security_token::is_oem_unlocked()) {
        audit::log_security_event("DANGER_ZONE: Tentativa falha (Hardware Lock)", 
                                  audit::SECURITY_LEVEL_BLOCK);
        oem_security_token::increment_failed_access();
        return false;
    }

    // 2. Valida a senha de software (ultima camada de protecao)
    if (password == OEM_MASTER_PASSWORD) {
        oem_security_token::reset_failed_access();
        
        // 3. Log de Auditoria
        audit::log_security_event("DANGER_ZONE: Desbloqueio de Software BEM-SUCEDIDO!", 
                                  audit::SECURITY_LEVEL_CRITICAL);

        // 4. Ativa Flags de Alto Nivel (Expondo APIs restritas)
        binder::activate_restricted_apis(true);
        return true;
    } else {
        audit::log_security_event("DANGER_ZONE: Senha de Software INCORRETA.", 
                                  audit::SECURITY_LEVEL_ALERT);
        oem_security_token::increment_failed_access();
        return false;
    }
}

/**
 * @brief Servico Binder: Verifica o estado atual de desbloqueio de software/hardware.
 * @return 1 se Hardware desbloqueado, 2 se Software desbloqueado (Acesso Completo), 0 se Bloqueado.
 */
int get_current_unlock_status() {
    if (binder::is_restricted_apis_active()) {
        return 2; // Acesso completo (Software + Hardware)
    }
    if (oem_security_token::is_oem_unlocked()) {
        return 1; // Apenas Hardware desbloqueado (Acesso limitado/Bootloader)
    }
    return 0; // Bloqueado
}

} // namespace security
} // namespace kernel
} // namespace comandro
