// voice_alert_ui.js (JavaScript - UI/Framework Layer)
// Executado no ComandroOS Application Engine (ART/JavaAppsEngine)

// Objeto FFI para interagir com o kernel C++
const KernelFFI = ComandroOS.getKernelFFI(); 

// Biblioteca de Sintese de Voz (Simulacao)
const VoiceSynth = ComandroOS.getVoiceSynthLibrary();

/**
 * @brief Funcao FFI chamada pelo codigo C++ de baixo nivel.
 * @param message A mensagem de alerta de voz.
 */
KernelFFI.registerFFI("caml_activate_voice_alert", (message) => {
    console.error("ALERTA CRÍTICO DE VOZ ATIVADO!");
    
    // 1. Sintetizar Voz
    // O texto em ingles fornece instrucoes de seguranca.
    VoiceSynth.speak(message, {
        pitch: 0.9,
        rate: 0.8, // Voz mais lenta
        volume: 1.0, 
        language: 'en-US'
    });

    // 2. Alertar Tela de Baixo Nivel (Full Screen Red)
    // O kernel ja setou a flag, esta funcao apenas garante a sobreposicao.
    renderFatalOverlay("LITHIUM DETECTED", "Device Compromised. Discard Immediately.");
});

/**
 * @brief Renderiza o alerta critico no canto da tela (Full Screen Overlay).
 */
function renderFatalOverlay(mainText, subText) {
    const uiContext = ComandroOS.getUIContext();
    
    // 1. Limpa a tela e coloca um fundo vermelho
    uiContext.setGlobalBackgroundColor('rgba(255, 0, 0, 0.95)'); 
    
    // 2. Exibe "LITHIUM DETECTED" em vermelho forte no canto
    uiContext.drawText(mainText, {
        x: '10%',
        y: '10%',
        color: '#FFDDDD', 
        fontSize: '120px',
        fontWeight: 'bold',
        textAlign: 'left'
    });
    
    // 3. Texto secundario no centro
    uiContext.drawText(subText, {
        x: '50%',
        y: '50%',
        color: '#FFFFFF', 
        fontSize: '40px',
        textAlign: 'center'
    });
    
    // 4. Exibe o log de falha (fritura da placa)
    uiContext.drawText("CAUSED MULTIPLE MOTHERBOARD CIRCUITS TO FRY.", {
        x: '50%',
        y: '90%',
        color: '#FFCCCC', 
        fontSize: '24px',
        textAlign: 'center'
    });

    uiContext.flush(); // Renderiza imediatamente
}
