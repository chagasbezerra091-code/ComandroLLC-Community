package comandro.kernel.core.sys.system.errors.lithium_detected;

import comandro.settings.debug.ComandroDebug;
import comandro.kernel.core.packages.scheduler.TaskScheduler;
import comandro.kernel.core.packages.alert_ui.AlertUISDK;

/**
 * LithiumErrorManager: Gerenciador de Erros Criticos de Bateria/Termico (Lithium).
 *
 * Este módulo e ativado pelo driver nativo (C/C++) do BatteryManager quando
 * uma condição critica e detectada, como: sobretemperatura da celula, 
 * sobrecarga/descarga extrema, ou falha de leitura do BMS (Battery Management System).
 *
 * O objetivo e executar politicas de desligamento imediato ou hibernacao forçada.
 */
public final class LithiumErrorManager {

    // Codigos de erro critico (compartilhados com o driver nativo)
    public static final int CODE_OVER_TEMPERATURE = 101;
    public static final int CODE_EXTREME_DISCHARGE = 102;
    public static final int CODE_BMS_FAILURE = 103;

    private static boolean criticalShutdownInitiated = false;

    // Metodos nativos para interagirem com o hardware de energia
    private static native void native_emergency_shutdown(int delaySeconds);
    private static native void native_throttle_cpu_gpu(int percentage);
    
    // Metodo para obter o estado atual de temperatura do kernel
    private static native int native_get_current_battery_temp_celsius();

    private LithiumErrorManager() {
        // Classe estatica
    }

    /**
     * @brief Acao primaria para tratar a deteccao de um erro critico de Lithium.
     * Este metodo deve ser chamado imediatamente apos a deteccao nativa.
     * @param errorCode O codigo de erro critico detectado.
     * @param message Mensagem detalhada do driver nativo.
     */
    public static void handleCriticalError(int errorCode, String message) {
        if (criticalShutdownInitiated) {
            ComandroDebug.w("LITHIUM_ERROR", "Erro recebido, mas desligamento ja esta em curso.");
            return;
        }

        ComandroDebug.e("LITHIUM_ERROR", "ERRO CRITICO DETECTADO: Codigo " + errorCode + " - " + message);
        
        switch (errorCode) {
            case CODE_OVER_TEMPERATURE:
                handleOverTemperature(message);
                break;
            case CODE_EXTREME_DISCHARGE:
                handleExtremeDischarge(message);
                break;
            case CODE_BMS_FAILURE:
                handleBMSFailure(message);
                break;
            default:
                ComandroDebug.e("LITHIUM_ERROR", "Codigo de erro desconhecido. Iniciando desligamento padrao.");
                initiateSafetyShutdown(5); // Desligamento de 5 segundos padrao
                break;
        }
    }

    /**
     * @brief Politica para sobretemperatura.
     */
    private static void handleOverTemperature(String message) {
        int currentTemp = native_get_current_battery_temp_celsius();
        
        // 1. Loga e exibe alerta critico
        AlertUISDK.displayCriticalError("ALERTA TERMICO CRITICO", "Bateria (" + currentTemp + "°C) atingiu temperatura maxima! Desligamento em 1 segundo.");
        
        // 2. Tenta desacelerar imediatamente CPU/GPU (Throttle)
        native_throttle_cpu_gpu(10); // Reduz para 10% do desempenho
        ComandroDebug.i("LITHIUM_ERROR", "Throttle forcado para tentar resfriar.");

        // 3. Inicia o desligamento imediato
        initiateSafetyShutdown(1); // Desliga em 1 segundo
    }

    /**
     * @brief Politica para descarga extrema.
     */
    private static void handleExtremeDischarge(String message) {
        // 1. Alerta o usuario
        AlertUISDK.displayWarning("ALERTA DE ENERGIA", "Descarga extrema! Sistema ira hibernar em 3 segundos para salvar dados.");
        
        // 2. Inicia uma tarefa de alta prioridade para salvar o estado
        TaskScheduler.scheduleTask(() -> {
            ComandroDebug.w("LITHIUM_ERROR", "Executando rotina de hibernacao forcada.");
            // native_save_system_state_to_flash();
            native_emergency_shutdown(0); // Desligamento imediato apos salvar
        }, TaskScheduler.PRIORITY_REAL_TIME, true);
    }
    
    /**
     * @brief Politica para falha do BMS (sistema de gerenciamento de bateria).
     * Falha do BMS significa que o sistema nao pode mais confiar nas leituras de segurança.
     */
    private static void handleBMSFailure(String message) {
        AlertUISDK.displayCriticalError("FALHA DE HARDWARE", "Sistema de Gerenciamento de Bateria (BMS) inoperante! DESLIGAMENTO IMEDIATO.");
        ComandroDebug.a("LITHIUM_ERROR", "FALHA DE BMS: Perigo de seguranca. Desligamento forcado.");
        initiateSafetyShutdown(0); // Desligamento imediato
    }

    /**
     * @brief Inicia o processo de desligamento de emergencia no kernel nativo.
     * @param delaySeconds O atraso em segundos antes do desligamento de hardware.
     */
    private static void initiateSafetyShutdown(int delaySeconds) {
        criticalShutdownInitiated = true;
        
        // Desliga a CPU/GPU em 100% para evitar qualquer atividade
        native_throttle_cpu_gpu(0); 
        
        ComandroDebug.a("LITHIUM_ERROR", "Desligamento de seguranca iniciado. Delay: " + delaySeconds + "s.");
        
        // Chamada nativa final que leva ao corte de energia
        native_emergency_shutdown(delaySeconds);
    }
}
