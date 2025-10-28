#ifndef COMANDRO_KERNEL_RADIO_WIFI_H
#define COMANDRO_KERNEL_RADIO_WIFI_H

#include <comandro/kernel/types.h>
#include <comandro/kernel/thread.h>
#include <comandro/kernel/spinlock.h>
#include <string>
#include <array>
#include <vector>

namespace comandro {
namespace kernel {
namespace radio {

// Tipos de dados essenciais
using MacAddress = std::array<uint8_t, 6>;

struct NetworkScanResult {
    MacAddress bssid;
    std::string ssid;
    int8_t rssi_dbm;
    uint8_t channel;
};

// Interface do Manager (simplificada para o .cc)
class WifiManager {
public:
    static WifiManager& instance();
    bool initializeHardware();
    void startScan();
    bool connect(const MacAddress& bssid, const std::string& passphrase);
    void handleHardwareIrq(uint32_t irq_status);

private:
    WifiManager();
    volatile bool m_is_initialized;
    SpinLock m_hardware_lock;
};

} // namespace radio
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_RADIO_WIFI_H
