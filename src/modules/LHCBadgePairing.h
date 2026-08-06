#pragma once

#include "mesh/generated/meshtastic/config.pb.h"
#include <cstdint>
#include <cstdio>

namespace lhc_badge
{
enum class PairingEvent : uint8_t {
    PASSKEY,
    CONNECTED,
    DISCONNECTED,
};

enum class PairingIndicator : uint8_t {
    NONE,
    RGB_CODE,
    FIXED_PIN,
    NO_PIN,
};

struct PairingColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

constexpr PairingColor FIXED_PIN_COLOR{255, 0, 255};
constexpr PairingColor NO_PIN_COLOR{0, 255, 255};

constexpr PairingIndicator pairingIndicatorFor(meshtastic_Config_BluetoothConfig_PairingMode mode, PairingEvent event)
{
    if (event == PairingEvent::PASSKEY) {
        switch (mode) {
        case meshtastic_Config_BluetoothConfig_PairingMode_RANDOM_PIN:
            return PairingIndicator::RGB_CODE;
        case meshtastic_Config_BluetoothConfig_PairingMode_NO_PIN:
            return PairingIndicator::NONE;
        case meshtastic_Config_BluetoothConfig_PairingMode_FIXED_PIN:
        default:
            return PairingIndicator::FIXED_PIN;
        }
    }
    if (event == PairingEvent::CONNECTED && mode == meshtastic_Config_BluetoothConfig_PairingMode_NO_PIN) {
        return PairingIndicator::NO_PIN;
    }

    return PairingIndicator::NONE;
}

inline void formatPairingPin(uint32_t passkey, char (&output)[7])
{
    snprintf(output, sizeof(output), "%06u", passkey);
}

constexpr uint32_t pairingPinFromRandomBytes(const uint8_t (&randomBytes)[6])
{
    uint32_t passkey = 0;
    for (uint8_t randomByte : randomBytes) {
        passkey = passkey * 10 + 4 + randomByte % 3;
    }
    return passkey;
}
} // namespace lhc_badge
