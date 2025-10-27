package comandro.settings.os_info;

/**
 * Classe Estática para Armazenar e Acessar Informações de Versão e Branding do ComandroOS.
 *
 * Estas informações são lidas por módulos do sistema, ferramentas de debug,
 * e pela interface do usuário (Settings).
 */
public final class OSInfo {

    // --- Informações de Versão do Sistema ---
    
    /** A versão principal e de patch do ComandroOS (Major.Minor.Patch). */
    public static final String VERSION_NUMBER = "1.0.0";
    
    /** O codinome (codename) da versão atual. */
    public static final String VERSION_CODENAME = "ice-scream"; // Codinome interno

    /** A string completa da versão, usada em displays de inicialização e logs. */
    public static final String FULL_VERSION = 
        "ComandroOS " + VERSION_NUMBER + " (" + VERSION_CODENAME + ")";

    // --- Informações de Liderança (Branding) ---
    
    /** O CEO e Arquiteto-Chefe do ComandroOS. */
    public static final String CEO_ARQUITETO_CHEFE = "Enzo Gabryel Bezerra dos Santos";
    
    /** O Co-CEO ou Co-Fundador (se aplicável). */
    public static final String CO_CEO = "Chagas Bezerra";
    
    /** Nome da Organização Proprietária do Kernel. */
    public static final String ORGANIZATION_NAME = "Comandro LLC";

    /**
     * Construtor privado para garantir que a classe não seja instanciada.
     * É uma classe estática de utilidade.
     */
    private OSInfo() {
        // Utility Class
    }
    
    /**
     * Retorna a string de copyright formatada para ser usada em arquivos de licença.
     * @return String de Copyright.
     */
    public static String getCopyrightNotice() {
        return "Copyright (C) 2025 " + ORGANIZATION_NAME + " - Todos os direitos reservados. " +
               "Licenciado sob a GNU General Public License v3.0.";
    }
}
