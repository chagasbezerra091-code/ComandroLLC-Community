/**
 * no_device.js - Tela de Erro de Incompatibilidade Crítica de Hardware (Brick Screen)
 * * Este script é carregado pelo bootloader/sistema de recuperação quando o kernel falha
 * ao iniciar devido a uma incompatibilidade de hardware detectada (ex: após um OTA).
 * Ele exibe uma mensagem de erro crítica em texto vermelho, usando uma fonte monoespaçada.
 *
 * @author ComandroOS Development Team
 */

// --- Dados de Erro ---
const ERROR_TITLE = "CRITICAL HARDWARE FAILURE";
const ERROR_MESSAGE_ENGLISH = "You accidentally updated to a version that is incompatible with your device. Your device has turned into a brick.";
const ERROR_MESSAGE_PORTUGUESE = "Você acidentalmente atualizou para uma versão que não é compatível com seu dispositivo. Seu aparelho virou um tijolo.";
const ACTION_PROMPT = "Please contact technical support with the code shown below.";
const ERROR_CODE = "COMANDRO-FATAL-0XDEB007"; // DEB007 = Dead Boot

// --- Configuração de Estilo (Simulando CSS/Canvas de Baixo Nível) ---
const STYLE_CONFIG = {
    // Fonte que parece "código" (Monospace)
    fontFamily: "'Courier New', monospace", 
    // Cor do texto
    color: "red", 
    // Cor de fundo
    backgroundColor: "black", 
    // Tamanho da fonte em pixels
    fontSize: "16px", 
    // Alinhamento do texto
    textAlign: "center" 
};

/**
 * @brief Função principal para renderizar a tela de erro.
 * Assume que existe um ambiente DOM ou uma API de renderização de texto de kernel.
 */
function renderBrickScreen() {
    // 1. Configura o estilo de fundo
    // Em um ambiente real, isso chamaria uma API nativa: native_set_background(STYLE_CONFIG.backgroundColor);
    if (typeof document !== 'undefined') {
        document.body.style.backgroundColor = STYLE_CONFIG.backgroundColor;
        document.body.style.fontFamily = STYLE_CONFIG.fontFamily;
        document.body.style.color = STYLE_CONFIG.color;
        document.body.style.textAlign = STYLE_CONFIG.textAlign;
        document.body.style.fontSize = STYLE_CONFIG.fontSize;
    }

    // 2. Cria e exibe o conteúdo
    const content = `
        <pre style="line-height: 1.5;">
${'-'.repeat(50)}
        
           [ ${ERROR_TITLE} ]
           
${ERROR_MESSAGE_ENGLISH}
${ERROR_MESSAGE_PORTUGUESE}

${'-'.repeat(50)}
        
           ${ACTION_PROMPT}
           
           ERROR CODE: ${ERROR_CODE}
           
           --- COMANDROOS V${getKernelVersion()} ---

        </pre>
    `;

    // 3. Renderiza o conteúdo (simulando a saída final para a tela)
    if (typeof document !== 'undefined') {
        document.body.innerHTML = content;
    } else {
        // Output para TTY de fallback (ambiente não-DOM)
        console.error(content);
    }
}

/**
 * @brief Obtém a versão do kernel (stub para função nativa).
 */
function getKernelVersion() {
    // Em um ambiente real, chamaria uma API nativa: native_get_kernel_version();
    return "2.1.0-BRICK"; 
}

// Executa a função de renderização ao carregar o script
renderBrickScreen();
