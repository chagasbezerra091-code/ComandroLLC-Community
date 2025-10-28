#include <comandro/kernel/security/delete_oem.h>
#include <comandro/kernel/security/danger_zone_access.h> // Para verificar o estado OEM
#include <comandro/kernel/security/oem_security_token.rs.h> // Para a reescrita do token (Rust FFI)
#include <comandro/kernel/security/audit_log.h>
#include <comandro/kernel/system_control.h>
#include <comandro/kernel/log.h>
#include <string>

// =====================================================================
// DELETE_OEM.cc - Desativacao PERMANENTE do Desbloqueio OEM (Lock Bootloader)
// Esta operacao e destrutiva, irreversivel e requer multiplas verificacoes.
// =====================================================================

namespace comandro {
namespace kernel {
namespace security {
namespace oem {

// Senha de confirmacao final para a operacao de bloqueio.
const std::string LOCK_CONFIRMATION_PASSWORD = "LOCK_BOOTLOADER_PERMANENTLY_COMANDRO_OS";
// Tamanho minimo do buffer de log para garantir que o evento seja salvo antes do reboot.
const size_t MIN_AUDIT_LOG_BUFFER_SIZE = 1024; 

/**
 * @brief Implementa o bloqueio permanente do estado OEM, revertendo o dispositivo 
 * ao estado de seguranca de producao.
 * @param confirmation_key Senha final de confirmacao.
 * @return OperationStatus: SUCCESS, FAILURE_NOT_UNLOCKED, FAILURE_INVALID_KEY, FAILURE_HW_WRITE.
 */
OperationStatus lock_oem_permanently(const std::string& confirmation_key) {
    // 1. Verificacao de Estado Atual (Acesso completo e ativo)
    if (danger_zone_access::get_current_unlock_status() != 2) {
        LOG_ERROR("DELETE_OEM: Tentativa de bloqueio sem acesso completo de software/hardware.");
        audit::log_security_event("DELETE_OEM: Falha (Nao Desbloqueado)", audit::SECURITY_LEVEL_BLOCK);
        return OperationStatus::FAILURE_NOT_UNLOCKED;
    }

    // 2. Verificacao da Chave de Confirmacao Final
    if (confirmation_key != LOCK_CONFIRMATION_PASSWORD) {
        LOG_ERROR("DELETE_OEM: Chave de confirmacao final invalida.");
        audit::log_security_event("DELETE_OEM: Falha (Chave Invalida)", audit::SECURITY_LEVEL_CRITICAL_BLOCK);
        // Incrementar o contador de falhas da DANGER_ZONE.
        oem_security_token::increment_failed_access();
        return OperationStatus::FAILURE_INVALID_KEY;
    }
    
    // 3. Log de Auditoria Iminente (Antes da operacao destrutiva)
    audit::log_security_event(
        "!!! DELETE_OEM INICIADO: BLOQUEIO PERMANENTE DO DISPOSITIVO ATIVADO !!!", 
        audit::SECURITY_LEVEL_EMERGENCY);

    // 4. Operacao Destrutiva: Gravar o estado de BLOQUEIO no hardware (eFuse)
    LOG_CRITICAL("DELETE_OEM: Chamando FFI para gravar o token de bloqueio no hardware.");
    
    // Assume que esta funcao FFI (Rust) reescreve o registrador OEM_UNLOCK_REGISTER_ADDR
    // com um valor diferente de OEM_UNLOCK_MAGIC_VALUE.
    bool hw_write_success = oem_security_token::write_lock_token(); 

    if (!hw_write_success) {
        // Se a escrita de hardware falhar, o dispositivo nao esta em um estado seguro.
        LOG_CRITICAL("DELETE_OEM: ERRO FATAL de escrita de hardware. O token permanece incerto.");
        audit::log_security_event("DELETE_OEM: ERRO HW WRITE.", audit::SECURITY_LEVEL_EMERGENCY);
        return OperationStatus::FAILURE_HW_WRITE;
    }
    
    // 5. Commit Final e Reinicializacao
    audit::log_security_event("DELETE_OEM: Escrita HW BEM-SUCEDIDA. Reiniciando para aplicar o bloqueio.", 
                              audit::SECURITY_LEVEL_CRITICAL);

    // Garante que o log de auditoria foi gravado no disco antes do reboot.
    audit::flush_log_buffer(MIN_AUDIT_LOG_BUFFER_SIZE); 
    
    // Reinicia o sistema imediatamente para que o bootloader detecte o novo estado BLOQUEADO.
    system_control::initiate_reboot("OEM_LOCK_COMPLETE");
    
    // Este codigo nao deve ser alcancado apos o reboot.
    return OperationStatus::SUCCESS; 
}

/**
 * @brief Servico Binder/Dexter: Retorna se a operacao e possivel neste momento.
 */
bool binder_is_lock_operation_possible() {
    // A operacao e possivel se o dispositivo estiver atualmente desbloqueado.
    return oem_security_token::is_oem_unlocked();
}

} // namespace oem
} // namespace security
} // namespace kernel
} // namespace comandro
