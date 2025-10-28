package comandro.kernel.core.packages.input;

import comandro.settings.debug.ComandroDebug;
import comandro.kernel.core.packages.drivers.DriverManager;
import comandro.kernel.core.packages.drivers.DriverManager.DriverStatus;

/**
 * InputManager: Gerenciador e Calibrador de Eventos de Entrada.
 *
 * Responsável por inicializar drivers de input (touchscreen, teclado), 
 * ler os eventos brutos de seus buffers nativos e calibrá-los antes de enviá-los 
 * para a camada de UI/aplicação.
 */
public final class InputManager {

    private static final String TOUCHSCREEN_DRIVER_NAME = "touchscreen";
    // O endereço do buffer de eventos do kernel para o driver Rust
    private static long nativeEventBufferPtr = 0; 
    
    // Estrutura interna para o evento de toque
    public static final class CalibratedTouchEvent {
        public long timestampNs;
        public int type;
        public int x; // Coordenada X Calibrada
        public int y; // Coordenada Y Calibrada
    }
    
    // Método nativo para alocar o buffer de I/O de baixa latência no kernel
    private static native long native_allocate_io_buffer(int sizeBytes);
    
    // Método nativo para ler e consumir o próximo evento do buffer (Zero-Copy)
    private static native CalibratedTouchEvent native_read_next_event();
    
    // Método nativo para chamar a função de controle (control_driver) do Rust
    private static native int native_control_driver(String driverName, int commandCode);


    private InputManager() {
        // Classe estática
    }

    /**
     * @brief Inicializa o subsistema de entrada.
     */
    public static void initialize() {
        ComandroDebug.i("INPUT_MGR", "Iniciando InputManager...");
        
        // 1. Aloca o buffer de I/O nativo (onde o driver Rust escreve)
        // O tamanho deve ser suficiente para alguns eventos de toque (ex: 1024 bytes)
        nativeEventBufferPtr = native_allocate_io_buffer(1024);
        
        if (nativeEventBufferPtr <= 0) {
            ComandroDebug.e("INPUT_MGR", "Falha ao alocar buffer de I/O nativo. Input desativado.");
            return;
        }
        
        // 2. Carrega o Driver de Touchscreen (Rust)
        DriverStatus status = DriverManager.loadDriver(TOUCHSCREEN_DRIVER_NAME, nativeEventBufferPtr);
        
        if (status == DriverStatus.ACTIVE) {
            ComandroDebug.i("INPUT_MGR", "Driver de Touchscreen (Rust) pronto e conectado ao buffer: 0x" + Long.toHexString(nativeEventBufferPtr));
            // Inicia o thread de leitura de eventos
            startEventPollingThread();
        } else {
            ComandroDebug.e("INPUT_MGR", "Falha ao carregar driver de Touchscreen.");
        }
    }
    
    /**
     * @brief Thread de alta prioridade para ler eventos do buffer nativo.
     */
    private static void startEventPollingThread() {
        // Cria um Thread que faz a leitura do buffer nativo
        Thread inputThread = new Thread(() -> {
            ComandroDebug.d("INPUT_MGR", "Thread de Polling de Input Iniciada (PRIORITY_REAL_TIME)");
            
            // Em um kernel real, esta Thread seria agendada com TaskScheduler.PRIORITY_REAL_TIME
            
            while (!Thread.interrupted()) {
                try {
                    // O método nativo espera por um evento no buffer e o consome
                    CalibratedTouchEvent event = native_read_next_event();
                    
                    if (event != null) {
                        // Envia o evento calibrado para a camada de UI/Window Manager
                        processCalibratedEvent(event);
                    } else {
                        // Sleep opcional de baixa latência se o buffer estiver vazio
                        Thread.sleep(1); 
                    }
                } catch (InterruptedException e) {
                    break;
                }
            }
        }, "Input_Polling_Thread");
        
        inputThread.start();
    }
    
    /**
     * @brief Processa o evento calibrado e o envia para os listeners de UI.
     */
    private static void processCalibratedEvent(CalibratedTouchEvent event) {
        // A UI consumiria este evento.
        ComandroDebug.d("INPUT_EVENT", 
            String.format("Event: Type=%d, X=%d, Y=%d (ts=%d)", 
                          event.type, event.x, event.y, event.timestampNs));
    }
    
    /**
     * @brief Chama a rotina de calibração no driver nativo.
     */
    public static void calibrateTouchscreen() {
        int result = native_control_driver(TOUCHSCREEN_DRIVER_NAME, 2 /* TouchCommand::Calibrate */);
        if (result == 0) {
             ComandroDebug.i("INPUT_MGR", "Calibracao de touchscreen iniciada/comandada.");
        } else {
             ComandroDebug.e("INPUT_MGR", "Falha ao iniciar calibracao. Codigo: " + result);
        }
    }
}
