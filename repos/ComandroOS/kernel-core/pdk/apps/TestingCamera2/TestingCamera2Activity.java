package comandro.pdk.apps.TestingCamera2;

import comandro.os.Activity;
import comandro.os.CameraManager;
import comandro.os.CameraDevice;
import comandro.os.CaptureRequest;
import comandro.os.Handler;
import comandro.os.Surface;
import comandro.os.Log;
import java.util.Arrays;

/**
 * TestingCamera2Activity: Aplicativo de teste avancado (Camera 2 API).
 * Foco em controle de pipeline, captura em burst e medicões precisas de latencia.
 */
public class TestingCamera2Activity extends Activity {

    private static final String TAG = "TestingCamera2";
    private CameraManager mCameraManager;
    private String mCameraId;
    private CameraDevice mCameraDevice;
    private Surface mPreviewSurface;

    @Override
    public void onCreate() {
        super.onCreate();
        // mPreviewSurface = findViewById(R.id.camera2_preview_surface);
        mCameraManager = (CameraManager) getSystemService(CAMERA_SERVICE);
        Log.i(TAG, "TestingCamera 2.0 - Inicializando CameraManager.");
        
        try {
            // Seleciona a camera padrao
            mCameraId = mCameraManager.getCameraIdList()[0]; 
        } catch (Exception e) {
            Log.e(TAG, "Nenhum ID de camera disponivel.");
        }
    }
    
    @Override
    public void onResume() {
        super.onResume();
        if (mCameraId != null) {
            openCamera();
        }
    }

    private void openCamera() {
        try {
            mCameraManager.openCamera(mCameraId, mStateCallback, new Handler());
        } catch (Exception e) {
            Log.e(TAG, "Falha ao abrir a Camera2: " + e.getMessage());
        }
    }

    // --- Callback de Estado do Dispositivo de Camera ---

    private final CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {
        
        @Override
        public void onOpened(CameraDevice cameraDevice) {
            mCameraDevice = cameraDevice;
            Log.i(TAG, "Camera 2.0 aberta. Criando sessao de captura.");
            createCaptureSession();
        }

        @Override
        public void onDisconnected(CameraDevice cameraDevice) {
            cameraDevice.close();
            mCameraDevice = null;
            Log.w(TAG, "Camera 2.0 desconectada.");
        }

        @Override
        public void onError(CameraDevice cameraDevice, int error) {
            cameraDevice.close();
            mCameraDevice = null;
            Log.e(TAG, "Erro fatal na Camera 2.0: " + error);
        }
    };
    
    // --- Criacao da Sessao de Captura (Pipeline) ---
    
    private void createCaptureSession() {
        try {
            // 1. Criar um Builder para a requisicao de Preview
            CaptureRequest.Builder mPreviewRequestBuilder = 
                mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            mPreviewRequestBuilder.addTarget(mPreviewSurface);
            
            // 2. Definir o pipeline de saida (PreviewSurface, ImageReader)
            mCameraDevice.createCaptureSession(
                Arrays.asList(mPreviewSurface), 
                new CameraCaptureSession.StateCallback() {
                    
                    @Override
                    public void onConfigured(CameraCaptureSession cameraCaptureSession) {
                        // 3. Pipeline configurado: iniciar a requisicao repetitiva
                        try {
                            cameraCaptureSession.setRepeatingRequest(mPreviewRequestBuilder.build(), mCaptureCallback, new Handler());
                            Log.i(TAG, "Requisicao de Preview repetitiva iniciada. Latencia sob teste.");
                        } catch (Exception e) {
                            Log.e(TAG, "Erro ao iniciar request repetitivo: " + e.getMessage());
                        }
                    }

                    @Override
                    public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
                        Log.e(TAG, "Falha na configuracao da sessao de captura (pipeline).");
                    }
                }, 
                null // Handler opcional
            );

        } catch (Exception e) {
            Log.e(TAG, "Erro na criacao da sessao de captura: " + e.getMessage());
        }
    }

    // --- Callback de Resultado da Captura ---
    
    private final CameraCaptureSession.CaptureCallback mCaptureCallback = 
        new CameraCaptureSession.CaptureCallback() {
        
        @Override
        public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
            // Logica de medicao de Latencia de Ponta a Ponta (End-to-End Latency)
            long exposureTime = result.get(CaptureResult.SENSOR_EXPOSURE_TIME);
            long frameTimestamp = result.get(CaptureResult.SENSOR_TIMESTAMP);
            
            // Log.v(TAG, "Frame Capturado. Tempo de Exposição: " + exposureTime + "ns");
            // [Teste PDK]: Analisar o jitter (variacao) no tempo de exposicao e no timestamp.
            // A latencia ultra-baixa de ComandroOS depende deste resultado ser consistente.
        }
        
        // Outros metodos para falhas e progresso seriam implementados aqui.
    };

    @Override
    public void onPause() {
        if (mCameraDevice != null) {
            mCameraDevice.close();
            mCameraDevice = null;
            Log.i(TAG, "Camera 2.0 liberada.");
        }
        super.onPause();
    }
}
