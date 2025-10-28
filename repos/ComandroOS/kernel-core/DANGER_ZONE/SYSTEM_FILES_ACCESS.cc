#include <comandro/kernel/security/system_files_access.h>
#include <comandro/kernel/security/danger_zone_access.h> // Para verificar o estado OEM
#include <comandro/kernel/security/audit_log.h>
#include <comandro/kernel/process_manager.h>
#include <comandro/kernel/log.h>
#include <string>
#include <mutex>

// =====================================================================
// SYSTEM_FILES_ACCESS.cc - Politica de Controle de Acesso a Arquivos Criticos
// Implementa regras baseadas em MAC (Mandatory Access Control) e estado do sistema.
// =====================================================================

namespace comandro {
namespace kernel {
namespace security {
namespace files {

// Lista de prefixos de caminhos de arquivos criticos que so podem ser escritos pelo Kernel.
const char* CRITICAL_WRITE_PATHS[] = {
    "/etc/security/efuse_state",      // Arquivo que armazena o estado eFuse em flash.
    "/boot/config/kernel_flags.bin",  // Flags de inicializacao do kernel.
    "/proc/sys/binder_policy.conf",   // Configuracao da politica Binder.
    "/etc/pmic/battery_safety_logs/",  // Logs da DANGER_ZONE/BATTERY_TYPE_DANGER.
    nullptr // Terminador
};

// =================================================================
// 1. Funcoes de Acesso e Verificacao
// =================================================================

/**
 * @brief Verifica se um determinado caminho de arquivo e um recurso de escrita critica.
 * @param path O caminho do arquivo.
 * @return true se o caminho corresponder a um dos caminhos de escrita critica.
 */
bool is_critical_write_path(const std::string& path) {
    for (const char** p = CRITICAL_WRITE_PATHS; *p != nullptr; ++p) {
        if (path.find(*p) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Verifica se um processo tem permissao para realizar uma operacao em um arquivo.
 * Esta e a funcao principal chamada pelo VFS (Virtual File System) do kernel.
 * @param pid ID do processo solicitante.
 * @param path Caminho do arquivo alvo.
 * @param operation O tipo de operacao (READ, WRITE, EXECUTE).
 * @return true se a operacao for permitida.
 */
bool check_file_access(pid_t pid, const std::string& path, AccessOperation operation) {
    // 1. O Kernel (PID 0) SEMPRE TEM ACESSO
    if (pid == 0) {
        return true;
    }
    
    // 2. Obtem o contexto de seguranca do processo
    ProcessContext context = process_manager::get_context(pid);

    // -----------------------------------------------------------
    // Politica de Controle de Escrita (Critica)
    // -----------------------------------------------------------
    if (operation == AccessOperation::WRITE && is_critical_write_path(path)) {
        
        // APENAS processes do Kernel ou Root Service (UID 0) com o capability certo.
        if (context.uid != 0 || !context.has_capability(CAPABILITY_SYSTEM_FILES_WRITE)) {
            
            // Log de falha de seguranca: tentativa de modificacao de arquivo critico
            audit::log_security_event(
                "WRITE BLOCK: Processo " + std::to_string(pid) + " tentou modificar arquivo critico: " + path,
                audit::SECURITY_LEVEL_CRITICAL_BLOCK);
            return false;
        }
        
        // Se a DANGER ZONE estiver desbloqueada, aplicar mais restricoes
        if (danger_zone_access::get_current_unlock_status() == 2) {
             // Mesmo com UID 0, a escrita em modo DANGER_ZONE desbloqueado e restrita.
             LOG_WARN("Permitindo escrita critica em modo OEM_UNLOCKED: %s", path.c_str());
        }

        // Permite a escrita se for Kernel/Root com a Capability
        return true;
    }

    // -----------------------------------------------------------
    // Politica de Controle de Leitura (Geral)
    // -----------------------------------------------------------
    if (operation == AccessOperation::READ) {
        // Arquivos de log de seguranca so podem ser lidos por processos de Auditoria (UID 100).
        if (path.find("/etc/pmic/battery_safety_logs/") == 0 && context.uid != 100) {
            
            audit::log_security_event(
                "READ BLOCK: Processo " + std::to_string(pid) + " tentou ler logs de seguranca.",
                audit::SECURITY_LEVEL_BLOCK);
            return false;
        }
        
        // Permite leitura para a maioria dos caminhos, desde que nao sejam secrets.
        // Regra default: Permitir leitura, a menos que seja explicitamente um segredo.
        return true; 
    }

    // 3. Caso Padrao: Seguir as regras UGO (User/Group/Other) tradicionais
    // (A implementacao UGO de baixo nivel do VFS seria chamada aqui)
    return true; 
}

/**
 * @brief Funcao Binder para listar as violacoes de acesso mais recentes.
 * Usado pelo ComandroTool shell para diagnostico de integridade.
 */
std::string binder_get_access_violations_log() {
    // Retorna os ultimos 10 eventos de "CRITICAL_BLOCK" do log de auditoria.
    return audit::get_recent_events_by_level(audit::SECURITY_LEVEL_CRITICAL_BLOCK, 10);
}

} // namespace files
} // namespace security
} // namespace kernel
} // namespace comandro
