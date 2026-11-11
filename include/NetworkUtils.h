#pragma once

#include <WiFi.h>

#if defined(ARDUINO_WT32_ETH01)
#include <ETH.h>
#endif

namespace RidenDongle
{

inline bool ethernetConnected()
{
#if defined(ARDUINO_WT32_ETH01)
    return ETH.hasIP();
#else
    return false;
#endif
}

inline bool wifiAccessPointActive()
{
    return (WiFi.getMode() & WIFI_MODE_AP) != 0;
}

inline IPAddress preferredNetworkIP()
{
#if defined(ARDUINO_WT32_ETH01)
    if (ethernetConnected()) {
        return ETH.localIP();
    }
#endif
    return WiFi.localIP();
}

inline String preferredNetworkMacAddress()
{
#if defined(ARDUINO_WT32_ETH01)
    if (ethernetConnected()) {
        return ETH.macAddress();
    }
#endif
    return WiFi.macAddress();
}

} // namespace RidenDongle
