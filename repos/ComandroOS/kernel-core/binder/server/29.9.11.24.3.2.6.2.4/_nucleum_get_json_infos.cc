#include <comandro/kernel/binder/server/nucleum.h>
#include <comandro/kernel/binder/server/atomic_info.h> // Para dados atomicos da CPU
#include <comandro/kernel/cpu_topology.h>
#include <comandro/kernel/memory_manager.h>
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/util/string_buffer.h>

// =====================================================================
// _nucleum_get_json_infos.cc - Subsistema Nucleum de Exportacao de Dados
// Fornece informacoes de diagnostico do kernel em formato JSON.
// =====================================================================

namespace comandro {
namespace kernel {
namespace binder {
namespace nucleum {

// Tamanho maximo do buffer JSON de diagnostico.
static constexpr size_t JSON_BUFFER_SIZE = 4096;

/**
 * @brief Serializa o estado atual do kernel em um buffer JSON.
 * @return StringBuffer contendo a string JSON formatada.
 */
StringBuffer get_diagnostic_json() {
    // Usa StringBuffer do kernel para construcao eficiente de strings.
    StringBuffer json_output(JSON_BUFFER_SIZE);
    
    // =================================================================
    // 1. DADOS BASE (Memoria, Uptime)
    // =================================================================
    const auto& mem_stats = memory::get_memory_stats();
    
    json_output.append("{\n");
    json_output.append("  \"sistema\": {\n");
    json_output.append("    \"uptime_ms\": %lu,\n", time::get_uptime_ms());
    json_output.append("    \"versao_kernel\": \"ComandroOS-");
    json_output.append(VERSION_STRING); // Assumindo uma macro VERSION_STRING
    json_output.append("\"\n");
    json_output.append("  },\n");

    json_output.append("  \"memoria\": {\n");
    json_output.append("    \"total_kb\": %lu,\n", mem_stats.total_memory_kb);
    json_output.append("    \"livre_kb\": %lu,\n", mem_stats.free_memory_kb);
    json_output.append("    \"binder_alloc_kb\": %lu\n", mem_stats.binder_memory_usage_kb);
    json_output.append("  },\n");

    // =================================================================
    // 2. DADOS DO BINDER (Filas, Cores Preferidos)
    // =================================================================
    json_output.append("  \"binder\": {\n");
    json_output.append("    \"threads_ativas\": %d,\n", scheduler::get_active_binder_threads());
    json_output.append("    \"core_preferido_id\": %d,\n", get_preferred_cpu_core()); // Do arquivo anterior
    json_output.append("    \"total_transacoes\": %lu\n", binder_state::get_total_transactions());
    json_output.append("  },\n");

    // =================================================================
    // 3. ESTATISTICAS DE CPU POR NÚCLEO
    // =================================================================
    json_output.append("  \"cpus\": [\n");
    
    const auto& topo = cpu::get_topology_info();
    for (int i = 0; i < topo.total_core_count; ++i) {
        // Acesso a dados atomicos de baixa latencia (do __atomic_get_cpu_info_ip_binder.cc)
        uint32_t freq = atomic_read_core_frequency(i);
        uint8_t load = atomic_read_core_load(i);
        
        json_output.append("    {\n");
        json_output.append("      \"id\": %d,\n", i);
        json_output.append("      \"tipo\": \"%s\",\n", (topo.is_big_core(i) ? "BIG" : "LITTLE"));
        json_output.append("      \"frequencia_mhz\": %u,\n", freq);
        json_output.append("      \"carga_perc\": %u\n", load);
        json_output.append("    }%s\n", (i < topo.total_core_count - 1 ? "," : ""));
    }
    json_output.append("  ]\n"); // Fim de "cpus"

    json_output.append("}\n"); // Fim do Objeto Principal

    return json_output;
}

/**
 * @brief Implementacao do comando Dexter/Shell para obter infos JSON.
 * @param output_buffer Buffer de saida para a resposta do shell.
 * @return Status de sucesso/falha.
 */
int handle_json_info_command(StringBuffer& output_buffer) {
    StringBuffer json_data = get_diagnostic_json();
    
    if (json_data.get_size() == 0) {
        output_buffer.append("ERROR: Falha ao gerar JSON do Nucleum.");
        return -1;
    }
    
    // Copia a string JSON para o buffer de resposta do Dexter.
    output_buffer.append(json_data.get_string());
    return 0;
}

} // namespace nucleum
} // namespace binder
} // namespace kernel
} // namespace comandro
