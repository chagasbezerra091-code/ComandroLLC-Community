package comandro.pdk.apps.TestingSensors;

import comandro.os.Activity;
import comandro.os.SensorManager;
import comandro.os.Sensor;
import comandro.os.SensorEvent;
import comandro.os.SensorEventListener;
import comandro.os.Log;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * TestingSensorsActivity: Aplicativo de teste abrangente para todos os sensores nao-criticos.
 * Focado em validar a disponibilidade, latencia e custo de energia.
 */
public class TestingSensorsActivity extends Activity implements SensorEventListener {

    private static final String TAG = "TestingSensorsPDK";
    
    // Taxa de amostragem alta para teste (500 Hz ou 2ms de intervalo)
    private static final int SENSOR_RATE_TEST = 500; 
    
    // Limite de latencia aceitavel para sensores nao-criticos (10ms)
    private static final int MAX_LATENCY_MS = 10; 
    
    private SensorManager mSensorManager;
    private List<Sensor> mAvailableSensors;
    
    // Mapas para rastrear latencia e consumo
    private Map<Integer, Long> mLastEventTimeNs = new HashMap<>();
    private Map<Integer, Long> mTotalEvents = new HashMap<>();

    @Override
    public void onCreate() {
        super.onCreate();
        mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        
        // Obtem todos os sensores exceto o giroscopio (ja testado separadamente) e o acelerometro (muito comum)
        mAvailableSensors = mSensorManager.getSensorList(Sensor.TYPE_ALL);
        
        Log.i(TAG, "Sensores Disponiveis: " + mAvailableSensors.size());
        
        // Inicia o teste imediatamente
        startSensorTest();
    }

    /**
     * @brief Itera sobre todos os sensores e os registra na taxa de teste.
     */
    private void startSensorTest() {
        Log.i(TAG, "Iniciando teste de alta taxa para todos os sensores...");
        
        // Remove sensores criticos para focar nos menos testados.
        mAvailableSensors.removeIf(s -> s.getType() == Sensor.TYPE_GYROSCOPE || 
                                        s.getType() == Sensor.TYPE_ACCELEROMETER);

        if (mAvailableSensors.isEmpty()) {
            Log.w(TAG, "Nenhum sensor nao-critico disponivel para teste.");
            comandro.kernel.core.ui.ConsoleMenuView.reportKernelError("SNS-0x03", "NO_TESTABLE_SENSORS");
            return;
        }

        for (Sensor sensor : mAvailableSensors) {
            // 1. Loga Informacoes Base
            Log.i(TAG, String.format("Teste: %s | Max Delay: %dμs | Power: %.2fmA",
                sensor.getName(), sensor.getMaxDelay() / 1000, sensor.getPower()));

            // 2. Registra o listener na taxa alta (500Hz)
            boolean registered = mSensorManager.registerListener(
                this, 
                sensor, 
                SENSOR_RATE_TEST
            );
            
            if (!registered) {
                Log.e(TAG, "Falha ao registrar " + sensor.getName() + " na taxa de teste.");
            } else {
                mLastEventTimeNs.put(sensor.getType(), System.nanoTime());
                mTotalEvents.put(sensor.getType(), 0L);
            }
        }
    }

    @Override
    public void onPause() {
        // Desregistra todos os listeners para evitar drenagem de bateria
        mSensorManager.unregisterListener(this);
        Log.i(TAG, "Todos os listeners de sensor desregistrados.");
        super.onPause();
    }
    
    // --- SensorEventListener Interface ---

    @Override
    public void onSensorChanged(SensorEvent event) {
        int sensorType = event.sensor.getType();
        
        // Incrementa o contador de eventos
        mTotalEvents.put(sensorType, mTotalEvents.getOrDefault(sensorType, 0L) + 1);

        long appTimestampNs = System.nanoTime(); 
        long kernelTimestampNs = event.timestamp;
        
        // 1. MEDICAO DE LATÊNCIA (Kernel -> App)
        long transportLatencyNs = appTimestampNs - kernelTimestampNs;
        double transportLatencyMs = transportLatencyNs / 1000000.0;
        
        if (transportLatencyMs > MAX_LATENCY_MS) {
            Log.alert(TAG, String.format("ALERTA LATENCIA: %s > %dms! Valor: %.2fms", 
                event.sensor.getName(), MAX_LATENCY_MS, transportLatencyMs));
        }

        // 2. MEDICAO DE JITTER (Consistência de amostragem)
        if (mLastEventTimeNs.containsKey(sensorType)) {
            long lastTime = mLastEventTimeNs.get(sensorType);
            long timeSinceLastEventNs = appTimestampNs - lastTime;
            
            long targetIntervalNs = 1_000_000_000 / SENSOR_RATE_TEST; // 2ms = 2,000,000 ns
            long jitterNs = Math.abs(timeSinceLastEventNs - targetIntervalNs);

            // Loga o Jitter se for alto, mas apenas ocasionalmente
            if (jitterNs > targetIntervalNs && mTotalEvents.get(sensorType) % 100 == 0) {
                 Log.warn(TAG, String.format("JITTER: %s. Delta: %.2fms", 
                     event.sensor.getName(), jitterNs / 1000000.0));
            }
        }

        // Atualiza o timestamp para a proxima iteracao
        mLastEventTimeNs.put(sensorType, appTimestampNs);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.v(TAG, String.format("Precisao de %s mudou para %d", sensor.getName(), accuracy));
    }
}
