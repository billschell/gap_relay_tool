/**
 * @file MAX4820.cpp
 * @brief Implementation of MAX4820 8-channel relay driver library
 * 
 * Uses hardware SPI for communication. Per datasheet:
 * - SCLK max frequency: 2.1MHz
 * - Data sampled on SCLK rising edge (SPI Mode 0)
 * - MSB first (D7 → OUT8)
 * - Data latches on CS rising edge
 * - RESET/SET pulse width: 70ns min
 */

#include "MAX4820.h"

MAX4820::MAX4820(uint8_t csPin, uint8_t resetPin, uint8_t setPin)
    : _csPin(csPin)
    , _resetPin(resetPin)
    , _setPin(setPin)
    , _currentState(0)
    , _spiSpeed(MAX4820_SPI_SPEED)
    , _spiSettings(MAX4820_SPI_SPEED, MSBFIRST, SPI_MODE0)
{
}

void MAX4820::begin() {
    // Configure control pins as outputs
    pinMode(_csPin, OUTPUT);
    pinMode(_resetPin, OUTPUT);
    
    // Configure SET pin if connected
    if (_setPin != MAX4820_PIN_NOT_CONNECTED) {
        pinMode(_setPin, OUTPUT);
        digitalWrite(_setPin, HIGH);  // SET is active low, keep inactive
    }

    // Initialize CS high (deselected)
    digitalWrite(_csPin, HIGH);
    
    // Pulse RESET to ensure known initial state (all outputs off)
    // Minimum pulse width per datasheet: 70ns
    digitalWrite(_resetPin, LOW);
    delayMicroseconds(1);  // Well above 70ns minimum
    digitalWrite(_resetPin, HIGH);
    
    _currentState = 0;
}

void MAX4820::reset() {
    /*
     * RESET: Reset Input. Drive RESET low to clear all latches and registers
     * (all outputs are turned off). RESET overrides all other inputs. If 
     * RESET and SET are pulled low at the same time, then RESET takes
     * precedence.
     * 
     * Minimum pulse width: 70ns
     */
    digitalWrite(_resetPin, LOW);
    delayMicroseconds(1);  // Well above 70ns minimum
    digitalWrite(_resetPin, HIGH);
    
    _currentState = 0;
}

void MAX4820::setAll() {
    /*
     * SET: Set Input. Drive SET low to set all latches and registers high
     * (all outputs are turned on). SET overrides all parallel and serial
     * control inputs. RESET overrides SET under all conditions.
     * 
     * Minimum pulse width: 70ns
     */
    if (_setPin == MAX4820_PIN_NOT_CONNECTED) {
        // SET pin not wired - use serial interface instead
        setRelays(0xFF);
        return;
    }
    
    digitalWrite(_setPin, LOW);
    delayMicroseconds(1);  // Well above 70ns minimum
    digitalWrite(_setPin, HIGH);
    
    _currentState = 0xFF;
}

void MAX4820::transfer(uint8_t data) {
    /*
     * Serial Interface (from datasheet):
     * - Drive CS low to select the device
     * - Data at DIN is clocked into 8-bit shift register on SCLK rising edge
     * - D7 is the FIRST bit in (MSB first) - handled by MSBFIRST setting
     * - D7→OUT8, D6→OUT7, ... D0→OUT1
     * - Drive CS from low to high to latch data and activate relays
     * 
     * SPI Mode 0 (CPOL=0, CPHA=0):
     * - Clock idles low
     * - Data sampled on rising edge
     * This matches MAX4820 requirements.
     */
    
    SPI.beginTransaction(_spiSettings);
    
    digitalWrite(_csPin, LOW);   // Select chip
    SPI.transfer(data);          // Clock out 8 bits MSB first
    digitalWrite(_csPin, HIGH);  // Deselect - rising edge latches data to outputs
    
    SPI.endTransaction();
}

void MAX4820::setRelays(uint8_t pattern) {
    transfer(pattern);
    _currentState = pattern;
}

void MAX4820::setRelay(uint8_t relay, bool on) {
    if (relay > 7) return;  // Invalid relay number (valid: 0-7 for OUT1-OUT8)
    
    if (on) {
        _currentState |= (1 << relay);   // Set bit
    } else {
        _currentState &= ~(1 << relay);  // Clear bit
    }
    
    transfer(_currentState);
}

bool MAX4820::getRelay(uint8_t relay) {
    if (relay > 7) return false;
    return (_currentState & (1 << relay)) != 0;
}

uint8_t MAX4820::getRelays() {
    return _currentState;
}

void MAX4820::pulseRelays(uint8_t pattern, uint16_t pulseMs) {
    /*
     * For dual-coil latching relays:
     * These relays need a current pulse to change state, then the current
     * can be removed and the relay maintains its position.
     * 
     * Sequence:
     * 1. Set the desired pattern to energize relay coils
     * 2. Wait for pulse duration (relay mechanical response time)
     * 3. Clear all outputs to de-energize coils
     */
    
    // Set the desired pattern
    transfer(pattern);
    
    // Wait for pulse duration
    delay(pulseMs);
    
    // Clear all outputs
    transfer(0);
    
    // Note: _currentState tracks electrical state (now 0), not mechanical 
    // relay position for latching relays
    _currentState = 0;
}

void MAX4820::clearAll() {
    setRelays(0);
}

void MAX4820::setSPISpeed(uint32_t speedHz) {
    // Clamp to MAX4820 maximum of 2.1MHz
    if (speedHz > 2100000) {
        speedHz = 2100000;
    }
    _spiSpeed = speedHz;
    _spiSettings = SPISettings(_spiSpeed, MSBFIRST, SPI_MODE0);
}

void MAX4820::pulseRelay(uint8_t relay, uint16_t pulseMs) {
    /*
     * Pulse a single relay output for dual-coil latching relays.
     * 
     * For KEMET EE2-xTNU double coil latch type relays:
     * - SET and RESET coils are on separate MAX4820 chips
     * - Pulse width must be > 10ms per relay datasheet
     * - Operating time is ~2ms, so 10-50ms pulse is typical
     * 
     * Sequence:
     * 1. Turn on the specified relay output (energize coil)
     * 2. Wait for pulse duration (relay mechanical response)
     * 3. Turn off the relay output (de-energize coil)
     */
    if (relay > 7) return;  // Invalid relay number
    
    // Turn on just this relay
    uint8_t pattern = (1 << relay);
    transfer(pattern);
    
    // Wait for pulse duration (>10ms per KEMET datasheet)
    delay(pulseMs);
    
    // Turn off
    transfer(0);
    
    _currentState = 0;
}
