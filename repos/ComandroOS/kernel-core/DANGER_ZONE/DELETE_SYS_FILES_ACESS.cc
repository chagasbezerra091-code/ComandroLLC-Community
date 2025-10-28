#include <comandro/kernel/security/delete_sys_files_access.h>
#include <comandro/kernel/security/danger_zone_access.h> // Para verificar o estado OEM
#include <comandro/kernel/security/audit_log.h>
#include <comandro/kernel/process_manager.h>
#include <comandro/kernel/log.h>
#include <string>
#include <set>
#include <mutex>

// =====================================================================
// DELETE_SYS_FILES_ACESS.cc - Politica Anti-Destruicao de Arquivos Criticos
// Implementa regras de kernel para BLOQUEAR a exclusao de arquivos e diretórios essenciais.
// =====================================================================

namespace comandro {
namespace kernel {
namespace security {
namespace files {

// Conjunto de hash de caminhos de arquivos/diretorios de EXCLUSAO PROIBIDA.
// Usamos std::set<std::string> para pesquisa rapida e estatica.
static const std::set<std::string> FORBIDDEN_DELETE_PATHS = {
    "/boot/vmlinuz",                        // Kernel binary
    "/boot/initrd.img",                     // Initial ramdisk
    "/etc/security/",                       // Diretório de configuracao de seguranca
    "/proc/sys/",                           // Configurações do sistema de kernel
    "/sbin/init",                           // Processo de inicializacao principal
    "/usr/bin/binder_service_manager",      // Binder core
    "/usr/lib/libc.so",                     // C standard library
    "/usr/lib/libcomandro_crypto.so",       // Biblioteca de criptografia
    "/kernel_modules/mandatory_drivers/",   // Drivers criticos
    "/var/log/audit.log"                    // Log de Auditoria
};

/**
 * @brief Verifica se um determinado caminho ou seu diretorio pai e um alvo de exclusao proibida.
 * @param path O caminho do arquivo/diretorio que se deseja excluir.
 * @return true se a exclusao for explicitamente proibida pelo kernel.
 */
bool is_delete_forbidden(const std::string& path) {
    // 1. Verificacao de correspondencia exata
    if (FORBIDDEN_DELETE_PATHS.count(path) > 0) {
        return true;
    }
    
    // 2. Verificacao de prefixo (para proteger diretorios inteiros)
    for (const auto& forbidden_path : FORBIDDEN_DELETE_PATHS) {
        // Se o path comeca com um dos caminhos proibidos (e.g., '/etc/security/rules.conf' vs '/etc/security/')
        if (path.rfind(forbidden_path, 0) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Politica de controle de acesso para a operacao UNLINK (exclusao/delecao).
 * Esta e a funcao critica chamada pelo VFS antes de deletar qualquer inode.
 * @param pid ID do processo solicitante.
 * @param path Caminho do arquivo/diretorio que se deseja excluir.
 * @return true se a exclusao for permitida.
 */
bool check_delete_access(pid_t pid, const std::string& path) {
    // 1. O Kernel (PID 0) SEMPRE PODE DELETAR. (Assumimos que o kernel sabe o que faz)
    if (pid == 0) {
        return true;
    }
    
    // 2. Verificacao de Proibicao
    if (is_delete_forbidden(path)) {
        
        // Log de falha de seguranca: tentativa de EXCLUSAO de arquivo critico.
        audit::log_security_event(
            "DELETE BLOCK: Processo " + std::to_string(pid) + " tentou excluir arquivo ESSENCIAL: " + path,
            audit::SECURITY_LEVEL_EMERGENCY_BLOCK);
        
        // Retorna falso: Bloqueio absoluto da exclusão.
        return false;
    }
    
    // 3. Verificacao de Modo OEM (Alerta)
    if (danger_zone_access::get_current_unlock_status() == 2) {
        // Mesmo se o arquivo nao for da lista FORBIDDEN, avisamos no modo perigoso.
        LOG_WARN("Permitindo DELECAO em modo OEM_UNLOCKED: %s", path.c_str());
    }

    // 4. Caso Padrao: Permitir a exclusao, dependendo das regras UGO normais do VFS.
    // O VFS continuara com a verificacao de permissoes normais (ex: é dono do arquivo?).
    return true; 
}

/**
 * @brief Servico Binder/Dexter: Alterna o estado de Protecao de Exclusao.
 * Somente o Root Service com o token OEM_UNLOCKED pode desabilitar temporariamente a protecao.
 * @param enable Se deve habilitar (true) ou desabilitar (false) o modulo temporariamente.
 * @return true se a operacao foi bem-sucedida.
 */
bool binder_set_delete_protection_state(bool enable) {
    // Requer desbloqueio de software COMPLETO.
    if (danger_zone_access::get_current_unlock_status() != 2) {
        audit::log_security_event("DELETE_LOCK: Tentativa de bypass sem permissao OEM.", 
                                  audit::SECURITY_LEVEL_ALERT);
        return false;
    }
    
    // A logica real de bypass de protecao seria implementada aqui (e.g., unset uma flag VFS)
    // Para fins de simulacao:
    if (!enable) {
        LOG_CRITICAL("DELETE_PROTECTION DESABILITADA! Sistema em risco extremo.");
    } else {
        LOG_INFO("DELETE_PROTECTION REABILITADA.");
    }

    // Assumindo sucesso para o Root Service em DANGER_ZONE.
    return true;
}

} // namespace files
} // namespace security
} // namespace kernel
} // namespace comandro
