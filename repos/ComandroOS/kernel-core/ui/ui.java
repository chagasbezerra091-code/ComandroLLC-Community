package comandro.kernel.core.ui;

import comandro.os.Log;
import comandro.os.DisplayDriver;
import comandro.os.InputDriver;
import java.util.List;

/**
 * ConsoleMenuView: A view padrao de ComandroOS.
 * Desenha um menu simples de texto e lida com a entrada bruta.
 * Implementa o visual de "letras estranhas" (ASCII art e codigos).
 */
public class ConsoleMenuView {

    private static final String TAG = "ComandroUI_Console";
    private static final int SCREEN_WIDTH = 80;
    private static final int SCREEN_HEIGHT = 25;
    
    // Codigo de status do ultimo erro (para o menu mostrar)
    private static volatile String lastErrorCode = "STATUS: OK";

    private List<String> menuItems;
    private int selectedIndex = 0;
    
    // Objeto que representa a tela (driver de baixo nivel)
    private final DisplayDriver display;
    private final InputDriver input;

    public ConsoleMenuView(List<String> items, DisplayDriver display, InputDriver input) {
        this.menuItems = items;
        this.display = display;
        this.input = input;
    }

    /**
     * @brief Desenha a tela inteira com o menu e o status de erro.
     */
    public void draw() {
        display.clearScreen();
        
        // 1. Desenho do Cabecalho (Visual "Estranho")
        display.writeAt(0, 0, "COMANDRO-OS v1.0 [ALPHA] - KERNEL-UI-MENU");
        display.writeAt(0, 1, "----------------------------------------");
        
        // 2. Desenho dos Itens do Menu
        for (int i = 0; i < menuItems.size(); i++) {
            String prefix = (i == selectedIndex) ? ">>> " : "    ";
            String itemText = prefix + menuItems.get(i);
            display.writeAt(2, 3 + i, itemText);
        }
        
        // 3. Desenho da Zona de Debug/Erro (Bottom Bar)
        display.writeAt(0, SCREEN_HEIGHT - 2, "--------------------------------------------------------------------------------");
        display.writeAt(0, SCREEN_HEIGHT - 1, "AUDIT ZONE: " + lastErrorCode + " | SEL: " + selectedIndex);
    }
    
    /**
     * @brief Loop de entrada de usuario.
     */
    public int handleInput() {
        int key = input.getRawKey();
        
        switch (key) {
            case InputDriver.KEY_UP:
                selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : menuItems.size() - 1;
                draw(); // Redesenha a tela apos o movimento
                break;
            case InputDriver.KEY_DOWN:
                selectedIndex = (selectedIndex < menuItems.size() - 1) ? selectedIndex + 1 : 0;
                draw();
                break;
            case InputDriver.KEY_ENTER:
                return selectedIndex; // Retorna o item selecionado
            default:
                Log.debug(TAG, "Tecla nao mapeada: " + key);
        }
        return -1; // Sem selecao ainda
    }

    /**
     * @brief Hook para o kernel reportar um erro.
     */
    public static void reportKernelError(String code, String description) {
        // Implementacao de "dar erro no codigo"
        lastErrorCode = "ERROR: " + code + " | " + description;
        Log.error(TAG, lastErrorCode);
    }
}
