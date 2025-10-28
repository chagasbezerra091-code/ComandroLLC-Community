package comandro.dev.tools.debug;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

/**
 * ComandroTool: A Interface de Linha de Comando (CLI) para o desenvolvedor.
 * Funciona como o 'adb' de alto nível.
 */
public class ComandroTool {

    private static final String CDB_HOST = "127.0.0.1";
    private static final int CDB_PORT = 5037; 
    
    // Porta que o kernel daemon (Dexter) escuta.
    public static final int KERNEL_DEBUG_PORT = 4242; 
    
    // IP do dispositivo de destino (Para comandos de shell diretos, simplificando a seleção do dispositivo)
    private static final String TARGET_DEVICE_IP = "192.168.1.10"; 

    private static final String HELP_MESSAGE = 
        "ComandroOS Debug Tool (CDB Client)\n" +
        "Uso: java ComandroTool <comando> [argumentos]\n" +
        "Comandos disponiveis:\n" +
        "  devices           Lista todos os dispositivos ComandroOS conectados (via CDB).\n" +
        "  shell <comando>   Executa um comando no shell do kernel (via Dexter) no " + TARGET_DEVICE_IP + ".\n" +
        "  logcat            Exibe logs de streaming do kernel.\n";

    /**
     * @brief Envia o comando para o CDB Server e exibe a resposta.
     */
    private static void sendToCDB(String command) {
        try (
            Socket socket = new Socket(CDB_HOST, CDB_PORT);
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))
        ) {
            out.println(command);

            String line;
            while ((line = in.readLine()) != null) {
                if (line.equals("END_CDB_RESPONSE")) {
                    break;
                }
                System.out.println(line);
            }
        } catch (IOException e) {
            System.err.println("ERRO: Nao foi possivel conectar ao ComandroDebugBridge (CDB).");
            System.err.println("Verifique se ComandroDebugBridge.java esta rodando no host.");
        }
    }

    /**
     * @brief Conecta diretamente ao kernel daemon para executar comandos de shell.
     */
    private static void executeRemoteShell(String fullCommand) {
        try (
            Socket socket = new Socket(TARGET_DEVICE_IP, KERNEL_DEBUG_PORT);
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))
        ) {
            out.println(fullCommand);
            
            StringBuilder response = new StringBuilder();
            String line;
            
            // Loop de leitura da resposta do kernel.
            while ((line = in.readLine()) != null) {
                // A resposta do kernel deve terminar com um token padrao.
                if (line.trim().equals("END_RESPONSE")) { 
                    break;
                }
                response.append(line).append("\n");
            }
            
            System.out.println("\n--- RESPOSTA DO KERNEL ---\n" + response);

        } catch (IOException e) {
            System.err.println("ERRO: Falha na conexao direta com o shell do kernel (porta " + KERNEL_DEBUG_PORT + ").");
            System.err.println("Verifique o IP e se o serviço Dexter esta ativo: " + e.getMessage());
        }
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.out.println(HELP_MESSAGE);
            return;
        }

        String command = args[0];

        switch (command) {
            case "devices":
                sendToCDB("devices");
                break;
            case "shell":
                if (args.length < 2) {
                    System.err.println("ERRO: Comando shell requer um argumento (ex: shell dexter thread_count)");
                    return;
                }
                StringBuilder shellCmd = new StringBuilder();
                for (int i = 1; i < args.length; i++) {
                    shellCmd.append(args[i]).append(" ");
                }
                executeRemoteShell(shellCmd.toString().trim());
                break;
            case "logcat":
                // Logcat seria apenas um 'shell' continuo
                executeRemoteShell("sys log_stream"); 
                break;
            case "reboot":
                executeRemoteShell("sys reboot"); 
                break;
            default:
                System.err.println("Comando desconhecido: " + command);
                System.out.println(HELP_MESSAGE);
                break;
        }
    }
}
