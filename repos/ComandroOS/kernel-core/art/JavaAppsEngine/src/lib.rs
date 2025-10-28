// src/lib.rs
#![no_std] // Sem a biblioteca padrao do Rust para ambiente de kernel
#![allow(non_snake_case)]

extern crate alloc; // Usando o alocador global do ComandroOS

use core::time::Duration;
use crate::collector::{Collector, GcPhase};
use crate::heap::Heap;
use comandro_kernel_ffi::scheduler::{self, Priority};

mod collector;
mod heap;
mod object;

// --- Definicoes FFI para interacao com o Framework Java/Kotlin ---

/// @brief Inicializa o Real-Time Garbage Collector e seus threads.
#[no_mangle]
pub extern "C" fn ComandroRTGC_init() -> bool {
    // 1. Inicializa o heap
    Heap::init(32 * 1024 * 1024); // Heap inicial de 32MB

    // 2. Cria e inicia a thread do Collector (baixa prioridade)
    match Collector::start_thread() {
        Ok(_) => {
            scheduler::log_kernel_info("RTGC Thread iniciada com sucesso.");
            true
        },
        Err(e) => {
            scheduler::log_kernel_error(&format!("Falha ao iniciar RTGC thread: {:?}", e));
            false
        }
    }
}

/// @brief Chamado pela thread de aplicacao para alocar um novo objeto.
/// @param size_bytes Tamanho do objeto em bytes.
/// @param is_critical Indica se a alocacao e de uma thread de tempo real (evita bloqueio).
/// @return Um ponteiro para o objeto alocado.
#[no_mangle]
pub extern "C" fn ComandroRTGC_allocate(size_bytes: usize, is_critical: bool) -> *mut u8 {
    let result = Heap::global().allocate(size_bytes);

    if result.is_null() {
        if is_critical {
            // Se for critico e falhar, reportar erro fatal ao kernel (evita deadlock)
            scheduler::log_kernel_critical("RTGC: Falha na alocacao critica. Heap Vazia.");
            return core::ptr::null_mut();
        }
        
        // Bloqueia e força uma coleta antes de tentar novamente (bloqueio maximo garantido)
        Collector::global().force_collection_and_wait(Duration::from_millis(5)); 
        return Heap::global().allocate(size_bytes);
    }
    result
}

/// @brief Forca um ciclo de coleta cooperativa (usado pelo Framework em momentos de inatividade).
#[no_mangle]
pub extern "C" fn ComandroRTGC_request_cooperative_cycle() {
    Collector::global().trigger_cycle_if_needed();
}

