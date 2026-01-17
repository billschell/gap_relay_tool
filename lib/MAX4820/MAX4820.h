/**
 * @file MAX4820.h
 * @brief Library for controlling MAX4820 8-channel relay driver
 * 
 * The MAX4820 is an 8-channel relay driver with SPI/QSPI/MICROWIRE-compatible
 * serial interface. It features:
 * - Built-in inductive kickback protection
 * - 8 independent open-drain outputs (2Ω on-resistance, 70mA min sink)
 * - Support for +3.3V/+5V non-latching or dual-coil-latching relays
 * - SET/RESET pins for turning all outputs on/off simultaneously
 * 
 * Serial Interface:
 * - Uses hardware SPI (MOSI for data, SCK for clock)
 * - Data is clocked MSB first (D7 first, corresponding to OUT8)
 * - D7→OUT8, D6→OUT7, ... D0→OUT1
 * - Data latches to outputs on CS rising edge
 * - SPI Mode 0 (CPOL=0, CPHA=0)
 * 
 * Hardware SPI Pins (board-specific):
 *   Arduino Uno/Nano: MOSI=11, SCK=13
 *   Arduino Mega: MOSI=51, SCK=52
 *   ESP32: MOSI=23, SCK=18 (default VSPI)
 *   ESP32-S3: MOSI=11, SCK=12 (default)
 * 
 * This library supports controlling individual MAX4820 chips with their own
 * chip select pins (directly addressed, not daisy-chained).
 * 
 * @note Relay numbering: relay 0 = OUT1, relay 7 = OUT8
 * @note Call SPI.begin() before MAX4820::begin()
 */

#ifndef MAX4820_H
#define MAX4820_H

#include <Arduino.h>
#include <SPI.h>

// Use this value to indicate pin is not connected
#define MAX4820_PIN_NOT_CONNECTED 255

// Default SPI clock speed (1MHz, well under 2.1MHz max)
#define MAX4820_SPI_SPEED 1000000

class MAX4820 {
public:
    /**
     * @brief Construct a new MAX4820 driver instance
     * 
     * SCLK and DIN (MOSI) are handled by hardware SPI - connect to your
     * board's SPI pins.
     * 
     * @param csPin    Chip Select pin (active low, directly selects this chip)
     * @param resetPin RESET pin (active low) - drives all outputs OFF
     * @param setPin   SET pin (active low) - drives all outputs ON
     *                 Use MAX4820_PIN_NOT_CONNECTED if not wired
     */
    MAX4820(uint8_t csPin, uint8_t resetPin, uint8_t setPin = MAX4820_PIN_NOT_CONNECTED);

    /**
     * @brief Initialize the MAX4820 pins and reset the device
     * 
     * @note You must call SPI.begin() before calling this method!
     * 
     * Must be called in setup() before using other methods.
     */
    void begin();

    /**
     * @brief Reset all relay outputs to OFF state using RESET pin
     * Pulses the RESET pin low to clear all latches and registers.
     * RESET overrides all other inputs including SET.
     */
    void reset();

    /**
     * @brief Turn all relay outputs ON using SET pin
     * Pulses the SET pin low to set all latches and registers high.
     * SET overrides serial control inputs. RESET overrides SET.
     * @note Only works if SET pin was provided in constructor
     */
    void setAll();

    /**
     * @brief Set all 8 relays at once with a bit pattern
     * 
     * @param pattern Bit pattern where bit 0 = OUT1, bit 7 = OUT8
     *                A '1' turns the relay ON (output pulled to PGND)
     *                A '0' turns the relay OFF (output high impedance)
     */
    void setRelays(uint8_t pattern);

    /**
     * @brief Set a single relay ON or OFF
     * 
     * @param relay Relay number (0-7, where 0=OUT1 and 7=OUT8)
     * @param on    true to turn ON, false to turn OFF
     */
    void setRelay(uint8_t relay, bool on);

    /**
     * @brief Get the current state of a single relay
     * 
     * @param relay Relay number (0-7, where 0=OUT1 and 7=OUT8)
     * @return true if relay is ON, false if OFF
     */
    bool getRelay(uint8_t relay);

    /**
     * @brief Get the current state of all relays as a bit pattern
     * 
     * @return uint8_t Current relay states (bit 0=OUT1, bit 7=OUT8)
     */
    uint8_t getRelays();

    /**
     * @brief Pulse relays for dual-coil latching relay control
     * 
     * For dual-coil latching relays that need a pulse to change state:
     * 1. Sets the specified pattern
     * 2. Waits for pulseMs milliseconds
     * 3. Clears all outputs
     * 
     * @param pattern  Bit pattern of relays to pulse (bit 0=OUT1, bit 7=OUT8)
     * @param pulseMs  Pulse duration in milliseconds (default: 50ms)
     */
    void pulseRelays(uint8_t pattern, uint16_t pulseMs = 50);

    /**
     * @brief Pulse a single relay output for dual-coil latching relays
     * 
     * For KEMET EE2-xTNU double coil latch type relays where SET and RESET
     * coils are connected to different MAX4820 chips. Per KEMET datasheet:
     * - Pulse width must be > 10ms
     * - Operating time is ~2ms
     * 
     * @param relay    Relay number (0-7, where 0=OUT1 and 7=OUT8)
     * @param pulseMs  Pulse duration in milliseconds (default: 15ms)
     */
    void pulseRelay(uint8_t relay, uint16_t pulseMs = 15);

    /**
     * @brief Turn off all relays (same as setRelays(0))
     */
    void clearAll();

    /**
     * @brief Set the SPI clock speed
     * 
     * The MAX4820 supports SCLK up to 2.1MHz.
     * Default is 1MHz which is safe for all conditions.
     * 
     * @param speedHz SPI clock frequency in Hz (max 2100000)
     */
    void setSPISpeed(uint32_t speedHz);

private:
    uint8_t _csPin;
    uint8_t _resetPin;
    uint8_t _setPin;
    
    uint8_t _currentState;    // Track current relay states
    uint32_t _spiSpeed;       // SPI clock speed in Hz
    
    SPISettings _spiSettings; // Cached SPI settings

    /**
     * @brief Transfer 8 bits of data to the MAX4820 via SPI
     * 
     * Data is shifted MSB first (D7 first) as per datasheet.
     * D7→OUT8, D6→OUT7, ... D0→OUT1
     * 
     * @param data Byte to transfer
     */
    void transfer(uint8_t data);
};

#endif // MAX4820_H
