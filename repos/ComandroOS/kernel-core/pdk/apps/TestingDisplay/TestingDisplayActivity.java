package comandro.pdk.apps.TestingDisplay;

import comandro.os.Activity;
import comandro.os.DisplayManager;
import comandro.os.SurfaceView;
import comandro.os.Canvas;
import comandro.os.Paint;
import comandro.os.Log;
import comandro.graphics.Color;

/**
 * TestingDisplayActivity: Aplicativo de teste do pipeline de display (PDK Level).
 * Focado em medir a latencia de ponta a ponta e o jitter do V-Sync.
 */
public class TestingDisplayActivity extends Activity implements SurfaceView.Callback {

    private static final String TAG = "TestingDisplayPDK";
    private static final long RENDER_PERIOD_MS = 16; // Aproximadamente 60 FPS
    private static final int MAX_JITTER_NS = 2_000_000; // Limite de Jitter de 2ms
    private static final int MAX_LATENCY_MS = 30; // Limite de latencia total

    private SurfaceView mSurfaceView;
    private DisplayManager mDisplayManager;
    private Paint mPaint;
    private Thread mRenderThread;
    private volatile boolean mIsRendering = false;
    
    private int mFrameCount = 0;
    private long mLastFrameTimeNs = 0;

    @Override
    public void onCreate() {
        super.onCreate();
        // mSurfaceView = findViewById(R.id.display_test_surface);
        // mSurfaceView.getHolder().addCallback(this);
        
        mDisplayManager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
        mPaint = new Paint();
        Log.i(TAG, "TestingDisplay: Inicializando. Target FPS: " + (1000 / RENDER_PERIOD_MS));
    }

    // --- Ciclo de Vida do Surface ---

    @Override
    public void surfaceCreated(SurfaceView holder) {
        startRenderTest();
    }

    @Override
    public void surfaceDestroyed(SurfaceView holder) {
        stopRenderTest();
    }
    
    // Omitindo surfaceChanged para brevidade

    // --- Controle de Renderizacao ---

    private void startRenderTest() {
        if (!mIsRendering) {
            mIsRendering = true;
            mRenderThread = new Thread(this::runRenderLoop, "ComandroOS_RenderTest");
            mRenderThread.start();
        }
    }

    private void stopRenderTest() {
        mIsRendering = false;
        if (mRenderThread != null) {
            try {
                mRenderThread.join();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            mRenderThread = null;
        }
    }

    /**
     * @brief Loop de renderizacao principal focado em medir a consistencia de V-Sync.
     */
    private void runRenderLoop() {
        Log.i(TAG, "Render Loop Iniciado.");
        mLastFrameTimeNs = System.nanoTime();
        
        // Simula o registro no Display Driver do Kernel para receber o sinal V-Sync
        mDisplayManager.registerVsyncListener(this::onVsyncSignal);

        while (mIsRendering) {
            // Sleep for the target frame time to minimize CPU waste
            try {
                Thread.sleep(RENDER_PERIOD_MS);
            } catch (InterruptedException e) {
                break;
            }
        }
        
        mDisplayManager.unregisterVsyncListener(this::onVsyncSignal);
    }
    
    /**
     * @brief Callback chamado pelo kernel no momento do sinal V-Sync do hardware.
     * @param vsyncTimeNs Timestamp do kernel para o V-Sync.
     */
    private void onVsyncSignal(long vsyncTimeNs) {
        if (!mIsRendering) return;

        long currentDrawTimeNs = System.nanoTime();
        long frameDeltaNs = currentDrawTimeNs - mLastFrameTimeNs;
        
        // 1. MEDICAO DE JITTER (Consistencia do tempo de frame)
        // O ideal é que frameDeltaNs seja proximo de 16.666.666 ns (16ms).
        long targetDeltaNs = RENDER_PERIOD_MS * 1_000_000;
        long jitterNs = Math.abs(frameDeltaNs - targetDeltaNs);

        if (jitterNs > MAX_JITTER_NS && mFrameCount > 10) {
            Log.alert(TAG, "JITTER ALTO: " + (jitterNs / 1000000.0) + "ms. Kernel V-Sync inconsistente.");
        }
        
        // 2. DESENHAR O NOVO FRAME
        // O desenho deve ser o mais simples possivel para isolar a latencia do driver.
        drawNextFrame(vsyncTimeNs);

        // 3. MEDICAO DE LATENCIA TOTAL (Da CPU para o Display Hardware)
        long renderToVsyncNs = currentDrawTimeNs - vsyncTimeNs; // Tempo que a CPU gastou apos o V-Sync
        double renderToVsyncMs = renderToVsyncNs / 1000000.0;
        
        if (renderToVsyncMs > MAX_LATENCY_MS) {
            Log.alert(TAG, "LATENCIA TOTAL ALTA: " + renderToVsyncMs + "ms. Buffer de Kernel muito lento.");
        }

        mLastFrameTimeNs = currentDrawTimeNs;
        mFrameCount++;
    }

    /**
     * @brief Desenha um quadrado que muda de cor a cada frame.
     */
    private void drawNextFrame(long vsyncTimeNs) {
        // Bloquear o Canvas para desenhar (simulando a chamada ao SurfaceFlinger/Hardware Composer)
        Canvas canvas = mSurfaceView.getHolder().lockCanvas(); 
        if (canvas != null) {
            try {
                // Alterna a cor para forcar o pixel do display a mudar
                mPaint.setColor(mFrameCount % 2 == 0 ? Color.RED : Color.BLUE);
                canvas.drawColor(Color.BLACK); // Limpa a tela
                
                int boxSize = 200;
                // Desenha um quadrado que se move levemente
                canvas.drawRect(50 + (mFrameCount % 10), 50, 50 + boxSize + (mFrameCount % 10), 50 + boxSize, mPaint);

                // Desenha o V-Sync Time para diagnostico
                mPaint.setColor(Color.WHITE);
                mPaint.setTextSize(40);
                canvas.drawText("Frame: " + mFrameCount, 10, 40, mPaint);
                canvas.drawText("V-Sync TS: " + vsyncTimeNs, 10, 80, mPaint);

            } finally {
                // Envia o frame para o kernel/driver
                mSurfaceView.getHolder().unlockCanvasAndPost(canvas);
            }
        }
    }
    
    @Override
    public void onPause() {
        stopRenderTest();
        super.onPause();
    }
}
