package comandro.kernel.core.packages.network;

import comandro.settings.debug.ComandroDebug;
import java.io.Closeable;
import java.io.IOException;

/**
 * LowLatencySocket: Uma implementação de socket otimizada para Zero Latência.
 *
 * Esta classe usa métodos nativos (implementados em C/C++) para interagir
 * diretamente com a pilha TCP/IP do kernel, minimizando a latência de I/O de rede.
 * Evita o overhead da API de Sockets padrão do Java sempre que possível.
 */
public final class LowLatencySocket implements Closeable {

    // Identificador opaco do socket no lado nativo (C/C++ do kernel)
    private final long nativeHandle; 
    
    // --- Métodos Nativos de Baixo Nível ---
    private static native long native_open(String host, int port, boolean noDelay);
    private static native int native_send(long handle, byte[] data, int offset, int length);
    private static native int native_receive(long handle, byte[] buffer, int offset, int length, long timeoutMs);
    private static native void native_close(long handle);

    /**
     * Construtor privado. Use o método estático createConnection.
     * @param handle O identificador nativo do socket.
     */
    private LowLatencySocket(long handle) {
        this.nativeHandle = handle;
    }

    /**
     * Cria e estabelece uma conexão de socket de baixa latência.
     * @param host O endereço do host.
     * @param port A porta.
     * @param tcpNoDelay Define o TCP_NODELAY (desabilita o algoritmo Nagle) para latência mínima.
     * @return Uma instância de LowLatencySocket.
     * @throws IOException Se a conexão falhar.
     */
    public static LowLatencySocket createConnection(String host, int port, boolean tcpNoDelay) throws IOException {
        ComandroDebug.i("NET_SDK", "Tentando conectar a " + host + ":" + port + " (NoDelay=" + tcpNoDelay + ")");
        
        long handle = native_open(host, port, tcpNoDelay);
        
        if (handle <= 0) {
            ComandroDebug.e("NET_SDK", "Falha ao abrir socket nativo para " + host);
            throw new IOException("Failed to open native low-latency socket.");
        }
        
        ComandroDebug.d("NET_SDK", "Socket aberto com handle: " + handle);
        return new LowLatencySocket(handle);
    }
    
    /**
     * Envia dados de forma síncrona, priorizando o caminho de baixa latência.
     * @param data O array de bytes a ser enviado.
     * @param offset O início dos dados no array.
     * @param length O número de bytes a enviar.
     * @return O número de bytes enviados.
     * @throws IOException Se a operação de envio falhar.
     */
    public int send(byte[] data, int offset, int length) throws IOException {
        long startNanos = System.nanoTime();
        int sent = native_send(this.nativeHandle, data, offset, length);
        ComandroDebug.measureTime("NET_SEND", startNanos);
        
        if (sent < 0) {
             throw new IOException("Native send failed.");
        }
        return sent;
    }

    /**
     * Recebe dados do socket.
     * @param buffer O buffer onde os dados serão escritos.
     * @param offset O índice inicial no buffer.
     * @param length O número máximo de bytes a receber.
     * @param timeoutMs O timeout em milissegundos para a operação.
     * @return O número de bytes recebidos, ou -1 se a conexão for fechada.
     * @throws IOException Se a operação de recebimento falhar.
     */
    public int receive(byte[] buffer, int offset, int length, long timeoutMs) throws IOException {
        long startNanos = System.nanoTime();
        int received = native_receive(this.nativeHandle, buffer, offset, length, timeoutMs);
        ComandroDebug.measureTime("NET_RECV", startNanos);

        if (received < 0) {
             throw new IOException("Native receive failed.");
        }
        return received;
    }
    
    /**
     * Implementação de Closeable para fechar o socket de forma segura.
     */
    @Override
    public void close() {
        if (this.nativeHandle > 0) {
            ComandroDebug.d("NET_SDK", "Fechando socket com handle: " + this.nativeHandle);
            native_close(this.nativeHandle);
        }
    }
}
