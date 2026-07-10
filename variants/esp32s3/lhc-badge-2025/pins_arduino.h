#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include <variant.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// Map the default SPI bus to the SX1262 radio.
static const uint8_t SS = LORA_CS;
static const uint8_t SCK = LORA_SCK;
static const uint8_t MOSI = LORA_MOSI;
static const uint8_t MISO = LORA_MISO;

static const uint8_t SCL = I2C_SCL;
static const uint8_t SDA = I2C_SDA;

#endif // Pins_Arduino_h
