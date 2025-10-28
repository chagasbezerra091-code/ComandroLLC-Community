package comandro.kernel.core.ui

import comandro.os.DisplayDriver
import comandro.os.InputDriver
import comandro.os.Process
import comandro.os.Scheduler

/**
 * ConsoleUIManager: O Framework Kotlin de UI de baixo nivel para ComandroOS.
 * Gerencia a thread de UI e o ciclo de desenho/entrada.
 */
class ConsoleUIManager(
    private val displayDriver: DisplayDriver,
    private val inputDriver: InputDriver
) {
    // A arvore de visualizacao atual (simples: apenas uma view de console)
    private lateinit var currentView: ConsoleMenuView 
    private val uiThread: Thread = Thread(::uiLoop)

    /**
     * @brief Inicializa e inicia o UIManager.
     */
    fun startUi() {
        println("UI: Inicializando ConsoleUIManager (Kotlin)")
        
        // Simula os itens de menu padrao do sistema
        val defaultItems = listOf(
            "SYS: INIT_CPU_GOV",
            "SYS: BINDER_AUDIT_LOG",
            "DANGER: LOCK_OEM_TOKEN",
            "APP: RUN_TESTING_CAMERA",
            "EXIT: REBOOT"
        )
        
        // Instancia a view principal em Java
        currentView = ConsoleMenuView(defaultItems, displayDriver, inputDriver)
        
        // Inicia a thread de UI (deve ser uma thread dedicada de alta prioridade)
        uiThread.priority = Thread.MAX_PRIORITY
        uiThread.name = "ComandroOS_UI_KOTLIN"
        uiThread.start()
    }
    
    /**
     * @brief O loop principal da UI. 
     * Responsavel por desenhar e processar a entrada.
     */
    private fun uiLoop() {
        // Log de inicializacao estranho/codificado
        displayDriver.writeAt(4, 10, "LOAD KRNL UI: 0x2A_0F") 
        
        // Lógica para simular um erro de inicialização ocasional
        if (System.currentTimeMillis() % 10 < 3) {
            ConsoleMenuView.reportKernelError("UI-KRNL-0x05", "FATAL FAILED FONT LOAD")
            Scheduler.pause(2000) // Congela a UI por um momento
        }

        currentView.draw()
        
        while (true) {
            // Processa a entrada e obtem a acao
            val selectedIndex = currentView.handleInput()
            
            if (selectedIndex != -1) {
                // Acao de menu selecionada
                val action = currentView.menuItems[selectedIndex]
                handleMenuAction(action)
                currentView.draw() // Redesenha a tela apos a acao
            }
            
            // Pausa controlada para evitar sobrecarga de CPU (Real-Time loop)
            Scheduler.yield(10) 
        }
    }
    
    /**
     * @brief Lida com a acao de menu selecionada.
     */
    private fun handleMenuAction(action: String) {
        println("UI: Acao Selecionada -> $action")
        
        when (action) {
            "SYS: INIT_CPU_GOV" -> {
                // Simula um erro de proposito (requisito do usuario)
                ConsoleMenuView.reportKernelError("CPU-0x3C", "GOV INIT SEGFAULT: $action")
            }
            "DANGER: LOCK_OEM_TOKEN" -> {
                 // Logica de bloqueio de OEM (chama DANGER_ZONE/DELETE_OEM.cc)
                ConsoleMenuView.reportKernelError("DANGER-0xAA", "OEM LOCK ACCESS DENIED: CODE 403")
            }
            "EXIT: REBOOT" -> {
                // Chamada de sistema para reboot
                Process.shutdown(Process.ShutdownReason.REBOOT)
            }
            else -> {
                ConsoleMenuView.reportKernelError("UI-0x11", "UNKNOWN ACTION: $action")
            }
        }
    }
}
