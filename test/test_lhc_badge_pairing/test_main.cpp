#include "modules/LHCBadgePairing.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_pairing_indicators_match_configured_mode()
{
    using lhc_badge::PairingEvent;
    using lhc_badge::PairingIndicator;

    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::RGB_CODE),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(
                              meshtastic_Config_BluetoothConfig_PairingMode_RANDOM_PIN, PairingEvent::PASSKEY)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::FIXED_PIN),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(meshtastic_Config_BluetoothConfig_PairingMode_FIXED_PIN,
                                                                          PairingEvent::PASSKEY)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::NO_PIN),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(meshtastic_Config_BluetoothConfig_PairingMode_NO_PIN,
                                                                          PairingEvent::CONNECTED)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::FIXED_PIN),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(
                              static_cast<meshtastic_Config_BluetoothConfig_PairingMode>(99), PairingEvent::PASSKEY)));
}

void test_pairing_indicators_clear_for_other_events()
{
    using lhc_badge::PairingEvent;
    using lhc_badge::PairingIndicator;

    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::NONE),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(
                              meshtastic_Config_BluetoothConfig_PairingMode_RANDOM_PIN, PairingEvent::CONNECTED)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::NONE),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(meshtastic_Config_BluetoothConfig_PairingMode_FIXED_PIN,
                                                                          PairingEvent::CONNECTED)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::NONE),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(meshtastic_Config_BluetoothConfig_PairingMode_NO_PIN,
                                                                          PairingEvent::PASSKEY)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingIndicator::NONE),
                          static_cast<int>(lhc_badge::pairingIndicatorFor(meshtastic_Config_BluetoothConfig_PairingMode_NO_PIN,
                                                                          PairingEvent::DISCONNECTED)));
}

void test_pairing_mode_colors_are_saturated()
{
    TEST_ASSERT_EQUAL_UINT8(255, lhc_badge::FIXED_PIN_COLOR.red);
    TEST_ASSERT_EQUAL_UINT8(0, lhc_badge::FIXED_PIN_COLOR.green);
    TEST_ASSERT_EQUAL_UINT8(255, lhc_badge::FIXED_PIN_COLOR.blue);
    TEST_ASSERT_EQUAL_UINT8(0, lhc_badge::NO_PIN_COLOR.red);
    TEST_ASSERT_EQUAL_UINT8(255, lhc_badge::NO_PIN_COLOR.green);
    TEST_ASSERT_EQUAL_UINT8(255, lhc_badge::NO_PIN_COLOR.blue);
}

void test_rgb_pairing_pin_contains_only_encoded_digits()
{
    const uint8_t sequence[6] = {0, 1, 2, 3, 4, 5};
    const uint8_t blue[6] = {2, 2, 2, 2, 2, 2};
    const uint8_t wrapped[6] = {255, 255, 255, 255, 255, 255};

    TEST_ASSERT_EQUAL_UINT32(456456, lhc_badge::pairingPinFromRandomBytes(sequence));
    TEST_ASSERT_EQUAL_UINT32(666666, lhc_badge::pairingPinFromRandomBytes(blue));
    TEST_ASSERT_EQUAL_UINT32(444444, lhc_badge::pairingPinFromRandomBytes(wrapped));
}

void test_pairing_pin_format_preserves_leading_zeroes()
{
    char output[7];

    lhc_badge::formatPairingPin(456, output);
    TEST_ASSERT_EQUAL_STRING("000456", output);
    lhc_badge::formatPairingPin(123456, output);
    TEST_ASSERT_EQUAL_STRING("123456", output);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_pairing_indicators_match_configured_mode);
    RUN_TEST(test_pairing_indicators_clear_for_other_events);
    RUN_TEST(test_pairing_mode_colors_are_saturated);
    RUN_TEST(test_rgb_pairing_pin_contains_only_encoded_digits);
    RUN_TEST(test_pairing_pin_format_preserves_leading_zeroes);
    return UNITY_END();
}
