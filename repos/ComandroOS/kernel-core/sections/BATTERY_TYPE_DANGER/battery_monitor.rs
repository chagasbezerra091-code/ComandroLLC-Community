//! battery_monitor.rs
//! Driver Rust para comunicacao segura com o PMIC e leitura de dados da bateria.

use comandro_kernel::io::{I2cController, Register, I2cDevice};
use comandro_kernel::sync::SpinLock;
use comandro_kernel::log;
use core::sync::atomic::{AtomicU16, Ordering};

// Endereco simulado do PMIC na barra I2C
const PMIC_I2C_ADDR: u8 = 0x68;
// Registradores cruciais do PMIC
const REG_VOLTAGE_MV: u8 = 0x01;
const REG_TEMP_DEGC: u8 = 0x02;
const REG_STATUS: u8 = 0x03; // Bateria carregando, descarregando, etc.

// Limites de Segurança (Hardcoded no Kernel para confiabilidade)
const DANGER_TEMP_HIGH_DEGC: u16 = 60; // 60°C
const DANGER_VOLT_LOW_MV: u16 = 3000;  // 3.0V (Risco de descarga profunda)

// Cache Atomica para latencia zero na leitura de temperatura/tensao
static CACHED_TEMP: AtomicU16 = AtomicU16::new(25); 
static CACHED_VOLTAGE: AtomicU16 = AtomicU16::new(4000);

pub struct BatteryMonitor {
    pmic_device: SpinLock<I2cDevice>,
}

impl BatteryMonitor {
    /// Inicializa o driver da bateria.
    pub fn new() -> Self {
        log::info!("BATTERY: Inicializando PMIC driver Rust (I2C: 0x{:X})", PMIC_I2C_ADDR);
        
        let i2c_bus = I2cController::get_primary_bus();
        
        BatteryMonitor {
            pmic_device: SpinLock::new(I2cDevice::new(i2c_bus, PMIC_I2C_ADDR)),
        }
    }

    /// Executa um ciclo de leitura do hardware e atualiza a cache atomica.
    pub fn poll_hardware(&self) {
        let mut pmic = self.pmic_device.lock();
        
        // 1. Leitura de Tensao (milivolts)
        if let Some(volt_mv) = pmic.read_register_u16(REG_VOLTAGE_MV) {
            CACHED_VOLTAGE.store(volt_mv, Ordering::Release);
        }

        // 2. Leitura de Temperatura (graus Celsius)
        if let Some(temp_degc) = pmic.read_register_u16(REG_TEMP_DEGC) {
            CACHED_TEMP.store(temp_degc, Ordering::Release);
        }
    }
    
    /// Leitura atomica da temperatura atual.
    pub fn get_temp_degc() -> u16 {
        CACHED_TEMP.load(Ordering::Acquire)
    }

    /// Leitura atomica da tensao atual.
    pub fn get_voltage_mv() -> u16 {
        CACHED_VOLTAGE.load(Ordering::Acquire)
    }
    
    /// @brief Verifica a bateria contra limites de perigo codificados.
    /// @return true se o sistema estiver em risco imediato e precisar de acao.
    pub fn check_danger_condition() -> bool {
        let temp = Self::get_temp_degc();
        let volt = Self::get_voltage_mv();

        if temp >= DANGER_TEMP_HIGH_DEGC {
            log::critical!("BATTERY DANGER: Temperatura alta (%uC).", temp);
            return true;
        }

        if volt <= DANGER_VOLT_LOW_MV {
            log::critical!("BATTERY DANGER: Tensao critica (%umV).", volt);
            return true;
        }
        
        false
    }
}
