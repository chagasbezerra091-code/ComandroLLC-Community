package comandro.packages.settings.accessibility;

import comandro.settings.debug.ComandroDebug;

/**
 * AccessibilityService: Gerenciador Central dos Recursos de Acessibilidade.
 *
 * Este serviço é responsável por habilitar e gerenciar recursos de acessibilidade
 * para garantir que o ComandroOS possa ser usado por todas as pessoas.
 *
 * NOTA: O ícone 'settings_accessibility' é usado na camada de UI/Desktop Environment
 * para representar este módulo de configurações.
 */
public final class AccessibilityService {

    // --- Constantes de Configuração ---
    private static final String ICON_NAME = "settings_accessibility";
    private static final String UI_LINK_REFERENCE = "<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200&icon_names=settings_accessibility\" />";
    
    // --- Flags de Estado ---
    private static boolean isHighContrastEnabled = false;
    private static boolean isScreenReaderEnabled = false;
    private static int fontSizeScale = 100; // 100% é o padrão

    private AccessibilityService() {
        // Classe estática
    }

    /**
     * Retorna o nome do ícone usado para representar este serviço na UI.
     * @return O nome do ícone do Material Symbols.
     */
    public static String getIconReference() {
        return ICON_NAME;
    }

    // --- Métodos de Controle ---

    /**
     * @brief Habilita ou desabilita o modo de alto contraste para a interface do sistema.
     * @param enable Se deve habilitar (true) ou desabilitar (false).
     */
    public static void setHighContrastMode(boolean enable) {
        if (isHighContrastEnabled != enable) {
            isHighContrastEnabled = enable;
            ComandroDebug.i("ACCESSIBILITY", "Modo Alto Contraste: " + (enable ? "ATIVADO" : "DESATIVADO"));
            // Chamada nativa para atualizar o Frame Buffer e o tema de renderização.
            // native_update_contrast_settings(enable);
        }
    }

    /**
     * @brief Habilita ou desabilita o leitor de tela (Screen Reader).
     * @param enable Se deve habilitar (true) ou desabilitar (false).
     */
    public static void setScreenReaderEnabled(boolean enable) {
        if (isScreenReaderEnabled != enable) {
            isScreenReaderEnabled = enable;
            ComandroDebug.i("ACCESSIBILITY", "Leitor de Tela: " + (enable ? "ATIVADO" : "DESATIVADO"));
            // Inicia ou para o thread do Screen Reader.
            // native_toggle_screen_reader(enable);
        }
    }
    
    /**
     * @brief Define a escala de tamanho da fonte (em porcentagem).
     * @param scale A nova escala (ex: 150 para 150%).
     */
    public static void setFontSizeScale(int scale) {
        if (scale > 50 && scale < 300) { // Limites de segurança
            fontSizeScale = scale;
            ComandroDebug.i("ACCESSIBILITY", "Escala de Fonte definida para " + scale + "%");
            // native_update_font_scale(scale);
        } else {
            ComandroDebug.w("ACCESSIBILITY", "Escala de Fonte invalida: " + scale);
        }
    }

    // --- Métodos de Status ---

    public static boolean isHighContrastModeEnabled() {
        return isHighContrastEnabled;
    }

    public static boolean isScreenReaderActive() {
        return isScreenReaderEnabled;
    }
}
