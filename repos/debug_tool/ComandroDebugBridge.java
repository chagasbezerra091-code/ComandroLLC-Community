package comandro.dev.tools.debug;

import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * ComandroDebugBridge (CDB): Servidor de fundo no host (PC).
 * Gerencia a detecção de dispositivos, conexões persistentes e roteamento de comandos.
 */
public class ComandroDebugBridge {

    private static final int CDB_HOST_PORT = 5037; 
    private final Map<String, DeviceConnection> connectedDevices = new ConcurrentHashMap<>();
    private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
    
    // Lista de IPs a serem monitorados (Dispositivos na rede)
    private static final String[] MONITORED_DEVICE_IPS = {"192.168.1.10", "192.168.1.11"};

    private static class DeviceConnection {
        public final String serial;
        public volatile String status; 
        public volatile Socket kernelSocket; 

        public DeviceConnection(String serial, String status, Socket kernelSocket) {
            this.serial = serial;
            this.status = status;
            this.kernelSocket = kernelSocket;
        }
    }

    /**
     * @brief Inicia o monitoramento de dispositivos e o servidor de escuta da CLI.
     */
    public void startServer() {
        System.out.println("ComandroDebugBridge (CDB) iniciando na porta " + CDB_HOST_PORT);
        
        // Inicia o agendador para monitorar conexões periodicamente (Polling USB/Network)
        scheduler.scheduleAtFixedRate(this::pollAndConnectDevices, 0, 10, TimeUnit.SECONDS);

        // Inicia o loop de escuta para comandos da CLI (ComandroTool)
        try (ServerSocket serverSocket = new ServerSocket(CDB_HOST_PORT)) {
            while (true) {
                Socket clientSocket = serverSocket.accept();
                new Thread(() -> handleCliCommand(clientSocket)).start();
            }
        } catch (IOException e) {
            System.err.println("Erro fatal no servidor CDB. Reinicie o processo: " + e.getMessage());
        }
    }

    /**
     * @brief Tenta se conectar a todos os IPs monitorados na porta de debug do kernel (4242).
     * Esta é a forma simplificada de detecção de dispositivo por IP/Rede.
     */
    private void pollAndConnectDevices() {
        for (String ip : MONITORED_DEVICE_IPS) {
            String serialId = "COMANDRO-NET-" + ip.replace('.', '-');
            
            try {
                // Tenta estabelecer a conexão TCP/IP na porta 4242
                Socket kernelSock = new Socket(ip, DebugToolClient.DEBUG_PORT);
                
                // Se o dispositivo já estava na lista, atualiza o socket e status
                if (connectedDevices.containsKey(serialId)) {
                    connectedDevices.get(serialId).kernelSocket = kernelSock;
                    connectedDevices.get(serialId).status = "online";
                } else {
                    connectedDevices.put(serialId, new DeviceConnection(serialId, "online", kernelSock));
                }
                
                System.out.println("[CDB] Dispositivo conectado/atualizado: " + serialId);
            } catch (IOException e) {
                // Falha ao conectar. Define/atualiza como offline.
                if (connectedDevices.containsKey(serialId)) {
                    connectedDevices.get(serialId).status = "offline";
                    connectedDevices.get(serialId).kernelSocket = null;
                } else {
                    connectedDevices.put(serialId, new DeviceConnection(serialId, "offline", null));
                }
            }
        }
    }

    /**
     * @brief Trata comandos de listagem de dispositivos (CLI). O roteamento de SHELL é feito pelo cliente.
     */
    private void handleCliCommand(Socket clientSocket) {
        try (
            BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true)
        ) {
            String command = in.readLine();
            if (command != null) {
                if (command.startsWith("devices")) {
                    out.println("Lista de dispositivos ComandroOS conectados:");
                    connectedDevices.values().forEach(dev -> 
                        out.println(dev.serial + "\t" + dev.status));
                    out.println("END_CDB_RESPONSE");
                } else {
                    out.println("FAIL: Comando CDB interno nao suportado.");
                    out.println("END_CDB_RESPONSE");
                }
            }
        } catch (IOException e) {
            // Logar erros de comunicação com a CLI
            System.err.println("[CDB] Erro de comunicação com a CLI: " + e.getMessage());
        }
    }

    public static void main(String[] args) {
        new ComandroDebugBridge().startServer();
    }
}
