#include "DevModeManager.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/fs/file_io.h>
#include <iostream> // Usado apenas em Dev Mode
#include <sstream>

namespace comandro {
namespace dev_mode {

using kernel::Log;
using kernel::fs::FileIO;

static constexpr const char* TAG = "DevModeManager";
static constexpr const char* DEFAULT_CONFIG_PATH = "/system/dev/config.json";

DevModeManager& DevModeManager::instance() {
    static DevModeManager s_instance;
    return s_instance;
}

DevModeManager::DevModeManager() : m_is_active(false) {
    // Tenta inicializar com o caminho padrao (se nao for chamado explicitamente)
    initialize(DEFAULT_CONFIG_PATH);
}

void DevModeManager::initialize(const std::string& config_path) {
    if (loadConfigFromFile(config_path)) {
        m_is_active = true;
        Log::alert(TAG, "Modo Desenvolvedor ATIVO. Configuracoes carregadas de: " + config_path);
        
        // Imprime todas as flags ativas (Log Dev)
        Log::info(TAG, "Flags Ativas:");
        for (const auto& pair : m_feature_flags) {
            if (pair.second) {
                Log::info(TAG, "  - " + pair.first + ": true");
            }
        }
    } else {
        m_is_active = false;
        Log::warn(TAG, "Modo Desenvolvedor INATIVO. Arquivo de configuracao nao encontrado ou falha no parsing.");
    }
}

bool DevModeManager::isFeatureEnabled(const std::string& feature_name) const {
    if (!m_is_active) {
        return false;
    }
    auto it = m_feature_flags.find(feature_name);
    return (it != m_feature_flags.end() && it->second);
}

bool DevModeManager::loadConfigFromFile(const std::string& path) {
    // SIMULACAO: Leitura e parsing de um arquivo JSON
    // Em um kernel real, o JSON parsing seria feito por uma biblioteca de kernel leve.
    
    std::string config_content;
    if (!FileIO::readFileToString(path, config_content)) {
        return false; // Falha na leitura (e.g., arquivo nao existe)
    }

    // Simulacao de parsing de linhas (assumindo formato key: value)
    std::stringstream ss(config_content);
    std::string line;
    
    while (std::getline(ss, line, '\n')) {
        // Limpeza e busca por ':'
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value_str = line.substr(colon_pos + 1);
            
            // Trim e conversao
            key.erase(0, key.find_first_not_of(" \t\r"));
            key.erase(key.find_last_not_of(" \t\r") + 1);
            
            value_str.erase(0, value_str.find_first_not_of(" \t\r"));
            value_str.erase(value_str.find_last_not_of(" \t\r") + 1);
            
            bool value = (value_str == "true");
            m_feature_flags[key] = value;
        }
    }
    return true;
}

} // namespace dev_mode
} // namespace comandro
