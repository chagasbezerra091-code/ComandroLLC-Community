package comandro.kernel.core.packages.memory;

import comandro.settings.debug.ComandroDebug;
import comandro.settings.saude.SaudeOS;
import comandro.kernel.core.packages.alert_ui.AlertUISDK;

/**
 * OOMKiller (Out-Of-Memory Killer) - Gerenciador de Estabilidade de Memória.
 *
 * Este módulo é ativado quando o sistema está com pouca memória disponível,
 * de acordo com o KPI de memória livre do SaudeOS. Sua função é identificar
 * e encerrar o processo com maior consumo e menor prioridade para restaurar
 * a estabilidade e a baixa latência do ComandroOS.
 */
public final class OOMKiller {

    // Limite crítico de memória livre em Kilobytes (KB).
    // Se a memória livre cair abaixo de 256MB (262144 KB), o OOMKiller entra em ação.
    private static final long CRITICAL_MEMORY_THRESHOLD_KB = 262144L; 

    // O critério para determinar a prioridade de um processo. 
    // Em um kernel real, seria um índice dinâmico (OOM_SCORE).
    private static final int PROCESS_PRIORITY_LOWEST = 0; 
    
    // Método nativo (C/C++) para obter o PID do processo a ser encerrado e executá-lo.
    private static native int native_find_and_kill_process(long minMemoryToFreeKb);
    private static native long native_get_process_memory_usage(int pid);
    private static native String native_get_process_name(int pid);

    private OOMKiller() {
        // Classe estática
    }

    /**
     * @brief Verifica o status da memória e, se for crítico, invoca o OOM Killer.
     * Deve ser chamado periodicamente pelo Thread Scheduler do kernel.
     */
    public static void checkAndExecute() {
        long freeMem = SaudeOS.getMemoriaLivreKb();
        
        if (freeMem < CRITICAL_MEMORY_THRESHOLD_KB) {
            long requiredToFree = CRITICAL_MEMORY_THRESHOLD_KB - freeMem;
            
            ComandroDebug.w("OOM_KILLER", 
                "Memória Crítica: " + (freeMem / 1024) + "MB Livre. Iniciando OOM Killer...");
                
            int killedPid = native_find_and_kill_process(requiredToFree);
            
            if (killedPid > 0) {
                String processName = native_get_process_name(killedPid);
                long freedMem = native_get_process_memory_usage(killedPid);
                
                String message = String.format(
                    "Encerrado PID %d (%s). Liberado: %d MB. Memória restaurada.", 
                    killedPid, processName, (freedMem / 1024)
                );
                
                ComandroDebug.e("OOM_KILLER", message);
                // Exibe alerta de estabilidade na interface
                AlertUISDK.displayWarning("Estabilidade Restaurada", message);
            } else {
                ComandroDebug.e("OOM_KILLER", "Falha ao liberar memória. Sistema em risco de Kernel Panic.");
                AlertUISDK.displayCriticalError("KERNEL PANIC IMINENTE", "Memória Esgotada! Falha ao matar processos.");
            }
        } else {
            // ComandroDebug.d("OOM_KILLER", "Memória OK. Livre: " + (freeMem / 1024) + "MB.");
        }
    }
}
