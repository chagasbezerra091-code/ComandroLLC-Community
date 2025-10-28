/*
 * comandro_low_latency_headers.h
 *
 * Headers de definicoes de baixa latencia e hooks do ComandroOS (referencia).
 * Estes seriam APIs internas usadas por outros modulos do kernel.
 */

#ifndef _LINUX_COMANDRO_LOW_LATENCY_HEADERS_H
#define _LINUX_COMANDRO_LOW_LATENCY_HEADERS_H

#include <linux/types.h>

// --- Prioridades de Latencia (Analogos as do ComandroOS Scheduler) ---
#define COMANDRO_PRIO_BACKGROUND        0
#define COMANDRO_PRIO_NORMAL            1
#define COMANDRO_PRIO_UI_INTERACTIVE    2
#define COMANDRO_PRIO_AUDIO_STREAM      3
#define COMANDRO_PRIO_CRITICAL_LATENCY  4 // Audio, Touch, V-Sync
#define COMANDRO_PRIO_CRITICAL_REALTIME 5 // Seguranca, Watchdog

// --- Tipos de Boost de CPU ---
typedef enum {
    COMANDRO_BOOST_NONE = 0,
    COMANDRO_BOOST_BINDER_CRITICAL = 1,
    COMANDRO_BOOST_TOUCH_EVENT = 2,
    COMANDRO_BOOST_DISPLAY_VSYNC = 3,
    COMANDRO_BOOST_SHORT_TASK = 4
} ComandroCpuBoostType;

// --- Hints para o CPU Governor ---
/**
 * @brief Envia um hint de boost de frequencia para o Comandro CPU Governor.
 * @param type O tipo de boost solicitado (e.g., por um evento de toque).
 * @param duration_ms Duracao do boost em milissegundos.
 */
extern void comandro_cpu_boost_hint(ComandroCpuBoostType type);

/**
 * @brief Seleciona a melhor CPU com base na prioridade da tarefa e tipo.
 * @param priority Prioridade da tarefa.
 * @param task_type Tipo de tarefa (e.g., BINDER, UI, AUDIO).
 * @return ID do CPU core alvo, ou -1 se nao houver preferencia.
 */
extern int comandro_select_best_cpu(int priority, int task_type);

/**
 * @brief Retorna se o modo de baixa latencia global esta ativo.
 */
extern bool comandro_is_low_latency_mode_active(void);


// --- APIs de Debugging de Latencia ---
/**
 * @brief Registra um ponto de latencia critico com timestamp no kernel.
 * @param event_id Identificador do evento.
 * @param timestamp_ns O timestamp em nanosegundos.
 */
extern void comandro_record_latency_event(u32 event_id, u64 timestamp_ns);

#endif // _LINUX_COMANDRO_LOW_LATENCY_HEADERS_H
