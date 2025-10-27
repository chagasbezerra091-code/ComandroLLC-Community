#ifndef COMANDRO_BATTERY_MANAGER_H
#define COMANDRO_BATTERY_MANAGER_H

#include <string>
#include <iostream>
#include <algorithm> // Necessário para std::max e std::min
#include <cstdlib>   // Necessário para rand()

namespace comandro {
namespace kernel {
namespace power {

/**
 * @brief Gerenciador de Bateria (Implementação C++ de Baixo Nível no Kernel Core).
 * * Responsável por ler o status do hardware e fornecer uma representação 
 * visual minimalista da carga para a linha de comando ou TTY.
 */
class BatteryManager {
public:
    
    // --- Interface de Leitura de Status ---
    
    /**
     * @brief Lê o status atual de carga da bateria do hardware.
     * @return O percentual de carga (0-100), ou -1 se não for reconhecida ou estiver ausente.
     */
    static int getCurrentChargePercent();

    /**
     * @brief Verifica se a bateria está conectada à fonte de alimentação (carregando).
     * @return True se estiver carregando, false caso contrário.
     */
    static bool isCharging();

    // --- Rendering Visual (Interface Minimalista ComandroOS) ---
    
    /**
     * @brief Retorna a representação de "frame" da bateria para o estado atual.
     * @return Uma string minimalista representando o ícone e a porcentagem.
     */
    static std::string getBatteryIconAndStatus() {
        int percent = getCurrentChargePercent();
        
        // Se a bateria não for reconhecida ou estiver ausente
        if (percent < 0) {
            return "[ ? ] Bateria Ausente";
        }

        bool charging = isCharging();
        
        // Determina o número de barras cheias (de 0 a 10)
        int bars = percent / 10;
        
        std::string icon = "[";
        // Usa '+' para indicar carregamento, '#' para carga estável.
        char fillChar = (charging) ? '+' : '#';
        char emptyChar = ' ';
        
        // Constrói o preenchimento da bateria
        for (int i = 0; i < 10; ++i) {
            icon += (i < bars) ? fillChar : emptyChar;
        }
        icon += "]";

        // Adiciona a porcentagem e o status de carregamento
        std::string status = icon + " " + std::to_string(percent) + "%";
        if (charging) {
            status += " (Carregando)";
        } else if (percent < 15) {
            status += " (LOW!)";
        }

        return status;
    }

private:
    // --- Funções Nativas (Chamadas ao Hardware) ---
    static int native_read_charge_percent();
    static bool native_read_charging_status();
};

} // namespace power
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_BATTERY_MANAGER_H
