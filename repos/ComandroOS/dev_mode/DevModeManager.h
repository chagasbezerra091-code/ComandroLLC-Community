#ifndef COMANDRO_DEV_MODE_MANAGER_H
#define COMANDRO_DEV_MODE_MANAGER_H

#include <comandro/kernel/types.h>
#include <string>
#include <map>

namespace comandro {
namespace dev_mode {

// =====================================================================
// DEV_MODE_MANAGER_H - Gerenciamento do Modo Desenvolvedor
// =====================================================================

/**
 * @brief Gerencia as flags de feature e configuracoes especificas de desenvolvimento.
 * * Este modulo DEVE ser excluído das builds de producao (Release).
 */
class DevModeManager {
public:
    static DevModeManager& instance();

    /**
     * @brief Inicializa o modo desenvolvedor, lendo as configuracoes.
     * @param config_path Caminho para o arquivo de configuracao (e.g., /data/dev/config.json).
     */
    void initialize(const std::string& config_path);

    /**
     * @brief Retorna se o modo desenvolvedor esta atualmente ativo no sistema.
     */
    bool isActive() const { return m_is_active; }

    /**
     * @brief Verifica o status de uma flag de recurso (feature flag) especifica.
     * @param feature_name O nome da feature a ser verificada (e.g., "ENABLE_RT_DEBUG").
     * @return true se a feature estiver ativada na configuracao.
     */
    bool isFeatureEnabled(const std::string& feature_name) const;

private:
    DevModeManager();
    volatile bool m_is_active;
    std::map<std::string, bool> m_feature_flags;

    /**
     * @brief Carrega as flags do arquivo JSON.
     */
    bool loadConfigFromFile(const std::string& path);
};

} // namespace dev_mode
} // namespace comandro

#endif // COMANDRO_DEV_MODE_MANAGER_H
