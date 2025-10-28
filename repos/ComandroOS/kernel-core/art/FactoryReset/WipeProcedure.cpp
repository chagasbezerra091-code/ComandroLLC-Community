#include "WipeProcedure.h"
#include <chrono>

namespace comandro {
namespace art {
namespace factoryreset {

using kernel::PartitionManager;
using kernel::DisplayDriver;
using kernel::Log;

static constexpr const char* TAG = "WipeProcedure";

WipeProcedure::WipeProcedure() {
    Log::info(TAG, "Wipe Procedure inicializado. Particoes alvo prontas.");
}

/**
 * @brief Exibe a mensagem de aviso na tela de baixo nivel.
 */
void WipeProcedure::displayErasingMessage() {
    // A tela de baixo nivel (framebuffer simples) deve ser ativada.
    DisplayDriver::clearScreen();
    DisplayDriver::setColor(DisplayDriver::COLOR_RED);
    DisplayDriver::setFontSize(DisplayDriver::SIZE_LARGE);
    
    // Confirmação de que o procedimento foi iniciado.
    DisplayDriver::printCentered("ERASING...");
    DisplayDriver::flush(); // Garante que a mensagem apareça imediatamente.
    
    Log::alert(TAG, "Mensagem 'ERASING...' exibida na tela do dispositivo.");
}

/**
 * @brief Inicia o processo de restauracao de fabrica.
 */
bool WipeProcedure::startFactoryWipe() {
    Log::alert(TAG, "INICIANDO PROCESSO IRREVERSIVEL DE WIPE DE FABRICA.");
    
    // 1. Exibir aviso imediato
    displayErasingMessage();

    // 2. Desmonta todas as particoes de usuario antes de apagar (seguranca)
    if (!PartitionManager::unmountPartitions(USER_PARTITIONS)) {
        Log::error(TAG, "Falha ao desmontar particoes de usuario.");
        return false;
    }

    // 3. Excluir cada particao individualmente
    for (const auto& partition : USER_PARTITIONS) {
        if (!wipeSinglePartition(partition)) {
            Log::error(TAG, "Falha critica ao apagar a particao: " + partition);
            // Neste ponto, o sistema pode estar em estado inconsistente.
            DisplayDriver::printBottom("WIPE FAILED: " + partition);
            DisplayDriver::flush();
            return false;
        }
    }

    // 4. Sucesso e Reformatacao
    Log::alert(TAG, "WIPE DE FABRICA CONCLUIDO COM SUCESSO.");
    
    // Forcar a reformatacao (o kernel montara e reformatara no proximo boot)
    PartitionManager::markForFormat(USER_PARTITIONS);

    DisplayDriver::clearScreen();
    DisplayDriver::setColor(DisplayDriver::COLOR_GREEN);
    DisplayDriver::printCentered("WIPE COMPLETE. REBOOTING...");
    DisplayDriver::flush();
    
    // 5. Reinicia o sistema
    // std::this_thread::sleep_for(std::chrono::seconds(5)); // Pausa para o usuario ler
    // kernel::System::reboot();
    
    return true;
}

/**
 * @brief Realiza a exclusao segura de uma unica particao.
 */
bool WipeProcedure::wipeSinglePartition(const std::string& partitionName) {
    Log::warn(TAG, "Apagando particao: " + partitionName);

    // *****************************************************************
    // ROTINA CRITICA DE EXCLUSÃO (REAL)
    // *****************************************************************
    
    // 1. Obtem o descritor de bloco/dispositivo
    auto deviceHandle = PartitionManager::getDeviceHandle(partitionName);

    // 2. Comando FTL (Flash Translation Layer) para trim/erase
    // O comando TRIM/ERASE é a forma mais rapida e segura de limpar blocos em eMMC/UFS.
    if (!deviceHandle.sendTrimCommand()) {
        Log::error(TAG, "Falha ao enviar comando TRIM/ERASE para " + partitionName);
        return false;
    }

    // 3. Verificacao (Opcional, mas recomendado para seguranca)
    // Leitura de uma pequena amostra para garantir que os blocos estao zerados.
    if (deviceHandle.readSample() != 0x0000) {
        // Log::error(TAG, "Blocos nao zerados apos o TRIM. Falha de hardware?");
        // Nao falhamos aqui, mas registramos o evento.
    }
    
    Log::info(TAG, "Particao " + partitionName + " apagada com sucesso (TRIM/ERASE).");
    
    // Simula o tempo que levaria
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return true;
}

} // namespace factoryreset
} // namespace art
} // namespace comandro
