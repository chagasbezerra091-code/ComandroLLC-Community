package comandro.settings.debug;

/**
 * Módulo de Debug e Log de Baixa Latência para o ComandroOS.
 *
 * Esta classe é projetada para ser extremamente leve, evitando operações
 * de I/O bloqueantes e alocação de memória desnecessária (zero-allocation)
 * no caminho crítico de execução.
 *
 * Níveis de Log (Prioridade Alta = menor valor):
 * 0: ERROR (Falhas Críticas que exigem intervenção imediata)
 * 1: WARN  (Condições potencialmente instáveis que podem levar a um erro)
 * 2: INFO  (Eventos operacionais normais e marcos importantes - ex: boot time)
 * 3: DEBUG (Informações detalhadas para engenharia e rastreamento de bugs)
 * 4: VERBOSE (Logs excessivamente detalhados - usar com extrema cautela)
 */
public final class ComandroDebug {

    // Nível de log mínimo que será registrado. 
    // Configurado estaticamente na compilação do kernel. 
    private static final int CURRENT_LOG_LEVEL = 2; // Default: INFO

    // Módulo de Kernel para onde os logs são direcionados (ex: TTY ou buffer de memória circular)
    private static native void native_log_print(int level, String tag, String message);

    // Construtor privado para evitar instanciamento. 
    private ComandroDebug() {
        // Esta é uma classe estática de utilidade
    }

    // --- Métodos de Logging Otimizados ---

    /**
     * Registra uma mensagem de Erro (Nível 0: CRITICAL).
     * @param tag  A origem da mensagem (ex: "KernelIO", "MemoryMgr").
     * @param msg  A mensagem de erro.
     */
    public static void e(String tag, String msg) {
        if (0 <= CURRENT_LOG_LEVEL) {
            native_log_print(0, tag, msg);
        }
    }

    /**
     * Registra uma mensagem de Alerta (Nível 1: WARN).
     * @param tag  A origem da mensagem.
     * @param msg  A mensagem de alerta.
     */
    public static void w(String tag, String msg) {
        if (1 <= CURRENT_LOG_LEVEL) {
            native_log_print(1, tag, msg);
        }
    }
    
    /**
     * Registra uma mensagem Informativa (Nível 2: INFO).
     * Usado para rastrear o tempo de inicialização, mudanças de estado, etc.
     * @param tag  A origem da mensagem.
     * @param msg  A mensagem informativa.
     */
    public static void i(String tag, String msg) {
        if (2 <= CURRENT_LOG_LEVEL) {
            native_log_print(2, tag, msg);
        }
    }

    /**
     * Registra uma mensagem detalhada de Debug (Nível 3: DEBUG).
     * @param tag  A origem da mensagem.
     * @param msg  A mensagem de debug.
     */
    public static void d(String tag, String msg) {
        if (3 <= CURRENT_LOG_LEVEL) {
            native_log_print(3, tag, msg);
        }
    }

    // --- Funcionalidades Avançadas de Debug ---

    /**
     * Verifica se o debug detalhado está ativo, permitindo que código
     * pesado de debug seja ignorado antes da chamada de log.
     * Ex: if (ComandroDebug.isDebugEnabled()) { String hugeStr = ... }
     * @return true se o nível de log for 3 ou superior.
     */
    public static boolean isDebugEnabled() {
        return 3 <= CURRENT_LOG_LEVEL;
    }

    /**
     * Imprime o tempo decorrido em milissegundos desde o último marco de tempo (timestamp).
     * Essencial para medir a latência.
     * @param tag O módulo que está sendo medido.
     * @param startNanos O tempo inicial em nanosegundos (obtido por System.nanoTime()).
     */
    public static void measureTime(String tag, long startNanos) {
        if (2 <= CURRENT_LOG_LEVEL) { // Registra a medição de tempo no nível INFO
            long elapsedNanos = System.nanoTime() - startNanos;
            // Converte para milissegundos para legibilidade, mas mantém a precisão na mensagem.
            double elapsedMillis = elapsedNanos / 1_000_000.0;
            native_log_print(2, tag + "_TIME", 
                String.format("TIME: %.3f ms (%d ns)", elapsedMillis, elapsedNanos));
        }
    }
}
