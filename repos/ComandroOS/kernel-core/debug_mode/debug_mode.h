#ifndef COMANDRO_DEBUG_MODE_H
#define COMANDRO_DEBUG_MODE_H

#include <string>
#include <iostream>
#include <vector>
#include <cstdio> // Para printf

namespace comandro {
namespace kernel {
namespace debug {

// Nome do Modo de Debug/Recuperação
const std::string MODE_NAME = "ComandroOS Debug Mode";

/**
 * @brief Classe que implementa a lógica do modo de debug e recuperação.
 * * Minimalista, TTY-based, sem simulações.
 */
class DebugMode {
public:
    
    /**
     * @brief Inicia o loop principal do modo de debug.
     * @return O código de saída (0 para sucesso, diferente de zero para erro).
     */
    static int start();

private:
    
    // --- Funções Nativas (Implementadas em Drivers de Baixo Nível) ---
    static bool native_usb_serial_connect(); // Tenta a conexão real com o PC.
    static int native_perform_io_latency_test(); // Executa o teste de latência I/O real.
    static void native_reboot_system(); // Executa a chamada real de reboot.

    // --- Lógica de UI/Comando ---
    
    /**
     * @brief Loop principal que busca por conexão de PC.
     * @return true se um PC foi detectado e a conexão foi estabelecida.
     */
    static bool searchForPC();
    
    /**
     * @brief Processa comandos digitados pelo usuário na console.
     * @param command O comando de entrada.
     * @return true se o modo deve continuar, false se "exit 0" for digitado.
     */
    static bool processCommand(const std::string& command);

    /**
     * @brief Função de exemplo para mostrar o uso do printf para diagnóstico.
     * @param data_set Conjunto de dados para inspeção.
     */
    static void diagnosticsExample(const std::vector<std::string>& data_set);
};

} // namespace debug
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_DEBUG_MODE_H
