#include "BatteryManager.h"

#include <time.h> 

namespace comandro {
namespace kernel {
namespace power {

// O estado de carga simulado será armazenado aqui (para persistir entre chamadas simuladas)
static int simulated_charge_state = 75;
static bool initialized = false;

// ------------------------------------------------------------------
// IMPLEMENTAÇÃO DAS FUNÇÕES NATIVAS (Simulação do Acesso ao Hardware)
// ------------------------------------------------------------------

/**
 * @brief Simula a leitura do status de carregamento.
 */
bool BatteryManager::native_read_charging_status() {
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }
    // Simula que está conectado à energia 60% do tempo.
    return (rand() % 100) < 60; 
}


/**
 * @brief Simula a leitura do percentual de carga do hardware.
 */
int BatteryManager::native_read_charge_percent() {
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }

    // 1. Simula a remoção ou falha na leitura da bateria (10% de chance)
    if (rand() % 10 == 0) {
        return -1; // -1 significa falha ou ausência
    }

    // 2. Simula o ciclo de carga/descarga
    if (!native_read_charging_status()) {
        // Descarregando (máximo de 2% por leitura simulada)
        simulated_charge_state = std::max(0, simulated_charge_state - (rand() % 3));
    } else {
        // Carregando (máximo de 3% por leitura simulada)
        simulated_charge_state = std::min(100, simulated_charge_state + (rand() % 4));
    }
    
    return simulated_charge_state;
}


// ------------------------------------------------------------------
// IMPLEMENTAÇÃO DA INTERFACE PÚBLICA (API C++)
// ------------------------------------------------------------------

int BatteryManager::getCurrentChargePercent() {
    return native_read_charge_percent();
}

bool BatteryManager::isCharging() {
    return native_read_charging_status();
}

} // namespace power
} // namespace kernel
} // namespace comandro
