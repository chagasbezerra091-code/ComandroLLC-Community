#include "wifi.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/core_hardware_access.h> // Para I/O de baixo nivel (SDIO/PCIe)
#include <comandro/kernel/scheduler.h>
#include <comandro/kernel/ipc/ComandroIpcBus.h>

namespace comandro {
namespace kernel {
namespace radio {

using kernel::Log;
using kernel::CoreHardwareAccess;
using kernel::Scheduler;
using kernel::ipc::ComandroIpcBus;

static constexpr const char* TAG = "WifiManager";
// Endereco de registro de hardware (exemplo SDIO)
static constexpr uint32_t WIFI_CONTROL_REG = 0xA0000000;
static constexpr uint32_t WIFI_DATA_PORT   = 0xA0000004;

WifiManager& WifiManager::instance() {
    static WifiManager s_instance;
    return s_instance;
}

WifiManager::WifiManager() : m_is_initialized(false) {
    // A inicializacao real do hardware ocorre em initializeHardware()
    Log::info(TAG, "WifiManager inicializado (Core).");
}

/**
 * @brief Inicializa o chip Wi-Fi, carrega o firmware e configura o modo HCI.
 */
bool WifiManager::initializeHardware() {
    SpinLock::Guard lock(m_hardware_lock);
    
    if (m_is_initialized) {
        return true;
    }
    
    Log::alert(TAG, "Iniciando hardware Wi-Fi e carregando firmware...");

    // 1. Resetar o Chip e Configurar o Clock
    CoreHardwareAccess::write_reg(WIFI_CONTROL_REG, 0x01); // Soft Reset
    CoreHardwareAccess::write_reg(WIFI_CONTROL_REG, 0x00); // Clear Reset

    // 2. Carregar o Firmware Binario (Low-level I/O)
    if (!CoreHardwareAccess::loadFirmware("wifi_fw.bin")) {
        Log::fatal(TAG, "Falha ao carregar firmware Wi-Fi.");
        return false;
    }

    // 3. Configurar IRQ Handler (tempo real)
    // O IRQ do chip deve ter alta prioridade para processar pacotes rapidamente.
    CoreHardwareAccess::registerIrqHandler(
        CoreHardwareAccess::IRQ_WIFI_CHIP, 
        [this](uint32_t status) { this->handleHardwareIrq(status); }, 
        Scheduler::RT_PRIORITY_HIGH
    );

    m_is_initialized = true;
    Log::info(TAG, "Hardware Wi-Fi pronto. IRQ de alta prioridade configurado.");
    return true;
}

/**
 * @brief Inicia uma busca ativa por redes.
 */
void WifiManager::startScan() {
    SpinLock::Guard lock(m_hardware_lock);
    
    if (!m_is_initialized) {
        Log::error(TAG, "Nao e possivel escanear: Hardware nao inicializado.");
        return;
    }

    Log::info(TAG, "Disparando comando de SCAN de baixo nivel...");
    // Comando real para o registrador/FIFO do chip
    CoreHardwareAccess::write_reg(WIFI_CONTROL_REG, 0x10); // Comando de Scan

    // O resultado sera entregue assincronamente via handleHardwareIrq()
}

/**
 * @brief Inicia o processo de conexao.
 */
bool WifiManager::connect(const MacAddress& bssid, const std::string& passphrase) {
    SpinLock::Guard lock(m_hardware_lock);

    if (!m_is_initialized) {
        return false;
    }
    
    Log::info(TAG, "Tentando conexao com a rede...");

    // 1. Comando de Associacao e Autenticacao (via firmware/HCI)
    // Na pratica, isso empurraria um pacote de comando WLC para o chip.
    if (!CoreHardwareAccess::sendWifiCommand(0x20 /* CMD_ASSOCIATE */, bssid.data(), passphrase.c_str())) {
        Log::error(TAG, "Falha ao enviar comando de associacao.");
        return false;
    }

    // 2. Espera por eventos (conectado, falha, autenticacao) no handleHardwareIrq

    // Esta funcao retorna imediatamente (assincrono). O status final sera notificado ao User Space via C-Bus.
    return true; 
}

/**
 * @brief Handler de Interrupcao de Hardware (IRQ) do chip Wi-Fi.
 * * Executado em contexto de SoftIRQ ou thread de kernel de alta prioridade.
 */
void WifiManager::handleHardwareIrq(uint32_t irq_status) {
    // 1. Le o status da interrupcao para saber o que aconteceu
    // A logica aqui precisa ser extremamente rapida.
    
    if (irq_status & 0x04) { // Bit 2: Dados Recebidos (RX)
        // Le o cabeçalho do pacote RX do FIFO/Data Port
        uint32_t packet_header = CoreHardwareAccess::read_reg(WIFI_DATA_PORT);
        size_t packet_len = packet_header & 0xFFFF;
        
        // Aloca o buffer e le os dados do porto de I/O
        uint8_t* data_buffer = new uint8_t[packet_len];
        CoreHardwareAccess::read_data(WIFI_DATA_PORT, data_buffer, packet_len);
        
        // Envia os dados para o subsistema de rede (IP Stack) via FFI/Kernel Call.
        // Isso deve ser feito em uma thread de kernel de menor prioridade ou por SoftIRQ.
        kernel::network::Ipv4Stack::processPacket(data_buffer, packet_len);
        delete[] data_buffer;

    } else if (irq_status & 0x08) { // Bit 3: Evento de Controle (Scan Complete, Conectado)
        // Le o evento de controle e notifica o User Space.
        std::string event = CoreHardwareAccess::readWifiEvent();
        
        if (event == "SCAN_COMPLETE") {
            Log::info(TAG, "Scan completo. Notificando User Space.");
            // Envia resultados para o SystemServer via C-Bus
            // ComandroIpcBus::instance().sendAsync(SYSTEM_SERVER_NODE, ScanResultsMsg);
        } else if (event == "CONNECTED") {
            Log::alert(TAG, "Conexao Wi-Fi estabelecida.");
            // ComandroIpcBus::instance().sendAsync(SYSTEM_SERVER_NODE, WifiConnectedMsg);
        }
    }
    
    // 2. Limpa o status da interrupcao
    CoreHardwareAccess::write_reg(WIFI_CONTROL_REG, irq_status); 
}


} // namespace radio
} // namespace kernel
} // namespace comandro
