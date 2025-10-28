/*
 * mobile_power_governor.c
 *
 * Governor de frequencia de CPU customizado para ARM64 Mobile.
 * Este governor prioriza a responsividade e latencia ultrabaixa para UI/Audio.
 *
 * Em ComandroOS, a logica de 'PowerGovernor' esta em Rust no kernel-core/sections/CPU_e_GPU/.
 */

#include <linux/cpufreq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/comandro_low_latency_headers.h> // Header do Comandro

// --- Constantes do Governor (Exemplo) ---
#define COMANDRO_MOBILE_UP_THRESHOLD    80      // Sobe a frequencia se a carga passar de 80%
#define COMANDRO_MOBILE_DOWN_THRESHOLD  30      // Desce a frequencia se a carga cair abaixo de 30%
#define COMANDRO_MOBILE_IDLE_FREQ_MHZ   500     // Frequencia minima em idle para latencia
#define COMANDRO_MOBILE_CRITICAL_FREQ_MHZ 1500  // Frequencia minima para tarefas criticas

static DEFINE_MUTEX(comandro_mobile_lock);

static void comandro_mobile_governor_freq_set(struct cpufreq_policy *policy, unsigned int freq)
{
    // A logica real aqui envolveria escrever em registradores PMIC/CPUFREQ.
    // Para o exemplo, apenas logamos.
    pr_debug("Comandro Mobile Gov: Setando CPU%d para %u MHz\n", policy->cpu, freq);
    cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);
}

static int comandro_mobile_governor_work(struct cpufreq_policy *policy)
{
    unsigned int load_percent;
    unsigned int new_freq;

    cpufreq_acct_update_integral(policy);
    load_percent = cpufreq_get_load_percent(policy);

    mutex_lock(&comandro_mobile_lock);

    // Logica de decisao
    if (load_percent > COMANDRO_MOBILE_UP_THRESHOLD) {
        new_freq = cpufreq_driver_get_freq_table_next(policy, policy->cur);
        pr_debug("Comandro Mobile Gov: Carga %u%%. Subindo para %u MHz.\n", load_percent, new_freq);
    } else if (load_percent < COMANDRO_MOBILE_DOWN_THRESHOLD) {
        new_freq = cpufreq_driver_get_freq_table_prev(policy, policy->cur);
        // Garante que a frequencia nao caia abaixo do minimo para tarefas criticas
        new_freq = max(new_freq, COMANDRO_MOBILE_CRITICAL_FREQ_MHZ);
        pr_debug("Comandro Mobile Gov: Carga %u%%. Descendo para %u MHz (min %u).\n", load_percent, new_freq, COMANDRO_MOBILE_CRITICAL_FREQ_MHZ);
    } else {
        new_freq = policy->cur; // Mantem a frequencia atual
    }
    
    // --- ComandroOS: Hint de Latencia (integrado ao governor) ---
    if (comandro_is_low_latency_mode_active()) {
        new_freq = max(new_freq, COMANDRO_MOBILE_CRITICAL_FREQ_MHZ);
    }


    if (new_freq != policy->cur) {
        comandro_mobile_governor_freq_set(policy, new_freq);
    }

    mutex_unlock(&comandro_mobile_lock);
    return 0;
}

static struct cpufreq_governor comandro_mobile_governor = {
    .name           = "comandro_mobile",
    .flags          = CPUFREQ_GOV_SET_LATE,
    .max_transition_latency = 1 * NSEC_PER_MSEC, // Transicao em 1ms
    .base_rate_us   = 2000,                      // Roda a cada 2ms
    .start_policy   = comandro_mobile_governor_work,
    .stop_policy    = NULL,
    .get_update_rate = cpufreq_governor_get_update_rate,
};

static int __init comandro_mobile_governor_init(void)
{
    pr_info("Comandro Mobile Power Governor carregado.\n");
    return cpufreq_register_governor(&comandro_mobile_governor);
}

static void __exit comandro_mobile_governor_exit(void)
{
    cpufreq_unregister_governor(&comandro_mobile_governor);
    pr_info("Comandro Mobile Power Governor descarregado.\n");
}

module_init(comandro_mobile_governor_init);
module_exit(comandro_mobile_governor_exit);
