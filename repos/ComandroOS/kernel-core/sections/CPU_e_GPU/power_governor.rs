//! power_governor.rs
//! Governor de Frequencia e Potencia da CPU/GPU (Rust)
//! Responsavel por tomar decisoes de clocking com base na carga e latencia.

use comandro_kernel::cpu::{CoreId, CpuMonitor, Policy};
use comandro_kernel::gpu::{GpuMonitor, GpuPolicy};
use comandro_kernel::time::Duration;
use comandro_kernel::log;
use core::sync::atomic::{AtomicU32, Ordering};

// Frequencia alvo atual (em MHz) para o cluster de desempenho (Big Cores)
static BIG_CORE_FREQ_TARGET: AtomicU32 = AtomicU32::new(0);
// Frequencia alvo atual (em MHz) para a GPU
static GPU_FREQ_TARGET: AtomicU32 = AtomicU32::new(0);

// Constantes de Limiar de Carga para o Governor (Hyper-Low Latency Profile)
const THRESHOLD_HIGH_LOAD: u8 = 85; 
const THRESHOLD_LOW_LOAD: u8 = 30; 
const MIN_LATENCY_FREQ_MHZ: u32 = 1200; // Frequencia minima para tarefas criticas

pub struct PowerGovernor;

impl PowerGovernor {
    /// Inicializa os governors e define as frequencias base.
    pub fn init() {
        log::info!("PowerGovernor: Inicializando subsistemas de frequencia.");
        
        // Define as frequencias iniciais de acordo com o perfil de energia.
        BIG_CORE_FREQ_TARGET.store(MIN_LATENCY_FREQ_MHZ, Ordering::Relaxed);
        GPU_FREQ_TARGET.store(500, Ordering::Relaxed);
        
        // Aplica as frequencias iniciais (simulacao de escrita de registro de hardware)
        CpuMonitor::set_frequency_policy(Policy::HighPerformance, MIN_LATENCY_FREQ_MHZ);
        GpuMonitor::set_frequency_policy(GpuPolicy::Balanced, 500);
    }
    
    /// Algoritmo principal de tomada de decisao do Governor, chamado periodicamente.
    /// @param current_load_avg Carga media atual de todos os nucleos de desempenho (0-100%).
    pub fn run_governor_cycle(current_load_avg: u8) {
        let current_freq = BIG_CORE_FREQ_TARGET.load(Ordering::Acquire);
        
        // --- 1. Logica de CPU ---
        if current_load_avg > THRESHOLD_HIGH_LOAD {
            // Carga alta: Aumentar a frequencia para o maximo ou proximo nivel.
            let new_freq = CpuMonitor::get_next_higher_frequency(current_freq);
            Self::set_cpu_frequency(new_freq);
            log::trace!("CPU Gov: Carga alta (%u%%). Subindo para %u MHz", current_load_avg, new_freq);

        } else if current_load_avg < THRESHOLD_LOW_LOAD {
            // Carga baixa: Reduzir a frequencia, mas nunca abaixo do minimo de latencia.
            let target_low_freq = CpuMonitor::get_next_lower_frequency(current_freq);
            let new_freq = target_low_freq.max(MIN_LATENCY_FREQ_MHZ);
            Self::set_cpu_frequency(new_freq);
            log::trace!("CPU Gov: Carga baixa (%u%%). Descendo para %u MHz", current_load_avg, new_freq);
        }
        
        // --- 2. Logica de GPU ---
        // A logica da GPU e separada, geralmente acionada por chamadas de renderizacao.
        // Apenas mantemos o log de GPU aqui.
        let gpu_load = GpuMonitor::get_load();
        if gpu_load > THRESHOLD_HIGH_LOAD {
             // log::trace!("GPU Gov: Carga alta (%u%%). Aumentando GPU Clock.", gpu_load);
        }
    }
    
    /// Funcao interna para aplicar e atualizar o estado atomico da CPU.
    fn set_cpu_frequency(new_freq: u32) {
        if new_freq != BIG_CORE_FREQ_TARGET.load(Ordering::Acquire) {
            CpuMonitor::set_frequency_policy(Policy::HighPerformance, new_freq);
            BIG_CORE_FREQ_TARGET.store(new_freq, Ordering::Release);
        }
    }

    /// Funcao publica para ser chamada pela API de kernel.
    pub fn get_current_cpu_freq() -> u32 {
        BIG_CORE_FREQ_TARGET.load(Ordering::Acquire)
    }

    /// Funcao publica para ser chamada pela API de kernel.
    pub fn get_current_gpu_freq() -> u32 {
        GPU_FREQ_TARGET.load(Ordering::Acquire)
    }
}
