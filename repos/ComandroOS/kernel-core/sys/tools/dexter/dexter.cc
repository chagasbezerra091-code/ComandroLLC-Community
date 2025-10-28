#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

// Incluindo headers de subsistemas do kernel para inspeção
// #include "comandro/kernel/core/packages/scheduler/TaskScheduler.h"
// #include "comandro/kernel/core/memory/MemoryManager.h" 

// Definição do namespace para a ferramenta
namespace comandro {
namespace kernel {
namespace tools {
namespace dexter {

const char* TOOL_NAME = "Dexter - Kernel Diagnostic Tool";

// --- Stubs de Funções Nativas do Kernel ---
// Estas seriam as APIs de baixo nível que Dexter chamaria.
extern "C" {
    // Retorna a contagem total de threads ativas no kernel
    size_t native_get_active_thread_count(); 
    
    // Imprime um dump da pilha de um endereço de thread
    void native_dump_stack(long thread_id);

    // Lê e retorna uma palavra (8 bytes) de um endereço de memória física
    unsigned long long native_read_physical_memory(unsigned long long address); 

    // Retorna um vetor de strings contendo os últimos logs de erro do kernel
    std::vector<std::string> native_get_error_log();
}


/**
 * @brief Dexter: Utilitário de Diagnóstico de Kernel (C++).
 * * Ferramenta essencial para inspecionar e diagnosticar o estado interno 
 * do ComandroOS, focado em logs de baixo nível, memória e threads.
 */
class Dexter {
public:

    /**
     * @brief Ponto de entrada principal para a ferramenta Dexter.
     * @param argc Número de argumentos da linha de comando.
     * @param argv Vetor de argumentos.
     * @return Código de saída (0 para sucesso).
     */
    static int run(int argc, char* argv[]) {
        if (argc < 2 || strcmp(argv[1], "help") == 0) {
            printHelp();
            return 0;
        }

        std::string command = argv[1];

        if (command == "mem_peek") {
            if (argc < 3) {
                printf("Uso: dexter mem_peek <endereco_hex>\n");
                return 1;
            }
            // Converte o argumento para endereço (base 16)
            unsigned long long address = std::stoull(argv[2], nullptr, 16);
            peekMemory(address);
        } else if (command == "thread_count") {
            printThreadCount();
        } else if (command == "log_errors") {
            printErrorLogs();
        } else if (command == "stack_trace") {
            if (argc < 3) {
                printf("Uso: dexter stack_trace <thread_id>\n");
                return 1;
            }
            long thread_id = std::atol(argv[2]);
            dumpStackTrace(thread_id);
        } else {
            printf("Comando desconhecido: %s. Use 'dexter help'.\n", command.c_str());
            return 1;
        }

        return 0;
    }

private:
    
    // --- Comandos de Diagnóstico ---

    /**
     * @brief Imprime o número total de threads ativas no kernel.
     */
    static void printThreadCount() {
        size_t count = native_get_active_thread_count();
        // Uso de printf no estilo de baixo nível com %zu para size_t
        printf("[%s] threads ativas: %zu\n", TOOL_NAME, count);
    }

    /**
     * @brief Lê e imprime o conteúdo de uma posição de memória física.
     * @param address O endereço de memória para ler.
     */
    static void peekMemory(unsigned long long address) {
        unsigned long long value = native_read_physical_memory(address);
        
        // Uso de printf com formato hexadecimal para endereços de memória
        printf("[%s] Endereco 0x%llX: 0x%llX\n", TOOL_NAME, address, value);
    }

    /**
     * @brief Imprime os últimos logs de erro do subsistema de logs do kernel.
     */
    static void printErrorLogs() {
        std::vector<std::string> errors = native_get_error_log();
        
        printf("[%s] Logs de Erro Encontrados: %zu\n", TOOL_NAME, errors.size());
        if (errors.empty()) {
            printf("Nenhum log de erro critico pendente.\n");
            return;
        }

        // Imprime os logs encontrados
        for (const auto& log : errors) {
            printf(" [LOG] -> %s\n", log.c_str());
        }
    }

    /**
     * @brief Solicita um dump da pilha (stack trace) para uma thread específica.
     * @param thread_id O ID da thread para inspecionar.
     */
    static void dumpStackTrace(long thread_id) {
        printf("[%s] Solicitando stack dump para Thread ID: %ld...\n", TOOL_NAME, thread_id);
        // O kernel nativo executa o dump e imprime diretamente na TTY
        native_dump_stack(thread_id);
        printf("[DUMP] Fim do stack dump.\n");
    }

    /**
     * @brief Imprime a mensagem de ajuda e uso.
     */
    static void printHelp() {
        printf("\n============================================\n");
        printf("  %s\n", TOOL_NAME);
        printf("============================================\n");
        printf("Uso: dexter <comando> [argumentos]\n\n");
        printf("Comandos:\n");
        printf("  help                - Exibe esta ajuda.\n");
        printf("  thread_count        - Exibe o numero de threads ativas.\n");
        printf("  mem_peek <addr_hex> - Le o valor de 8 bytes no endereco de memoria (ex: 0x1A00).\n");
        printf("  stack_trace <id>    - Imprime o stack trace (pilha) de uma thread especifica.\n");
        printf("  log_errors          - Lista os ultimos logs de erro critico.\n");
        printf("\n");
    }
};


// ------------------------------------------------------------------
// Ponto de entrada C (Chamado pelo Kernel ou Shell)
// ------------------------------------------------------------------
// O kernel/shell chamaria esta função.

extern "C" int main_dexter(int argc, char* argv[]) {
    // Inicialização da biblioteca C++ (se necessário)
    return Dexter::run(argc, argv);
}

// ------------------------------------------------------------------
// Implementação de Stubs de Funções Nativas (A serem implementadas no Kernel Core)
// ------------------------------------------------------------------

// Implementação Stub para Demonstração:

extern "C" size_t native_get_active_thread_count() {
    // Deveria ler o estado do TaskScheduler
    return 42; // Simulação de 42 threads ativas
}

extern "C" void native_dump_stack(long thread_id) {
    printf("  -> [DUMP] Base: 0xDEAD0000\n");
    printf("  -> [DUMP] RBP: 0xDEADBEEF\n");
    printf("  -> [DUMP] Chamada: kernel::scheduler::schedule_loop()\n");
}

extern "C" unsigned long long native_read_physical_memory(unsigned long long address) {
    // Apenas retorna um valor fixo ou simulado para o endereço
    return 0xAAAAAAAA00000000ULL | (address & 0xFF);
}

extern "C" std::vector<std::string> native_get_error_log() {
    std::vector<std::string> logs;
    logs.push_back("OOM: Processo ID 12 (AppService) encerrado.");
    logs.push_back("IRQ_42: Interrupcao Touchscreen nao reconhecida.");
    return logs;
}

} // namespace dexter
} // namespace tools
} // namespace kernel
} // namespace comandro
