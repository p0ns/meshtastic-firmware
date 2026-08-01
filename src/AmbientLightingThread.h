#ifndef AMBIENTLIGHTINGTHREAD_H
#define AMBIENTLIGHTINGTHREAD_H

#include "Observer.h"
#include "configuration.h"
#include "detect/ScanI2C.h"
#include "sleep.h"

#ifdef HAS_NCP5623
#include <NCP5623.h>
#include <Wire.h>
#endif

#ifdef HAS_LP5562
#include <graphics/NomadStarLED.h>
#endif

#ifdef HAS_NEOPIXEL
#ifdef HAS_NEOPIXEL_ANIMATION
#include "NodeDB.h"
#include <WS2812FX.h>
#include <string>
// WS2812FX exposes generic color macros that collide with Meshtastic display
// enums.
#undef BLACK
#undef WHITE
#undef RED
#undef GREEN
#undef BLUE
#undef YELLOW
#undef CYAN
#undef MAGENTA
#undef PURPLE
#undef ORANGE
#undef PINK
#undef GRAY
#undef ULTRAWHITE
#else
#include <Adafruit_NeoPixel.h>
#endif
#endif

#ifdef UNPHONE
#include "unPhone.h"
extern unPhone unphone;
#endif

class AmbientLightingThread : public concurrency::OSThread
{
    friend class StatusLEDModule;
    friend class ExternalNotificationModule;

  private:
#ifdef HAS_NCP5623
    NCP5623 rgb;
#endif

#ifdef HAS_LP5562
    LP5562 rgbw;
#endif

#ifdef HAS_NEOPIXEL
#ifdef HAS_NEOPIXEL_ANIMATION
    WS2812FX pixels = WS2812FX(NEOPIXEL_COUNT, NEOPIXEL_DATA, NEOPIXEL_TYPE);

    enum class OverrideMode : uint8_t{
        NONE,
        ALERT,
        PAIRING,
    };

    static constexpr uint8_t EFFECT_COUNT = 71;
    static constexpr uint32_t ALERT_DURATION_MS = 5000;
    static constexpr uint32_t PAIRING_DURATION_MS = 30000;

    OverrideMode overrideMode = OverrideMode::NONE;
    uint32_t overrideStartedAt = 0;
#else
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_DATA, NEOPIXEL_TYPE);
#endif
#endif

  public:
    explicit AmbientLightingThread(ScanI2C::DeviceType type) : OSThread("AmbientLighting")
    {
        notifyDeepSleepObserver.observe(&notifyDeepSleep);

#ifdef HAS_RGB_LED
#ifdef ENABLE_AMBIENTLIGHTING
        moduleConfig.ambient_lighting.led_state = true;
#endif
#endif
#if AMBIENT_LIGHTING_TEST
        moduleConfig.ambient_lighting.led_state = true;
        moduleConfig.ambient_lighting.current = 10;
        moduleConfig.ambient_lighting.red = (myNodeInfo.my_node_num & 0xFF0000) >> 16;
        moduleConfig.ambient_lighting.green = (myNodeInfo.my_node_num & 0x00FF00) >> 8;
        moduleConfig.ambient_lighting.blue = myNodeInfo.my_node_num & 0x0000FF;
#endif
#if defined(HAS_NCP5623) || defined(HAS_LP5562)
        _type = type;
        if (_type == ScanI2C::DeviceType::NONE) {
            LOG_DEBUG("AmbientLighting Disable due to no RGB leds found on I2C bus");
            disable();
            return;
        }
#endif
#ifdef HAS_RGB_LED
        LOG_DEBUG("AmbientLighting init");
#ifdef HAS_NCP5623
        if (_type == ScanI2C::NCP5623) {
#endif
#ifdef HAS_LP5562
            if (_type == ScanI2C::LP5562) {
#endif
#ifdef RGBLED_RED
                pinMode(RGBLED_RED, OUTPUT);
                pinMode(RGBLED_GREEN, OUTPUT);
                pinMode(RGBLED_BLUE, OUTPUT);
#endif
#ifdef HAS_NEOPIXEL
#ifdef HAS_NEOPIXEL_ANIMATION
                pixels.init();
                applyPersistedLighting();
#else
                pixels.begin();
                pixels.clear();
                pixels.setBrightness(moduleConfig.ambient_lighting.current);
#endif
#endif
#ifndef HAS_NEOPIXEL_ANIMATION
                if (!moduleConfig.ambient_lighting.led_state) {
                    LOG_DEBUG("AmbientLighting Disable due to "
                              "moduleConfig.ambient_lighting.led_state OFF");
                    disable();
                    return;
                }
                setLighting(moduleConfig.ambient_lighting.current, moduleConfig.ambient_lighting.red,
                            moduleConfig.ambient_lighting.green, moduleConfig.ambient_lighting.blue);
#endif
#if defined(HAS_NCP5623) || defined(HAS_LP5562)
            }
#endif
#endif
        }

#ifdef HAS_NEOPIXEL_ANIMATION
        static constexpr uint8_t animationEffectCount()
        {
            return EFFECT_COUNT;
        }

        bool setEffect(int effect)
        {
            if (effect < 0 || effect >= EFFECT_COUNT) {
                return false;
            }
            moduleConfig.ambient_lighting.animation = static_cast<uint8_t>(effect);
            persistSettings();
            applyIfNotOverridden();
            return true;
        }

        void setBrightness(uint8_t brightness)
        {
            moduleConfig.ambient_lighting.brightness = brightness;
            persistSettings();
            applyIfNotOverridden();
        }

        bool setSpeed(int speed)
        {
            if (speed < 1 || speed > 10000) {
                return false;
            }
            moduleConfig.ambient_lighting.speed = static_cast<uint16_t>(speed);
            persistSettings();
            applyIfNotOverridden();
            return true;
        }

        void setColor(uint8_t red, uint8_t green, uint8_t blue)
        {
            moduleConfig.ambient_lighting.red = red;
            moduleConfig.ambient_lighting.green = green;
            moduleConfig.ambient_lighting.blue = blue;
            persistSettings();
            applyIfNotOverridden();
        }

        void setEnabled(bool enabled)
        {
            moduleConfig.ambient_lighting.led_state = enabled;
            if (enabled && moduleConfig.ambient_lighting.current == 0) {
                moduleConfig.ambient_lighting.current = 10;
            } else if (!enabled) {
                moduleConfig.ambient_lighting.current = 0;
            }
            persistSettings();
            applyIfNotOverridden();
        }

        bool isEnabled() const
        {
            return moduleConfig.ambient_lighting.led_state && moduleConfig.ambient_lighting.current > 0;
        }

        uint8_t nextEffect()
        {
            const uint8_t effect = (moduleConfig.ambient_lighting.animation + 1) % EFFECT_COUNT;
            setEffect(effect);
            return effect;
        }

        uint8_t previousEffect()
        {
            const uint8_t effect = (moduleConfig.ambient_lighting.animation + EFFECT_COUNT - 1) % EFFECT_COUNT;
            setEffect(effect);
            return effect;
        }

        void showAlert()
        {
            if (overrideMode != OverrideMode::NONE) {
                return;
            }
            overrideMode = OverrideMode::ALERT;
            overrideStartedAt = millis();
            pixels.setMode(12);
            pixels.setBrightness(100);
            pixels.setSpeed(moduleConfig.ambient_lighting.speed);
            pixels.setColor(moduleConfig.ambient_lighting.red, moduleConfig.ambient_lighting.green,
                            moduleConfig.ambient_lighting.blue);
            pixels.start();
            setIntervalFromNow(0);
        }

        bool showPairingCode(const std::string &passkey)
        {
            if (passkey.size() != 6) {
                return false;
            }

            overrideMode = OverrideMode::PAIRING;
            overrideStartedAt = millis();
            pixels.stop();
            pixels.setBrightness(100);
            pixels.clear();

            const size_t count = NEOPIXEL_COUNT < 6 ? NEOPIXEL_COUNT : 6;
            for (size_t i = 0; i < count; ++i) {
                switch (passkey[i]) {
                case '4':
                    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
                    break;
                case '5':
                    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
                    break;
                case '6':
                    pixels.setPixelColor(i, pixels.Color(0, 0, 255));
                    break;
                default:
                    restorePersistedLighting();
                    return false;
                }
            }
            pixels.show();
            setIntervalFromNow(0);
            return true;
        }

        void restorePersistedLighting()
        {
            overrideMode = OverrideMode::NONE;
            applyPersistedLighting();
            setIntervalFromNow(0);
        }
#endif

      protected:
        int32_t runOnce() override
        {
#ifdef HAS_NEOPIXEL_ANIMATION
            if (overrideMode != OverrideMode::NONE) {
                const uint32_t duration = overrideMode == OverrideMode::PAIRING ? PAIRING_DURATION_MS : ALERT_DURATION_MS;
                if (millis() - overrideStartedAt >= duration) {
                    restorePersistedLighting();
                }
            }

            if (overrideMode != OverrideMode::PAIRING && pixels.isRunning()) {
                pixels.service();
                return 50;
            }
            return overrideMode == OverrideMode::PAIRING ? 50 : 1000;
#else
#ifdef HAS_RGB_LED
#if defined(HAS_NCP5623) || defined(HAS_LP5562)
        if ((_type == ScanI2C::NCP5623 || _type == ScanI2C::LP5562) && moduleConfig.ambient_lighting.led_state) {
#endif
            setLighting(moduleConfig.ambient_lighting.current, moduleConfig.ambient_lighting.red,
                        moduleConfig.ambient_lighting.green, moduleConfig.ambient_lighting.blue);
            return 30000;
#if defined(HAS_NCP5623) || defined(HAS_LP5562)
        }
#endif
#endif
        return disable();
#endif
        }

        CallbackObserver<AmbientLightingThread, void *> notifyDeepSleepObserver =
            CallbackObserver<AmbientLightingThread, void *>(this, &AmbientLightingThread::setLightingOff);

      private:
        ScanI2C::DeviceType _type = ScanI2C::DeviceType::NONE;

#ifdef HAS_NEOPIXEL_ANIMATION
        void persistSettings()
        {
            if (nodeDB) {
                nodeDB->saveToDisk(SEGMENT_MODULECONFIG);
            }
        }

        void applyIfNotOverridden()
        {
            if (overrideMode == OverrideMode::NONE) {
                applyPersistedLighting();
                setIntervalFromNow(0);
            }
        }

        void applyPersistedLighting()
        {
            pixels.setMode(moduleConfig.ambient_lighting.animation);
            pixels.setBrightness(moduleConfig.ambient_lighting.brightness);
            pixels.setSpeed(moduleConfig.ambient_lighting.speed);
            pixels.setColor(moduleConfig.ambient_lighting.red, moduleConfig.ambient_lighting.green,
                            moduleConfig.ambient_lighting.blue);

            if (isEnabled()) {
                pixels.start();
            } else {
                pixels.stop();
                pixels.clear();
                pixels.show();
            }
        }
#endif

        int setLightingOff(void *unused)
        {
#ifdef HAS_NCP5623
            rgb.setCurrent(0);
            rgb.setRed(0);
            rgb.setGreen(0);
            rgb.setBlue(0);
            LOG_INFO("OFF: NCP5623 Ambient lighting");
#endif
#ifdef HAS_LP5562
            rgbw.setCurrent(0);
            rgbw.setRed(0);
            rgbw.setGreen(0);
            rgbw.setBlue(0);
            rgbw.setWhite(0);
            LOG_INFO("OFF: LP5562 Ambient lighting");
#endif
#ifdef HAS_NEOPIXEL
#ifdef HAS_NEOPIXEL_ANIMATION
            pixels.stop();
#endif
            pixels.clear();
            pixels.show();
            LOG_INFO("OFF: NeoPixel Ambient lighting");
#endif
#ifdef RGBLED_CA
            analogWrite(RGBLED_RED, 255);
            analogWrite(RGBLED_GREEN, 255);
            analogWrite(RGBLED_BLUE, 255);
#elif defined(RGBLED_RED)
        analogWrite(RGBLED_RED, 0);
        analogWrite(RGBLED_GREEN, 0);
        analogWrite(RGBLED_BLUE, 0);
#endif
#ifdef UNPHONE
            unphone.rgb(0, 0, 0);
#endif
            return 0;
        }

      protected:
        void setLighting(float current, uint8_t red, uint8_t green, uint8_t blue)
        {
#ifdef HAS_NCP5623
            rgb.setCurrent(current);
            rgb.setRed(red);
            rgb.setGreen(green);
            rgb.setBlue(blue);
#endif
#ifdef HAS_LP5562
            rgbw.setCurrent(current);
            rgbw.setRed(red);
            rgbw.setGreen(green);
            rgbw.setBlue(blue);
#endif
#ifdef HAS_NEOPIXEL
#ifndef HAS_NEOPIXEL_ANIMATION
            pixels.setBrightness(current);
            pixels.fill(pixels.Color(red, green, blue), 0, NEOPIXEL_COUNT);
#if defined(BUTTON1_COLOR) && defined(BUTTON1_COLOR_INDEX)
            pixels.fill(BUTTON1_COLOR, BUTTON1_COLOR_INDEX, 1);
#endif
#if defined(BUTTON2_COLOR) && defined(BUTTON2_COLOR_INDEX)
            pixels.fill(BUTTON2_COLOR, BUTTON2_COLOR_INDEX, 1);
#endif
            pixels.show();
#else
        (void)current;
        (void)red;
        (void)green;
        (void)blue;
        if (overrideMode == OverrideMode::NONE && pixels.isRunning()) {
            pixels.service();
        }
#endif
#endif
#ifdef RGBLED_CA
            analogWrite(RGBLED_RED, 255 - red);
            analogWrite(RGBLED_GREEN, 255 - green);
            analogWrite(RGBLED_BLUE, 255 - blue);
#elif defined(RGBLED_RED)
        analogWrite(RGBLED_RED, red);
        analogWrite(RGBLED_GREEN, green);
        analogWrite(RGBLED_BLUE, blue);
#endif
#ifdef UNPHONE
            unphone.rgb(red, green, blue);
#endif
        }
    };

    extern AmbientLightingThread *ambientLightingThread;

#endif // AMBIENTLIGHTINGTHREAD_H
