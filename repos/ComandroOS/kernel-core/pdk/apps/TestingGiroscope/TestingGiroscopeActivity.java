package comandro.pdk.apps.TestingGiroscope;

import comandro.os.Activity;
import comandro.os.SensorManager;
import comandro.os.Sensor;
import comandro.os.SensorEvent;
import comandro.os.SensorEventListener;
import comandro.os.Log;
import comandro.os.SystemProperties;

/**
 * TestingGiroscopeActivity: Aplicativo de teste do pipeline do Sensor (PDK Level).
 * Focado em medir a latencia e o jitter do giroscopio (Gyroscope).
 */
public class TestingGiroscopeActivity extends Activity implements SensorEventListener {

    private static final String TAG = "TestingGyroPDK";
    
    // Frequencia alvo de leitura: 1000 Hz (1ms de atraso maximo entre leituras)
    private static final int SENSOR_RATE_HZ = 1000; 
    private static final int MAX_LATENCY_MS = 2; // Limite de latencia aceitavel
    private static final int MAX_JITTER_NS = 1_000_000; // Limite de 1ms de jitter

    private SensorManager mSensorManager;
    private Sensor mGyroscope;
    
    private long mLastEventTimestampNs = 0;
    private long mTotalEvents = 0;

    @Override
    public void onCreate() {
        super.onCreate();
        mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        
        // 1. Tenta obter o sensor de giroscopio padrao
        mGyroscope = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);

        if (mGyroscope == null) {
            Log.e(TAG, "Giroscópio não encontrado. Teste falhou.");
            // Reporta erro ao Console UI
            comandro.kernel.core.ui.ConsoleMenuView.reportKernelError("SNS-0x01", "GYROSCOPE_NOT_FOUND");
            finish();
            return;
        }

        Log.i(TAG, "Giroscópio encontrado. Taxa alvo: " + SENSOR_RATE_HZ + "Hz");
    }

    @Override
    public void onResume() {
        super.onResume();
        
        // 2. Registra o listener com a taxa mais alta possivel
        boolean registered = mSensorManager.registerListener(
            this, 
            mGyroscope, 
            SENSOR_RATE_HZ // Passando 1000Hz como o "delay" desejado
        );

        if (!registered) {
            Log.e(TAG, "Falha ao registrar listener na taxa desejada.");
            comandro.kernel.core.ui.ConsoleMenuView.reportKernelError("SNS-0x02", "REGISTER_RATE_FAILURE");
        } else {
            // Se o sistema suporta baixa latencia, ele deve logar a taxa real
            int actualRate = SystemProperties.getInt("comandro.sensor.gyro.rate_actual", SENSOR_RATE_HZ);
            Log.i(TAG, "Girosópio registrado. Taxa real esperada: " + actualRate + "Hz");
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        // 3. Desregistra o listener para economizar energia
        mSensorManager.unregisterListener(this);
        Log.i(TAG, "Giroscópio desregistrado. Total de eventos: " + mTotalEvents);
    }
    
    // --- SensorEventListener Interface ---

    /**
     * @brief Chamado quando o sensor reporta novos dados.
     * @param event O evento do sensor contendo os dados e o timestamp do kernel.
     */
    @Override
    public void onSensorChanged(SensorEvent event) {
        mTotalEvents++;
        
        // Timestamp do Kernel (quando o dado saiu do driver do sensor)
        long kernelTimestampNs = event.timestamp;
        // Timestamp da Aplicação (quando a thread recebeu o evento)
        long appTimestampNs = System.nanoTime(); 

        // 1. MEDICAO DE LATÊNCIA DE TRANSPORTE (Kernel -> App)
        long transportLatencyNs = appTimestampNs - kernelTimestampNs;
        double transportLatencyMs = transportLatencyNs / 1000000.0;
        
        if (transportLatencyMs > MAX_LATENCY_MS) {
            Log.alert(TAG, "ALERTA LATENCIA: " + String.format("%.2f", transportLatencyMs) + "ms. Kernel IPC muito lento.");
        }
        
        // 2. MEDICAO DE JITTER (Consistência da entrega de eventos)
        if (mLastEventTimestampNs != 0) {
            long eventDeltaNs = kernelTimestampNs - mLastEventTimestampNs;
            long targetDeltaNs = 1_000_000_000 / SENSOR_RATE_HZ; // 1ms = 1,000,000 ns
            long jitterNs = Math.abs(eventDeltaNs - targetDeltaNs);
            
            if (jitterNs > MAX_JITTER_NS) {
                Log.alert(TAG, "ALERTA JITTER: " + (jitterNs / 1000000.0) + "ms. Sensor/Scheduler inconsistente.");
            }
        }
        
        // 3. Log dos dados (apenas para debug)
        if (mTotalEvents % 100 == 0) {
            Log.v(TAG, String.format("Gyro: X=%.2f, Y=%.2f, Latencia: %.2fms", 
                event.values[0], event.values[1], transportLatencyMs));
        }

        mLastEventTimestampNs = kernelTimestampNs;
    }

    /**
     * @brief Chamado quando a precisao do sensor muda. Ignorado para o teste de latencia.
     */
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // Nenhuma acao necessaria para este teste de desempenho.
    }
}
