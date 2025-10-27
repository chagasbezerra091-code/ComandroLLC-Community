package comandro.kernel.core.packages.alert_ui;

import comandro.settings.debug.ComandroDebug;

/**
 * AlertUISDK: Interface de Baixa Latência para o Sistema de Alerta Crítico.
 *
 * Este SDK garante que mensagens de ERRO (Nível 0) e ALERTA (Nível 1) 
 * do kernel e dos módulos de saúde sejam exibidas ao usuário instantaneamente,
 * com prioridade máxima (PRIORITY_HIGH), fora do ciclo de renderização normal 
 * da interface de usuário (DE) para garantir latência mínima.
 */
public final class AlertUISDK {

    // Constante de Otimização: Latência máxima aceitável para exibição de um alerta.
    private static final int MAX_DISPLAY_LATENCY_MS = 10;
    
    // Níveis de Prioridade para a fila de renderização nativa.
    public static final int PRIORITY_CRITICAL = 0; // Exibir imediatamente, interromper DE.
    public static final int PRIORITY_HIGH = 1;     // Exibir imediatamente, não interromper DE.
    public static final int PRIORITY_NORMAL = 2;   // Exibir na próxima atualização do DE.

    // Método nativo que chama a função de renderização binária otimizada (C/C++).
    private static native void native_display_alert(
        int level, String title, String message, int priority, boolean requiresUserAck
    );

    // Método nativo para checagem do módulo
    private static native boolean isNativeModuleReady();

    // Construtor privado
    private AlertUISDK() {
        // Classe estática
    }
    
    /**
     * Verifica se o módulo nativo de UI de Alerta de Baixa Latência está carregado e pronto.
     * Crucial para a inicialização do kernel.
     * @return true se o binário nativo estiver acessível.
     */
    public static boolean isReady() {
        return isNativeModuleReady();
    }
    
    /**
     * Exibe um alerta de nível ERROR (Crítico - Vermelho).
     * @param title O título conciso do erro.
     * @param message A descrição detalhada do erro.
     */
    public static void displayCriticalError(String title, String message) {
        ComandroDebug.e("ALERT_UI", "Alerta Crítico Emitido: " + title);
        
        // Verifica se o SDK está pronto, se não, faz fallback para log crítico TTY/serial
        if (!isReady()) {
            System.err.println("FATAL UI ERROR: " + title + " - " + message);
            return;
        }
        
        native_display_alert(
            0, // Nível ERROR
            title, 
            message, 
            PRIORITY_CRITICAL, // Prioridade máxima: interrupção do sistema se necessário.
            true // Requer confirmação explícita do usuário
        );
    }
    
    /**
     * Exibe um alerta de nível WARN (Atenção - Laranja/Amarelo).
     * @param title O título do alerta.
     * @param message A descrição do alerta.
     */
    public static void displayWarning(String title, String message) {
        ComandroDebug.w("ALERT_UI", "Alerta de Warning Emitido: " + title);
        if (!isReady()) {
            ComandroDebug.w("ALERT_UI_FALLBACK", title + " - " + message);
            return;
        }

        native_display_alert(
            1, // Nível WARN
            title, 
            message, 
            PRIORITY_HIGH, // Alta Prioridade, mas não crítico o suficiente para interromper.
            false // Não requer confirmação explícita; desaparece após timeout
        );
    }

    /**
     * Exibe uma notificação de nível INFO (Informativo - Azul/Verde).
     * Usado para eventos como "WiFi Conectado" ou "Atualização Concluída".
     * @param title O título da notificação.
     * @param message A mensagem da notificação.
     */
    public static void displayInfoNotification(String title, String message) {
        ComandroDebug.i("ALERT_UI", "Notificação Emitida: " + title);
        if (!isReady()) {
            ComandroDebug.i("ALERT_UI_FALLBACK", title + " - " + message);
            return;
        }
        
        native_display_alert(
            2, // Nível INFO
            title, 
            message, 
            PRIORITY_NORMAL, // Prioridade normal: exibido no ciclo regular do DE.
            false 
        );
    }
    
    /**
     * Retorna a latência máxima de exibição de alerta que o sistema está configurado para tolerar.
     * @return Latência máxima em milissegundos.
     */
    public static int getMaxDisplayLatencyMs() {
        return MAX_DISPLAY_LATENCY_MS;
    }
}
