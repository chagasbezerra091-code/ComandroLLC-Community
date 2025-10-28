package comandro.packages.system.talkback;

import comandro.settings.debug.ComandroDebug;
import comandro.packages.settings.accessibility.AccessibilityService; // Conexão com o serviço de acessibilidade

/**
 * TalkBackEngine: Módulo de Leitor de Tela (Screen Reader) do ComandroOS.
 * * Implementa a lógica para ler eventos de UI usando uma API de Text-to-Speech (TTS) nativa.
 * Ativado e desativado pelo AccessibilityService.
 */
public final class TalkBackEngine {

    // --- API de TTS Nativa (Requer implementação em C++ para o driver de áudio) ---
    private static native boolean native_tts_initialize();
    private static native void native_tts_speak(String text, int queueMode, float pitch, float speed);
    private static native void native_tts_stop();

    // Modos de Fila (para gerenciamento de fala)
    public static final int QUEUE_ADD = 0;    // Adiciona à fila de fala
    public static final int QUEUE_FLUSH = 1;  // Limpa a fila e fala imediatamente

    private static boolean isInitialized = false;
    private static final String ENGINE_VERSION = "1.0-lowlatency";

    private TalkBackEngine() {
        // Classe estática
    }

    /**
     * @brief Inicializa o motor TTS e o sistema TalkBack.
     * @return true se o TTS foi inicializado com sucesso.
     */
    public static boolean initialize() {
        if (isInitialized) {
            return true;
        }

        ComandroDebug.i("TALKBACK", "Inicializando TalkBack Engine (v" + ENGINE_VERSION + ")");
        isInitialized = native_tts_initialize();

        if (isInitialized) {
            ComandroDebug.i("TALKBACK", "TTS API nativa inicializada com sucesso.");
            // Fala inicial para indicar que o serviço está ativo
            speak("ComandroOS Leitor de Tela ativado.", QUEUE_FLUSH, 1.0f, 1.0f);
        } else {
            ComandroDebug.e("TALKBACK", "Falha ao inicializar a API TTS nativa.");
        }
        return isInitialized;
    }

    /**
     * @brief Converte texto em fala usando a API TTS.
     */
    public static void speak(String text, int queueMode, float pitch, float speed) {
        if (!isInitialized) {
            ComandroDebug.w("TALKBACK", "Tentativa de falar sem inicialização: " + text);
            return;
        }
        
        // Só fala se o Screen Reader estiver globalmente ativo no Settings
        if (AccessibilityService.isScreenReaderActive()) {
            native_tts_speak(text, queueMode, pitch, speed);
        } else {
            ComandroDebug.d("TALKBACK", "TTS silenciado (Screen Reader inativo).");
        }
    }
    
    /**
     * @brief Para qualquer fala em andamento.
     */
    public static void stopSpeaking() {
        if (isInitialized) {
            native_tts_stop();
        }
    }
    
    /**
     * @brief Processa um evento de acessibilidade da UI (simulação).
     * @param eventDescription Uma descrição do evento (ex: "Botão 'Enviar' pressionado").
     */
    public static void processAccessibilityEvent(String eventDescription) {
        if (AccessibilityService.isScreenReaderActive()) {
            ComandroDebug.d("TALKBACK_EVENT", "Evento recebido: " + eventDescription);
            // Prioriza a fala para garantir baixa latência no feedback visual/auditivo.
            speak(eventDescription, QUEUE_FLUSH, 1.0f, 1.2f); 
        }
    }
    
    // Método para ser chamado ao desligar o sistema
    public static void shutdown() {
        if (isInitialized) {
            stopSpeaking();
            isInitialized = false;
            ComandroDebug.i("TALKBACK", "TalkBack Engine desligado.");
        }
    }
}
