package comandro.kernel.core.packages.scheduler;

import comandro.settings.debug.ComandroDebug;
import comandro.settings.saude.SaudeOS;

/**
 * TaskScheduler: Agendador de Tarefas e Threading Otimizado.
 * * Implementa um agendamento de prioridade preemptivo focado em baixa latência (KPI <1ms I/O).
 * Threads de I/O Bound recebem prioridade máxima sobre threads de CPU Bound.
 */
public final class TaskScheduler {

    // Níveis de Prioridade do ComandroOS (Alto número = Alta Prioridade)
    public static final int PRIORITY_REAL_TIME = 9; // Kernel I/O, UI Crítico (p/ KPI)
    public static final int PRIORITY_HIGH_IO = 7;   // Threads de Rede/Disco
    public static final int PRIORITY_NORMAL = 5;    // Aplicações de usuário padrão
    public static final int PRIORITY_LOW_BACKGROUND = 3; // Coleta de lixo, OOM Check
    public static final int PRIORITY_IDLE = 1;      // Tarefas ociosas

    // Estado interno para rastrear a carga do agendador
    private static int activeThreadCount = 0;

    // Métodos nativos (C/C++ do kernel) para manipular o agendador
    private static native long native_create_thread(Runnable task, int priority, boolean isIoBound);
    private static native void native_set_priority(long threadId, int newPriority);
    private static native void native_yield_cpu(long threadId);

    private TaskScheduler() {
        // Classe estática
    }

    /**
     * @brief Agenda uma nova tarefa no kernel.
     * @param task A tarefa a ser executada.
     * @param priority A prioridade de 1 (ocioso) a 9 (tempo real).
     * @param isIoBound Indica se a tarefa é limitada por I/O (maior prioridade) ou CPU.
     * @return O ID nativo da thread criada (handle).
     */
    public static long scheduleTask(Runnable task, int priority, boolean isIoBound) {
        if (priority > PRIORITY_REAL_TIME || priority < PRIORITY_IDLE) {
            ComandroDebug.e("SCHEDULER", "Prioridade invalida: " + priority + ". Usando NORMAL.");
            priority = PRIORITY_NORMAL;
        }

        long threadId = native_create_thread(task, priority, isIoBound);
        
        if (threadId > 0) {
            activeThreadCount++;
            ComandroDebug.d("SCHEDULER", 
                String.format("Nova thread agendada. ID: %d, Prioridade: %d, I/O Bound: %s", 
                              threadId, priority, isIoBound ? "SIM" : "NAO"));
        } else {
            ComandroDebug.e("SCHEDULER", "Falha ao criar thread nativa.");
        }
        
        return threadId;
    }

    /**
     * @brief Tenta reduzir a prioridade de threads que consomem muita CPU.
     * Chamado pelo monitor de saúde (SaudeOS) se a carga média estiver alta.
     */
    public static void rebalancePriorities() {
        double load = SaudeOS.getCargaMediaSistema();
        
        if (load > SaudeOS.MAX_LOAD_AVERAGE) {
            ComandroDebug.w("SCHEDULER", "ALERTA: Carga alta (Load=" + load + "). Reduzindo prioridade de threads de CPU Bound.");
            // Implementação nativa para encontrar e reduzir processos PRIORITY_NORMAL para PRIORITY_LOW_BACKGROUND.
            // native_dynamic_priority_boost_and_drop(load);
        }
    }
    
    /**
     * @brief Retorna o número de threads ativas gerenciadas pelo TaskScheduler.
     */
    public static int getActiveThreadCount() {
        return activeThreadCount;
    }
}
