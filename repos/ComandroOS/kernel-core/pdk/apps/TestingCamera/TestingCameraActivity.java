package comandro.pdk.apps.TestingCamera;

import comandro.os.Activity;
import comandro.os.Camera;
import comandro.os.SurfaceView;
import comandro.os.Log;

/**
 * TestingCameraActivity: Aplicativo de teste simples usando a API de camera legada (Camera 1).
 * Foco em inicializacao rapida e capturas basicas.
 */
public class TestingCameraActivity extends Activity implements Camera.PreviewCallback {

    private static final String TAG = "TestingCamera1";
    private Camera mCamera;
    private SurfaceView mPreview;

    @Override
    public void onCreate() {
        super.onCreate();
        // Assume que a SurfaceView foi inflada via XML (setContentView)
        // mPreview = findViewById(R.id.camera_preview_surface); 
        Log.i(TAG, "TestingCamera 1.0 - Inicializando interface.");
    }

    @Override
    public void onResume() {
        super.onResume();
        
        try {
            // 1. Abrir a camera (bloqueante)
            mCamera = Camera.open(0); // Abrir camera traseira padrao
            if (mCamera == null) {
                Log.e(TAG, "Falha ao abrir o dispositivo de camera.");
                return;
            }

            // 2. Configurar o Preview
            mCamera.setPreviewDisplay(mPreview.getHolder());
            mCamera.setPreviewCallback(this);
            
            // 3. Iniciar o fluxo de dados (Iniciando o IRQ Handler no kernel)
            mCamera.startPreview();
            Log.i(TAG, "Camera 1.0 Preview iniciado.");

        } catch (Exception e) {
            Log.e(TAG, "Erro ao iniciar a camera: " + e.getMessage());
        }
    }

    @Override
    public void onPause() {
        if (mCamera != null) {
            // Parar o fluxo e liberar recursos de hardware
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
            Log.i(TAG, "Camera 1.0 liberada.");
        }
        super.onPause();
    }

    /**
     * Callback chamado a cada frame de preview recebido do kernel.
     * Esta e a camada onde a latencia pode ser medida.
     * @param data Buffer de imagem.
     * @param camera Objeto da camera.
     */
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        // Logica de medicao de latencia da imagem (PDK Level)
        long currentTimestamp = System.nanoTime();
        // Log.v(TAG, "Frame recebido em: " + currentTimestamp);
        // [Teste PDK]: Analisar se o delta de tempo entre frames e consistente e baixo.
    }
    
    // Metodos adicionais para tirar foto (capture()) e gravar video seriam implementados aqui.
}
