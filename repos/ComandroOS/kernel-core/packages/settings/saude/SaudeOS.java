package comandro.settings.saude;

import comandro.settings.debug.ComandroDebug;
import comandro.kernel.core.packages.alert_ui.AlertUISDK;

/**
 * Módulo de Monitoramento da Saúde e Desempenho do ComandroOS (SaudeOS).
 *
 * Esta classe é o centro de rastreamento dos KPIs (Key Performance Indicators)
 * do ComandroOS, garantindo que a promessa de Zero Latência e Alta Eficiência
 * seja mantida.
 */
public final class SaudeOS {

    // --- KPIs de Otimização da Comandro LLC ---
    
    /** Alerta se o Tempo de Boot exceder 15 segundos (15000 ms). */
    private static final long MAX_BOOT_TIME_MS = 15000L; // META: 15s
    
    /** Alerta se a Latência de I/O exceder 1 milissegundo (1 ms). */
    private static final long MAX_IO_LATENCY_MS = 1L; // META: 1ms (Zero Latência Visual)
    
    /** Alerta se a carga média (load average) for maior que 2.0 (KPI de Estabilidade) */
    private static final double MAX_LOAD_AVERAGE = 2.0; 
    
    // --- Verificação de Hardware de Tensão (Voltage) ---
    /** Tensão Mínima de Operação em Volts (15V). */
    private static final int MIN_VOLTAGE_V = 15;
    /** Tensão Máxima de Operação em Volts (19V). */
    private static final int MAX_VOLTAGE_V = 19; 

    // Métodos nativos (implementados no código C/C++ do kernel) para acessar dados brutos
    private static native long getNativeBootTimeMs();
    private static native double getNativeSystemLoadAverage();
    private static native long getNativeIoLatencyMs();
    private static native long getNativeAvailableMemoryKb();
    private static native int getNativeCurrentVoltageV();
    
    // Construtor privado
    private SaudeOS() {
        // Classe estática
    }

    /** Retorna o tempo total de inicialização do sistema em milissegundos. */
    public static long getTempoBootMs() {
        return getNativeBootTimeMs();
    }

    /** Retorna o nível de estresse do processador (load average) nos últimos 1 minuto. */
    public static double getCargaMediaSistema() {
        return getNativeSystemLoadAverage();
    }

    /** Retorna a latência média de Input/Output (I/O) em milissegundos. */
    public static long getLatenciaIoMs() {
        return getNativeIoLatencyMs();
    }

    /** Retorna a quantidade de memória RAM disponível em Kilobytes (KB). */
    public static long getMemoriaLivreKb() {
        return getNativeAvailableMemoryKb();
    }
    
    /** Retorna a tensão de entrada atual do sistema em Volts. */
    public static int getCurrentVoltageV() {
        return getNativeCurrentVoltageV();
    }
    
    /**
     * Gera um relatório de saúde do sistema e registra alertas se as métricas
     * excederem os limites definidos, usando o AlertUISDK para alertas críticos.
     * @return Uma string de relatório de status para log ou interface.
     */
    public static String gerarRelatorioSaude() {
        long bootTime = getTempoBootMs();
        double load = getCargaMediaSistema();
        long ioLatency = getLatenciaIoMs();
        int voltage = getCurrentVoltageV();

        StringBuilder relatorio = new StringBuilder();
        relatorio.append("--- RELATÓRIO DE SAÚDE DO COMANDROOS ---\n");
        relatorio.append("KPI Boot Time:   ").append(bootTime).append(" ms (Meta: <").append(MAX_BOOT_TIME_MS/1000).append("s)\n");
        relatorio.append("KPI I/O Latency: ").append(ioLatency).append(" ms (Meta: <").append(MAX_IO_LATENCY_MS).append("ms)\n");
        relatorio.append("Carga Média (1min): ").append(String.format("%.2f", load)).append("\n");
        relatorio.append("Tensão de Entrada: ").append(voltage).append(" V (Range: ").append(MIN_VOLTAGE_V).append("V-").append(MAX_VOLTAGE_V).append("V)\n");
        relatorio.append("Memória Livre:   ").append(getMemoriaLivreKb() / 1024).append(" MB\n");
        relatorio.append("---------------------------------------\n");
        
        // --- Verificação e Log de Alertas ---
        
        // Alerta de Boot Time (Warning, mas grave)
        if (bootTime > MAX_BOOT_TIME_MS) {
             AlertUISDK.displayWarning("LENTIDÃO NO BOOT", 
                "O Tempo de Inicialização (" + bootTime + "ms) excedeu o KPI de 15s. Investigar drivers ou serviços.");
        }
        
        // Alerta de Latência de I/O (CRÍTICO: Viola o princípio Zero Latência)
        if (ioLatency > MAX_IO_LATENCY_MS) {
            AlertUISDK.displayCriticalError("FALHA DE LATÊNCIA CRÍTICA", 
                "Latência de I/O (" + ioLatency + "ms) está acima da meta de 1ms. Performance comprometida.");
        }
        
        // Alerta de Tensão (CRÍTICO: Segurança do Hardware)
        if (voltage < MIN_VOLTAGE_V || voltage > MAX_VOLTAGE_V) {
            AlertUISDK.displayCriticalError("FALHA DE TENSÃO/HARDWARE", 
                "Tensão de entrada (" + voltage + "V) fora do range seguro. RISCO DE QUEIMA.");
        }
        
        // Alerta de Carga Média (Log interno de Debug)
        if (load > MAX_LOAD_AVERAGE) {
            ComandroDebug.w("SAUDE_CPU", "Carga Média do Sistema (L=" + load + ") está alta. Investigar loop de thread.");
        }

        return relatorio.toString();
    }
}
