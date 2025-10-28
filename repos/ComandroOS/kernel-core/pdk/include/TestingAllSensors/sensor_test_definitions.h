#ifndef COMANDRO_PDK_SENSORS_TEST_DEFINITIONS_H
#define COMANDRO_PDK_SENSORS_TEST_DEFINITIONS_H

#include <stdint.h>

// =====================================================================
// sensor_test_definitions.h - Definicoes de Interface de Teste de Sensores
// Usado pela camada PDK (Java/Kotlin) para interagir com o kernel.
// =====================================================================

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------------
// 1. Constantes de Limite e Configuracao
// -------------------------------------------------------------------

// Taxa de amostragem padrao usada para testes de latencia (1000 Hz)
#define SENSOR_TEST_RATE_HZ_DEFAULT 1000 

// Limite maximo de latencia de transporte (Kernel -> App) em nanosegundos (1 ms)
#define MAX_TRANSPORT_LATENCY_NS    1000000 

// Limite maximo de Jitter (variacao de intervalo) em nanosegundos (0.5 ms)
#define MAX_JITTER_NS               500000 

// Tipo de log de teste (Usado pela FFI para classificar o output)
typedef enum {
    TEST_LOG_INFO = 0,
    TEST_LOG_WARNING = 1,
    TEST_LOG_ALERT = 2,       // Latencia/Jitter excedeu o limite
    TEST_LOG_CRITICAL = 3     // Falha no driver
} TestLogLevel;


// -------------------------------------------------------------------
// 2. Estrutura de Dados de Metricas
// -------------------------------------------------------------------

/**
 * @brief Estrutura de metricas de latencia e jitter para um unico sensor.
 * Esta estrutura é populada pelo kernel (C++) e lida pelo PDK (Java).
 */
typedef struct {
    int32_t sensor_type;            // Tipo do sensor (e.g., TYPE_GYROSCOPE)
    uint64_t last_latency_ns;       // Latencia de transporte da ultima amostra (Kernel->App)
    uint64_t avg_jitter_ns;         // Media movel do jitter (inconsistencia de tempo)
    uint64_t kernel_buffer_depth;   // Profundidade da fila de eventos do kernel para este sensor
    float power_consumption_ma;     // Consumo de energia reportado pelo driver em mA
} SensorTestMetrics;

// -------------------------------------------------------------------
// 3. Funcoes de Interface (FFI - Foreign Function Interface)
// -------------------------------------------------------------------

/**
 * @brief Funcao FFI (Fire-and-forget) para o kernel iniciar o monitoramento de teste.
 * O kernel deve criar uma thread de alta prioridade para coletar os dados.
 * @param sensor_type O tipo de sensor a ser testado (0 para todos).
 * @param rate_hz A frequencia de amostragem desejada.
 * @return 0 em sucesso, -1 se o kernel falhar ao alocar recursos de teste.
 */
int32_t pdk_start_sensor_latency_test(int32_t sensor_type, int32_t rate_hz);

/**
 * @brief Funcao FFI para o PDK obter o estado atual das metricas de um sensor.
 * Deve ser uma leitura atomica e de baixa latencia (sem bloqueio).
 * @param sensor_type O tipo de sensor para o qual as metricas sao solicitadas.
 * @param metrics Ponteiro para a estrutura a ser preenchida.
 * @return 0 em sucesso, -1 se o sensor nao estiver sendo monitorado.
 */
int32_t pdk_get_current_sensor_metrics(int32_t sensor_type, SensorTestMetrics* metrics);

/**
 * @brief Funcao FFI para o kernel enviar logs de teste de volta para o PDK.
 * @param level O nivel de severidade do log.
 * @param message_ptr Ponteiro para a string C-style da mensagem.
 */
void pdk_log_test_event(int32_t level, const char* message_ptr);

#ifdef __cplusplus
}
#endif

#endif // COMANDRO_PDK_SENSORS_TEST_DEFINITIONS_H
