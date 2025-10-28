package comandro.kernel.core.packages.shell;

import comandro.settings.debug.ComandroDebug;
import comandro.settings.os_info.OSInfo;
import comandro.settings.saude.SaudeOS;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

/**
 * ComandroShell: Interface de Linha de Comando (CLI) de Baixa Latência.
 *
 * Este shell opera diretamente sobre a TTY ou console serial, 
 * fornecendo acesso aos diagnósticos e comandos do kernel-core.
 */
public final class ComandroShell {

    private static final String PROMPT = OSInfo.VERSION_CODENAME.toUpperCase() + "@ComandroOS # ";

    // Método nativo para chamar o módulo C++ do BatteryManager
    private static native String native_get_battery_status();

    private ComandroShell() {
        // Classe estática
    }

    /**
     * @brief Inicia o loop interativo do Shell.
     */
    public static void start() {
        ComandroDebug.i("SHELL_INIT", "Iniciando ComandroShell.");
        
        System.out.println("\nComandroOS Shell (" + OSInfo.VERSION_NUMBER + ") - Digite 'help'.");
        
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while (true) {
                System.out.print(PROMPT);
                line = reader.readLine();
                
                if (line == null) {
                    break; // Fim do stream (ex: TTY desconectado)
                }
                
                if (processCommand(line.trim())) {
                    break; // Comando de saída
                }
            }
        } catch (IOException e) {
            ComandroDebug.e("SHELL_IO", "Erro na entrada/saída do Shell: " + e.getMessage());
        }
        
        ComandroDebug.i("SHELL_EXIT", "ComandroShell encerrado.");
    }

    /**
     * @brief Processa um comando digitado.
     * @param command O comando de entrada.
     * @return true se um comando de saída foi processado.
     */
    private static boolean processCommand(String command) {
        if (command.isEmpty()) {
            return false;
        }

        String[] parts = command.split("\\s+");
        String cmd = parts[0].toLowerCase();

        switch (cmd) {
            case "exit":
            case "quit":
                return true;
            case "help":
                displayHelp();
                break;
            case "info":
                displayOSInfo();
                break;
            case "saude":
                System.out.println(SaudeOS.gerarRelatorioSaude());
                break;
            case "batt":
            case "battery":
                System.out.println("Status da Bateria: " + native_get_battery_status());
                break;
            default:
                System.out.println("Comando desconhecido: " + cmd + ". Tente 'help'.");
                break;
        }
        return false;
    }

    private static void displayHelp() {
        System.out.println("\n--- Comandos ComandroShell ---\n");
        System.out.println("info        - Exibe informações de versão e liderança do OS.");
        System.out.println("saude       - Exibe o Relatório de Saúde do Sistema (KPIs).");
        System.out.println("batt        - Exibe o status da bateria (usando módulo C++).");
        System.out.println("exit | quit - Sai do Shell.");
        System.out.println("\n------------------------------\n");
    }

    private static void displayOSInfo() {
        System.out.println("\n--- Informações do ComandroOS ---");
        System.out.println("Versão: " + OSInfo.FULL_VERSION);
        System.out.println("CEO & Arq. Chefe: " + OSInfo.CEO_ARQUITETO_CHEFE);
        System.out.println("Co-CEO: " + OSInfo.CO_CEO);
        System.out.println(OSInfo.getCopyrightNotice());
        System.out.println("---------------------------------\n");
    }
}
