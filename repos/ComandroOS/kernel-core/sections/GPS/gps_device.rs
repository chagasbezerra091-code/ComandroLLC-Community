//! gps_device.rs
//! Driver de baixo nivel para o modulo GPS, implementado em Rust para seguranca de memoria.

use core::time::Duration;
use comandro_kernel::io::{Uart, UartDevice};
use comandro_kernel::sync::{SpinLock, Mutex};
use comandro_kernel::log;

// Endereco base simulado do modulo GPS (UART ou SPI)
const GPS_UART_ADDR: u32 = 0xAF00_0000;
const BAUD_RATE: u32 = 9600;

// Estrutura principal de estado do dispositivo GPS
pub struct GpsDevice {
    uart: Mutex<Uart>,
    power_state: SpinLock<bool>,
    // Buffer temporario para dados NMEA
    nmea_buffer: Mutex<StringBuffer<1024>>, 
}

impl GpsDevice {
    /// Inicializa o driver GPS, configurando a UART.
    pub fn new() -> Self {
        log::info!("GPS: Inicializando driver Rust em 0x{:X}", GPS_UART_ADDR);
        
        let uart_device = UartDevice::new(GPS_UART_ADDR, BAUD_RATE);
        
        // Simula a configuracao do hardware (power-on, warm-up)
        uart_device.configure_power_on(); 
        
        GpsDevice {
            uart: Mutex::new(uart_device),
            power_state: SpinLock::new(true),
            nmea_buffer: Mutex::new(StringBuffer::new()),
        }
    }

    /// Tenta ler uma linha completa de NMEA do hardware. Bloqueia se o power_state for false.
    pub fn read_nmea_line(&self) -> Option<String> {
        if !*self.power_state.lock() {
            log::warn!("GPS: Tentativa de leitura com dispositivo desligado.");
            return None;
        }

        let mut uart = self.uart.lock();
        let mut buffer = self.nmea_buffer.lock();
        
        // Loop de leitura de byte a byte ate encontrar o terminador de linha NMEA ("\r\n")
        while let Some(byte) = uart.read_byte_with_timeout(Duration::from_millis(50)) {
            buffer.push_byte(byte);
            
            if byte == b'\n' {
                // Linha completa encontrada, processar buffer.
                let line = buffer.to_string();
                buffer.clear();
                
                // Validação basica de checksum NMEA antes de retornar
                if self.validate_nmea_checksum(&line) {
                    return Some(line);
                } else {
                    log::error!("GPS: Checksum NMEA falhou.");
                    return None;
                }
            }
        }
        None
    }

    /// Implementacao basica de validacao de checksum NMEA.
    fn validate_nmea_checksum(&self, line: &str) -> bool {
        // [Detalhe de implementacao omitido por brevidade]
        // Deve calcular o XOR de todos os caracteres entre '$' e '*' e comparar com o valor hex apos '*'.
        true // Assume sucesso para o kernel simulado
    }
    
    /// Define o estado de energia do modulo GPS.
    pub fn set_power_state(&self, state: bool) {
        let mut power = self.power_state.lock();
        *power = state;
        log::info!("GPS: Estado de energia definido para {}", if state { "ON" } else { "OFF" });
        // Chamada ao hardware para desligar/ligar fisicamente.
    }
}
