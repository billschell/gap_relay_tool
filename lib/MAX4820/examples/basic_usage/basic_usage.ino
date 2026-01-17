 /**
 * @file basic_usage.ino
 * @brief Example demonstrating MAX4820 library usage with hardware SPI
 * 
 * This example shows how to use the MAX4820 library to control
 * individual relay driver chips with separate chip select pins.
 * 
 * MAX4820 Pin Connections:
 *   DIN    -> Board's MOSI pin (hardware SPI)
 *   SCLK   -> Board's SCK pin (hardware SPI)
 *   CS     -> Any GPIO (active low, chip select)
 *   RESET  -> Any GPIO (active low, all outputs OFF)
 *   SET    -> Any GPIO (active low, all outputs ON) - optional
 *   VCC    -> 3.3V or 5V
 *   COM    -> VCC (for kickback protection diodes)
 *   GND    -> GND
 *   PGND   -> GND
 * 
 * Hardware SPI Pins by Board:
 *   Arduino Uno/Nano:  MOSI=11, SCK=13
 *   Arduino Mega:      MOSI=51, SCK=52
 *   ESP32 (VSPI):      MOSI=23, SCK=18
 *   ESP32-S3:          MOSI=11, SCK=12
 * 
 * Relay numbering: relay 0 = OUT1, relay 7 = OUT8
 */

#include <SPI.h>
#include <MAX4820.h>

// Pin definitions for first MAX4820 chip (with SET pin)
#define CHIP1_CS    6
#define CHIP1_RESET 5
#define CHIP1_SET   9   // Optional SET pin

// Pin definitions for second MAX4820 chip (without SET pin)
#define CHIP2_CS    8
#define CHIP2_RESET 5   // Can share RESET (if OK to reset both chips together)

// Create MAX4820 instances - each with its own chip select
// Chip 1: All pins connected (including SET)
MAX4820 relayChip1(CHIP1_CS, CHIP1_RESET, CHIP1_SET);

// Chip 2: Without SET pin (use MAX4820_PIN_NOT_CONNECTED)
MAX4820 relayChip2(CHIP2_CS, CHIP2_RESET, MAX4820_PIN_NOT_CONNECTED);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for serial port
    }
    
    Serial.println("MAX4820 Library Demo (Hardware SPI)");
    Serial.println("====================================");
    Serial.println("Relay mapping: relay 0=OUT1 ... relay 7=OUT8");
    Serial.println();
    
    // IMPORTANT: Initialize SPI bus BEFORE calling begin() on MAX4820
    SPI.begin();
    
    // Initialize both chips (resets all outputs to OFF)
    relayChip1.begin();
    relayChip2.begin();
    
    Serial.println("SPI initialized");
    Serial.println("Both chips initialized (all outputs OFF)");
}

void loop() {
    // =========================================
    // Example 1: Set individual relays
    // =========================================
    Serial.println("\n--- Individual Relay Control ---");
    
    relayChip1.setRelay(0, true);   // Turn on relay 0 (OUT1)
    Serial.println("Chip1: Relay 0 (OUT1) ON");
    delay(500);
    
    relayChip1.setRelay(3, true);   // Turn on relay 3 (OUT4)
    Serial.println("Chip1: Relay 3 (OUT4) ON");
    delay(500);
    
    relayChip1.setRelay(0, false);  // Turn off relay 0 (OUT1)
    Serial.println("Chip1: Relay 0 (OUT1) OFF");
    delay(500);
    
    // =========================================
    // Example 2: Set all relays with a pattern
    // =========================================
    Serial.println("\n--- Pattern Control ---");
    
    // Pattern 0b10101010 = relays 1,3,5,7 ON (OUT2,OUT4,OUT6,OUT8)
    relayChip1.setRelays(0b10101010);
    Serial.println("Chip1: Pattern 0b10101010 (OUT2,OUT4,OUT6,OUT8 ON)");
    delay(1000);
    
    // Pattern 0b01010101 = relays 0,2,4,6 ON (OUT1,OUT3,OUT5,OUT7)
    relayChip2.setRelays(0b01010101);
    Serial.println("Chip2: Pattern 0b01010101 (OUT1,OUT3,OUT5,OUT7 ON)");
    delay(1000);
    
    // =========================================
    // Example 3: Read current state
    // =========================================
    Serial.println("\n--- Reading State ---");
    Serial.print("Chip1 state: 0b");
    Serial.println(relayChip1.getRelays(), BIN);
    Serial.print("Chip2 state: 0b");
    Serial.println(relayChip2.getRelays(), BIN);
    Serial.print("Chip1 Relay 3 (OUT4) is: ");
    Serial.println(relayChip1.getRelay(3) ? "ON" : "OFF");
    
    // =========================================
    // Example 4: Clear all relays
    // =========================================
    Serial.println("\n--- Clear All ---");
    relayChip1.clearAll();
    relayChip2.clearAll();
    Serial.println("Both chips cleared (all outputs OFF)");
    delay(1000);
    
    // =========================================
    // Example 5: SET pin - turn all ON at once
    // =========================================
    Serial.println("\n--- SET Pin (all outputs ON) ---");
    relayChip1.setAll();  // Uses SET pin (fast hardware method)
    Serial.println("Chip1: All outputs ON via SET pin");
    delay(1000);
    
    relayChip2.setAll();  // Falls back to SPI (SET pin not connected)
    Serial.println("Chip2: All outputs ON via SPI (no SET pin)");
    delay(1000);
    
    // =========================================
    // Example 6: Pulse relays (for dual-coil latching relays)
    // =========================================
    Serial.println("\n--- Pulse Mode (for latching relays) ---");
    Serial.println("Pulsing Chip1 relays 4-7 (OUT5-OUT8) for 100ms...");
    relayChip1.pulseRelays(0b11110000, 100);  // Pulse high nibble
    Serial.println("Pulse complete - outputs now OFF");
    delay(1000);
    
    // =========================================
    // Example 7: Hardware RESET
    // =========================================
    Serial.println("\n--- Hardware RESET ---");
    relayChip1.reset();
    relayChip2.reset();
    Serial.println("Both chips reset via RESET pin (all outputs OFF)");
    
    // =========================================
    // Example 8: Adjust SPI speed (optional)
    // =========================================
    Serial.println("\n--- SPI Speed Adjustment ---");
    relayChip1.setSPISpeed(2000000);  // 2MHz (near max of 2.1MHz)
    Serial.println("Chip1 SPI speed set to 2MHz");
    relayChip1.setRelays(0xFF);       // Test at higher speed
    delay(100);
    relayChip1.clearAll();
    relayChip1.setSPISpeed(1000000);  // Back to default 1MHz
    Serial.println("Chip1 SPI speed restored to 1MHz");
    
    Serial.println("\n--- Demo Complete, restarting in 3 seconds ---");
    delay(3000);
}
