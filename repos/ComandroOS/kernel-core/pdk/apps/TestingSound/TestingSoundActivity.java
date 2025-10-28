package comandro.pdk.apps.TestingSound;

import comandro.os.Activity;
import comandro.media.AudioTrack;
import comandro.media.AudioRecord;
import comandro.media.AudioFormat;
import comandro.os.Log;
import java.nio.ByteBuffer;

/**
 * TestingSoundActivity: Aplicativo de teste do pipeline de áudio (PDK Level).
 * Focado em medir a latencia 'round-trip' (entrada -> kernel -> saida).
 */
public class TestingSoundActivity extends Activity {

    private static final String TAG = "TestingSoundPDK";
    private static final int SAMPLE_RATE = 48000; // Taxa de amostragem padrao
    private static final int BUFFER_SIZE_FRAMES = 128; // Buffer ultra-baixo (2.6 ms de latencia)
    private static final int BUFFER_SIZE_BYTES = BUFFER_SIZE_FRAMES * 2; // 16-bit Mono

    private AudioTrack mAudioTrack;
    private AudioRecord mAudioRecord;
    private Thread mTestThread;
    private volatile boolean mIsRunning = false;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "TestingSound: Inicializando com taxa de amostragem: " + SAMPLE_RATE + "Hz");
    }

    @Override
    public void onResume() {
        super.onResume();
        // Garante que a thread de teste esteja limpa e inicia o teste
        if (mTestThread == null) {
            startAudioTest();
        }
    }

    /**
     * @brief Configura e inicia as pipelines de gravação e reprodução.
     */
    private void startAudioTest() {
        try {
            // 1. Configurar AudioRecord (Entrada/Input)
            mAudioRecord = new AudioRecord(
                AudioFormat.SOURCE_DEFAULT, 
                SAMPLE_RATE, 
                AudioFormat.CHANNEL_IN_MONO, 
                AudioFormat.ENCODING_PCM_16BIT, 
                BUFFER_SIZE_BYTES
            );

            // 2. Configurar AudioTrack (Saída/Output)
            mAudioTrack = new AudioTrack(
                AudioFormat.STREAM_DEFAULT,
                SAMPLE_RATE,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                BUFFER_SIZE_BYTES,
                AudioTrack.MODE_STREAM
            );

            mAudioRecord.startRecording();
            mAudioTrack.play();
            Log.i(TAG, "Audio Pipeline: Record/Track iniciados. Buffer de " + BUFFER_SIZE_FRAMES + " frames.");

            mIsRunning = true;
            mTestThread = new Thread(this::runLatencyTest, "ComandroOS_AudioTest");
            mTestThread.start();

        } catch (Exception e) {
            Log.e(TAG, "Falha ao iniciar o teste de audio: " + e.getMessage());
            // Relatar erro critico ao Console UI (ui.java)
            comandro.kernel.core.ui.ConsoleMenuView.reportKernelError("AUD-0x01", "Driver falhou ao alocar recursos.");
        }
    }

    /**
     * @brief O loop principal que mede a latência 'round-trip'.
     * Gera um sinal, o captura e mede o tempo.
     */
    private void runLatencyTest() {
        short[] buffer = new short[BUFFER_SIZE_FRAMES];
        long lastSignalTime = 0;
        long maxLatencyNs = 0;

        // Gerar um pulso de teste (simulacao de um sinal de 1kHz)
        generateTestSignal(buffer);
        
        while (mIsRunning) {
            // 1. Escreve o sinal de teste (Output)
            mAudioTrack.write(buffer, 0, buffer.length);
            lastSignalTime = System.nanoTime();

            // 2. Le o buffer (Input - o som que acabou de sair deve ser capturado)
            int readBytes = mAudioRecord.read(buffer, 0, buffer.length);

            // 3. Medir a latência
            if (readBytes > 0 && lastSignalTime != 0) {
                long currentCaptureTime = System.nanoTime();
                long latencyNs = currentCaptureTime - lastSignalTime;
                
                // Conversao para milissegundos
                double latencyMs = latencyNs / 1000000.0; 
                
                maxLatencyNs = Math.max(maxLatencyNs, latencyNs);
                
                // [CRITICO]: Se a latencia exceder o limite (e.g., 5ms), logar alerta.
                if (latencyMs > 5.0) {
                    Log.alert(TAG, "ALERTA LATENCIA: " + String.format("%.2f", latencyMs) + "ms!");
                }
                
                // Logar a latencia atual (com baixa frequencia para nao sobrecarregar o I/O)
                if (System.currentTimeMillis() % 1000 < 100) {
                    Log.i(TAG, "Latencia atual: " + String.format("%.2f", latencyMs) + "ms");
                }
            }
        }
    }
    
    /**
     * @brief Preenche o buffer com um sinal de teste (onda senoidal simples).
     */
    private void generateTestSignal(short[] buffer) {
        double frequency = 1000.0; // 1kHz
        for (int i = 0; i < buffer.length; i++) {
            double time = (double) i / SAMPLE_RATE;
            // 32767 e o maximo para short (16-bit PCM)
            buffer[i] = (short) (32767.0 * Math.sin(2.0 * Math.PI * frequency * time)); 
        }
    }

    @Override
    public void onPause() {
        mIsRunning = false;
        if (mTestThread != null) {
            try {
                mTestThread.join();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            mTestThread = null;
        }

        if (mAudioRecord != null) {
            mAudioRecord.stop();
            mAudioRecord.release();
        }
        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack.release();
        }
        Log.i(TAG, "Teste de audio finalizado.");
        super.onPause();
    }
}
