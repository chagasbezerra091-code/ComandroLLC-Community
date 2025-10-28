// src/collector.rs
use core::sync::atomic::{AtomicBool, Ordering};
use core::time::Duration;
use comandro_kernel_ffi::scheduler::{self, Priority};
use crate::heap::Heap;

pub enum GcPhase {
    Idle,
    Marking,
    Sweeping,
    WaitingForAllocation,
}

pub struct Collector {
    state: GcPhase,
    collection_needed: AtomicBool,
}

impl Collector {
    /// O singleton do Collector.
    pub fn global() -> &'static Self {
        // Implementacao segura de Lazy Static (padrao Rust)
        // ...
        // Simplificado:
        static COLLECTOR: Collector = Collector {
            state: GcPhase::Idle,
            collection_needed: AtomicBool::new(false),
        };
        &COLLECTOR
    }

    /// @brief Inicia a thread de baixa prioridade do GC.
    pub fn start_thread() -> Result<(), ()> {
        // API do Kernel para criar uma thread de baixa prioridade
        scheduler::create_kernel_thread("comandro_rtgc", Priority::VERY_LOW, Self::collector_thread_loop)
    }

    /// @brief O loop principal da thread do GC.
    fn collector_thread_loop() {
        scheduler::log_kernel_info("RTGC: Collector Loop iniciado.");
        
        loop {
            // 1. Espera por trabalho (ou dorme)
            if !Self::global().collection_needed.load(Ordering::Relaxed) {
                // Dorme, cedendo o controle ao kernel para threads de prioridade mais alta
                scheduler::sleep(Duration::from_millis(10)); 
                continue;
            }

            // 2. Ciclo Cooperativo (Mark & Sweep Incremental)
            Self::global().state = GcPhase::Marking;
            Self::mark_incremental();
            
            Self::global().state = GcPhase::Sweeping;
            Self::sweep_incremental();
            
            Self::global().collection_needed.store(false, Ordering::Relaxed);
            Self::global().state = GcPhase::Idle;
        }
    }

    /// @brief Executa a fase de marcação em pequenos incrementos.
    fn mark_incremental() {
        let max_time = Duration::from_micros(400); // MAX PAUSE TIME (400us)
        
        // Simulação de iterador de objetos raiz
        let mut root_iterator = Heap::global().get_root_iterator();
        
        loop {
            let start_time = scheduler::get_current_time_ns();

            // Marca uma pequena quantidade de objetos
            if root_iterator.mark_next_chunk() {
                // Se a marcação foi concluída
                return;
            }
            
            let elapsed_ns = scheduler::get_current_time_ns() - start_time;
            
            // Verifica o limite de latência de 400us
            if elapsed_ns > max_time.as_nanos() {
                // Cede o controle imediatamente ao Scheduler
                scheduler::log_kernel_info("RTGC Mark excedeu 400us. Yielding.");
                scheduler::yield_to_higher_prio();
            }
        }
    }
    
    /// @brief Executa a fase de varredura em pequenos incrementos.
    fn sweep_incremental() {
        // Logica similar a mark_incremental, liberando memoria gradualmente.
        scheduler::yield_to_higher_prio(); 
        // ...
    }

    /// @brief Força uma coleta completa e bloqueia (usado como fallback de alocacao critica).
    pub fn force_collection_and_wait(&self, max_wait: Duration) {
        scheduler::log_kernel_warn("RTGC: Coleta forcada por alocacao.");
        self.collection_needed.store(true, Ordering::Relaxed);
        
        // Bloqueia a thread atual por no maximo 5ms, permitindo que o GC termine.
        scheduler::wait_for_condition(max_wait, || self.state == GcPhase::Idle);
    }
    
    pub fn trigger_cycle_if_needed(&self) {
        if Heap::global().is_heap_full() {
             self.collection_needed.store(true, Ordering::Relaxed);
        }
    }
}
