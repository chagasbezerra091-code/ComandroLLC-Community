#ifndef COMANDRO_ART_FACTORY_RESET_WIPE_PROCEDURE_H
#define COMANDRO_ART_FACTORY_RESET_WIPE_PROCEDURE_H

#include <comandro/kernel/partition_manager.h>
#include <comandro/kernel/display_driver.h>
#include <comandro/kernel/log.h>
#include <string>
#include <vector>

namespace comandro {
namespace art {
namespace factoryreset {

/**
 * @brief Gerencia o procedimento seguro e irreversivel de exclusao de dados.
 * * Esta rotina deve ser executada apenas em modos especiais (Recovery, Fastboot, etc.).
 */
class WipeProcedure {
public:
    WipeProcedure();
    
    /**
     * @brief Inicia o processo de restauracao de fabrica (exclusao de dados do usuario).
     * @return true se o processo foi concluido com sucesso.
     */
    bool startFactoryWipe();

private:
    // Lista de particoes que devem ser apagadas
    const std::vector<std::string> USER_PARTITIONS = {
        "userdata", 
        "cache", 
        "metadata",
        "sdcard_emulated"
    };

    /**
     * @brief Exibe a mensagem de aviso na tela de baixo nivel.
     */
    void displayErasingMessage();

    /**
     * @brief Realiza a exclusao segura de uma unica particao.
     * @param partitionName Nome da particao a ser excluida.
     * @return true se a exclusao foi bem-sucedida.
     */
    bool wipeSinglePartition(const std::string& partitionName);
};

} // namespace factoryreset
} // namespace art
} // namespace comandro

#endif // COMANDRO_ART_FACTORY_RESET_WIPE_PROCEDURE_H
