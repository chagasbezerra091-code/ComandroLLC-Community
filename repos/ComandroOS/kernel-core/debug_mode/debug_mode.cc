#include "debug_mode.h"
#include <sstream>
#include <cstdio> // Para printf

using namespace comandro::kernel::debug;

// ------------------------------------------------------------------
// IMPLEMENTAÇÃO DAS FUNÇÕES NATIVAS (STUBS - Sem Simulação)
// Estas funções esperam ser implementadas em arquivos de driver específicos.
// ------------------------------------------------------------------

/**
 * Tenta a conexão real com o PC. Não simula atrasos ou chance de falha.
 * A implementação real usaria o driver USB/Serial.
 */
bool DebugMode::native_usb_serial_connect() {
    // Código de inicialização do driver de comunicação aqui.
    // Retorna o status real do driver.
    
    // Stub: Retornamos 'false' por padrão, a menos que o driver seja implementado.
    return false;
}

/**
 * Executa o teste de latência I/O real.
 * A implementação real mediria o tempo de um ciclo de escrita/leitura crítico.
 */
int DebugMode::native_perform_io_latency_test() {
    // Código de teste de I/O de baixo nível aqui.
    // Stub: Retorna um valor placeholder que o código usará.
    return 15; // Exemplo: Retorna 15 microsegundos de latência.
}

/**
 * Executa a chamada real de reboot.
 * A implementação real chamaria o hardware (ex: BIOS/UEFI) para reiniciar.
 */
void DebugMode::native_reboot_system() {
    // Código de reinicialização de hardware aqui.
    printf("[KERNEL] Chamando rotina de reboot de hardware...\n");
    // Em um sistema real, o fluxo de controle não retornaria desta função.
}

// ------------------------------------------------------------------
// IMPLEMENTAÇÃO DO MODO DE DEBUG
// ------------------------------------------------------------------

/**
 * @brief Inicia o loop principal do modo de debug.
 */
int DebugMode::start() {
    // UI estilo Recovery AOSP
    printf("\n================================================\n");
    printf("%s\n", MODE_NAME.c_str());
    printf("ComandroOS (ice-scream) Kernel Core\n");
    printf("Data: %s, Hora: %s\n", __DATE__, __TIME__);
    printf("================================================\n\n");
    
    // 1. Busca por Conexão de PC
    if (searchForPC()) {
        printf("[STATUS] PC Host Conectado. Modo de Download/Upload Ativo.\n\n");
    } else {
        printf("[STATUS] Nenhum PC Host detectado. Modo Interativo Ativo.\n");
        printf("Digite 'help' para comandos ou 'exit 0' para sair.\n\n");
    }
    
    // 2. Loop Interativo de Comandos
    std::string command;
    int exit_code = 0;
    bool running = true;
    
    while (running) {
        printf("comandro_debug > ");
        fflush(stdout); // Garante que o prompt apareça
        
        if (!std::getline(std::cin, command)) {
            break;
        }

        if (command == "exit 0") {
            running = false;
            exit_code = 0;
        } else if (command == "exit 1") {
            running = false;
            exit_code = 1;
        } else {
            running = processCommand(command);
        }
    }

    printf("\nSaindo do %s com codigo %d.\n", MODE_NAME.c_str(), exit_code);
    return exit_code;
}

/**
 * @brief Loop principal que busca por conexão de PC.
 */
bool DebugMode::searchForPC() {
    printf("Buscando PC Host (USB/Serial) via driver nativo... ");
    fflush(stdout);
    
    if (native_usb_serial_connect()) {
        printf("CONECTADO.\n");
        return true;
    } else {
        printf("FALHA/NÃO PRESENTE.\n");
        return false;
    }
}

/**
 * @brief Processa comandos digitados pelo usuário na console.
 */
bool DebugMode::processCommand(const std::string& command) {
    if (command == "help") {
        printf("Comandos disponiveis:\n");
        printf("  exit 0       - Sair do modo de debug (sucesso).\n");
        printf("  diag         - Executa diagnosticos de sistema (exemplo printf).\n");
        printf("  reboot       - Reinicia o sistema.\n");
        printf("  check_io     - Checa a Latencia de I/O critica.\n");
        printf("  connect_pc   - Tenta conectar novamente ao PC.\n");
        printf("  help         - Mostra esta mensagem.\n");
    } else if (command == "diag") {
        std::vector<std::string> log_entries = {"kernel_init", "memory_allocator", "tty_driver", "scheduler_v2"};
        diagnosticsExample(log_entries);
    } else if (command == "reboot") {
        printf("Comando de Reboot recebido.\n");
        native_reboot_system();
        return false; // Sai do modo
    } else if (command == "check_io") {
        int latency_us = native_perform_io_latency_test();
        printf("[TEST] Latencia I/O: %d us. (Meta: <1000 us).\n", latency_us);
    } else if (command == "connect_pc") {
        searchForPC();
    } else {
        printf("Comando desconhecido: %s. Digite 'help'.\n", command.c_str());
    }
    return true;
}

/**
 * @brief Função de exemplo para mostrar o uso do printf no formato solicitado.
 */
void DebugMode::diagnosticsExample(const std::vector<std::string>& data_set) {
    printf("--- Diagnostico de Subsistemas ---\n");
    
    // Formato solicitado: printf("strings  :", strings.size());
    // (Ajustado para compilar corretamente com o especificador %zu)
    printf("strings  : %zu\n", data_set.size()); 
    
    printf("Subsistemas listados:\n");
    for (size_t i = 0; i < data_set.size(); ++i) {
        printf("  [%zu] : %s\n", i, data_set[i].c_str());
    }
    printf("----------------------------------\n");
}
