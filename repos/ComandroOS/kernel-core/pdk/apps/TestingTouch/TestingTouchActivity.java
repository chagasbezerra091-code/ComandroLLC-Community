package comandro.pdk.apps.TestingTouch;

import comandro.os.Activity;
import comandro.os.MotionEvent;
import comandro.os.Log;
import comandro.os.Process;
import comandro.view.View;

/**
 * TestingTouchActivity: Aplicativo de teste do pipeline de Toque (PDK Level).
 * Focado em medir a latencia de ponta a ponta do evento de toque (Touch Latency).
 */
public class TestingTouchActivity extends Activity implements View.OnTouchListener {

    private static final String TAG = "TestingTouchPDK";
    
    // Limite superior aceitável para a latência de toque em tempo real.
    private static final int MAX_LATENCY_MS = 10; // 10ms é o limite de 'percepção' do usuário
    private static final int WARNING_LATENCY_MS = 5; // 5ms é o alvo de performance do ComandroOS

    private View mTouchArea;
    private long mTotalEvents = 0;

    @Override
    public void onCreate() {
        super.onCreate();
        
        // mTouchArea = findViewById(R.id.touch_test_view); 
        // Simplificando: A Activity assume que a view principal é a area de toque
        mTouchArea = getWindow().getDecorView(); 
        
        mTouchArea.setOnTouchListener(this);
        Log.i(TAG, "TestingTouch: Inicializado. Pronta para receber eventos.");
        
        // Habilitar modo de baixa latência (API do kernel)
        Process.setLatencyPolicy(Process.LatencyPolicy.TOUCH_CRITICAL);
    }

    @Override
    public void onResume() {
        super.onResume();
        mTotalEvents = 0;
    }

    /**
     * @brief Listener principal que recebe e mede o evento de toque.
     * @param v A View onde o toque ocorreu.
     * @param event O evento de movimento (touch) com o timestamp do kernel.
     * @return true para indicar que o evento foi consumido.
     */
    @Override
    public boolean onTouch(View v, MotionEvent event) {
        // Ignora eventos de movimento (MOVE) e foca em DOWN e UP.
        if (event.getAction() != MotionEvent.ACTION_DOWN && event.getAction() != MotionEvent.ACTION_UP) {
            return false;
        }

        mTotalEvents++;
        
        // 1. Timestamp do Kernel (quando o driver de toque reportou o evento)
        long driverTimestampNs = event.getEventTimeNanos();
        // 2. Timestamp da Aplicação (quando a thread recebeu o evento)
        long appTimestampNs = System.nanoTime(); 

        // 3. MEDICAO DE LATÊNCIA DE TRANSPORTE (Driver -> Kernel -> App)
        // É o tempo total que o evento passou pelo sistema operacional.
        long totalLatencyNs = appTimestampNs - driverTimestampNs;
        double totalLatencyMs = totalLatencyNs / 1000000.0;
        
        // Conversão dos limites para nanossegundos para comparação de baixa latência
        long maxLatencyNs = MAX_LATENCY_MS * 1_000_000;
        long warningLatencyNs = WARNING_LATENCY_MS * 1_000_000;

        if (totalLatencyNs > maxLatencyNs) {
            // Latência catastrófica: o usuário certamente perceberá.
            Log.alert(TAG, "ALERTA CRÍTICO: Latência de Toque > " + MAX_LATENCY_MS + "ms! Valor: " + String.format("%.2f", totalLatencyMs) + "ms.");
            comandro.kernel.core.ui.ConsoleMenuView.reportKernelError("TCH-0x05", "LATENCY_FAILURE:" + String.format("%.0f", totalLatencyMs));
        } else if (totalLatencyNs > warningLatencyNs) {
            // Latência alta: falha no alvo de performance do ComandroOS.
            Log.warn(TAG, "ALERTA PERFORMANCE: Latência de Toque alta. Valor: " + String.format("%.2f", totalLatencyMs) + "ms.");
        } else {
            // Sucesso: Atendimento do alvo de performance.
            Log.v(TAG, "Latência OK: " + String.format("%.2f", totalLatencyMs) + "ms.");
        }
        
        // Log dos dados (apenas para debug de toque DOWN)
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            Log.i(TAG, String.format("Touch DOWN em (%.0f, %.0f). Total Eventos: %d", 
                event.getX(), event.getY(), mTotalEvents));
        }
        
        return true;
    }
    
    @Override
    public void onPause() {
        // Restaurar política de latência
        Process.setLatencyPolicy(Process.LatencyPolicy.DEFAULT);
        super.onPause();
    }
}
