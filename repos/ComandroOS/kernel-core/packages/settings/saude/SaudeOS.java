package comandro.settings.saude;

import comandro.settings.debug.ComandroDebug;

/**
 * Módulo de Monitoramento da Saúde e Desempenho do ComandroOS (SaudeOS).
 *
 * Esta classe fornece métodos para medir e relatar o desempenho do kernel
 * em tempo real, sendo crucial para a promessa de Zero Latência.
 */
public final class SaudeOS {

    // Constantes para definir os limites de alerta (KPIs de Otimização)
    // Definiremos o foco no próximo passo. Por enquanto, valores simbólicos:
    
    // Alerta se a latência de I/O for maior que 5 milissegundos (muito alto)
    private static final int MAX_IO_LATENCY_MS = 5; 
    
    // Alerta se a carga média (load average) for maior que 2.0
    private static final double MAX_LOAD_AVERAGE = 2.0; 

    // Métodos nativos (implementados no código C/C++ do kernel) para acessar dados brutos
    private static native long getNativeBootTimeMs();
    private static native double getNativeSystemLoadAverage();
    private static native long getNativeIoLatencyMs();
    private static native long getNativeAvailableMemoryKb();
    
    // Construtor privado
    private SaudeOS() {
        // Classe estática
    }

    /**
     * Retorna o tempo total de inicialização do sistema em milissegundos.
     * Esta é uma métrica chave para demonstrar a otimização do ComandroOS.
     * @return O tempo de boot em ms.
     */
    public static long getTempoBootMs() {
        return getNativeBootTimeMs();
    }

    /**
     * Retorna o nível de estresse do processador (load average) nos últimos 1 minuto.
     * @return O valor de carga média do sistema (Load Average).
     */
    public static double getCargaMediaSistema() {
        return getNativeSystemLoadAverage();
    }

    /**
     * Retorna a latência média de Input/Output (I/O) em milissegundos.
     * **Métrica CRÍTICA para a filosofia Zero Latência.**
     * @return Latência de I/O em ms.
     */
    public static long getLatenciaIoMs() {
        return getNativeIoLatencyMs();
    }

    /**
     * Retorna a quantidade de memória RAM disponível em Kilobytes (KB).
     * @return Memória livre em KB.
     */
    public static long getMemoriaLivreKb() {
        return getNativeAvailableMemoryKb();
    }
    
    /**
     * Gera um relatório de saúde do sistema e registra alertas se as métricas
     * excederem os limites definidos.
     * @return Uma string de relatório de status.
     */
    public static String gerarRelatorioSaude() {
        long bootTime = getTempoBootMs();
        double load = getCargaMediaSistema();
        long ioLatency = getLatenciaIoMs();
        long freeMem = getMemoriaLivreKb();

        StringBuilder relatorio = new StringBuilder();
        relatorio.append("--- RELATÓRIO DE SAÚDE DO COMANDROOS ---\n");
        relatorio.append("Boot Time: ").append(bootTime).append(" ms\n");
        relatorio.append("Load Avg (1min): ").append(String.format("%.2f", load)).append("\n");
        relatorio.append("I/O Latency: ").append(ioLatency).append(" ms\n");
        relatorio.append("Memória Livre: ").append(freeMem / 1024).append(" MB\n");
        relatorio.append("---------------------------------------\n");
        
        // Verificação e Log de Alertas
        if (ioLatency > MAX_IO_LATENCY_MS) {
            ComandroDebug.w("SAUDE_IO", "ALERTA: Latência de I/O (L=" + ioLatency + 
                             "ms) excedeu o limite de " + MAX_IO_LATENCY_MS + "ms.");
        }
        if (load > MAX_LOAD_AVERAGE) {
            ComandroDebug.w("SAUDE_CPU", "ALERTA: Carga Média do Sistema (L=" + load + 
                             ") está alta. Investigar.");
        }

        return relatorio.toString();
    }
}
