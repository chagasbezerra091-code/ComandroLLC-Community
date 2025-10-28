package comandro.kernel.core.packages.drivers;

import comandro.settings.debug.ComandroDebug;
import java.util.HashMap;
import java.util.Map;

/**
 * DriverManager: Gerenciador Central de Módulos e Drivers.
 *
 * Responsável por carregar e inicializar dinamicamente drivers nativos (C/C++/Rust)
 * no kernel-core. Essencial para drivers modulares e para gerenciar recursos de hardware.
 */
public final class DriverManager {

    private static final Map<String, DriverStatus> loadedDrivers = new HashMap<>();

    // Status de um Driver
    public enum DriverStatus {
        UNLOADED, 
        LOADING, 
        ACTIVE, 
        FAILED 
    }
    
    // Método nativo que faz o carregamento real do código nativo (dlopen/System.loadLibrary)
    private static native boolean native_load_library(String driverName);
    
    // Método nativo que busca e chama a função de inicialização (registerDriver) do módulo
    private static native int native_call_init_function(String driverName, long initAddressPtr);

    private DriverManager() {
        // Classe estática
    }

    /**
     * @brief Tenta carregar e inicializar um driver nativo.
     * @param driverName O nome do driver (ex: "touchscreen", "gpu_vulkan").
     * @param initAddressPtr O endereço de memória do buffer inicial ou recurso de I/O.
     * @return O status final da inicialização.
     */
    public static DriverStatus loadDriver(String driverName, long initAddressPtr) {
        if (loadedDrivers.containsKey(driverName) && loadedDrivers.get(driverName) == DriverStatus.ACTIVE) {
            ComandroDebug.w("DRIVERS", "Driver " + driverName + " ja esta ativo.");
            return DriverStatus.ACTIVE;
        }

        loadedDrivers.put(driverName, DriverStatus.LOADING);
        
        // Passo 1: Carregar o código binário (ex: .so)
        if (!native_load_library(driverName)) {
            ComandroDebug.e("DRIVERS", "Falha critica ao carregar binario do driver " + driverName);
            loadedDrivers.put(driverName, DriverStatus.FAILED);
            return DriverStatus.FAILED;
        }

        // Passo 2: Chamar a função de inicialização (Ex: registerDriver do Rust)
        int initResult = native_call_init_function(driverName, initAddressPtr);

        if (initResult == 0) {
            loadedDrivers.put(driverName, DriverStatus.ACTIVE);
            ComandroDebug.i("DRIVERS", "Driver " + driverName + " inicializado com sucesso.");
            return DriverStatus.ACTIVE;
        } else {
            ComandroDebug.e("DRIVERS", "Funcao init do driver " + driverName + " retornou codigo de erro: " + initResult);
            loadedDrivers.put(driverName, DriverStatus.FAILED);
            return DriverStatus.FAILED;
        }
    }
    
    /**
     * @brief Retorna o status atual de um driver.
     */
    public static DriverStatus getStatus(String driverName) {
        return loadedDrivers.getOrDefault(driverName, DriverStatus.UNLOADED);
    }
}
