/**
 * @file main.cpp
 * @brief MAX4820 Relay Board Tester for ESP32-S3-DevKitC
 * 
 * Controls 5 MAX4820 chips (40 relay outputs total) via serial commands.
 * 
 * Hardware Configuration:
 *   ESP32-S3-DevKitC
 *   
 *   SPI Bus:
 *     MOSI  -> GPIO 11
 *     SCLK  -> GPIO 12
 *   
 *   Chip Select Pins:
 *     CS1   -> GPIO 10  (maxChip1: C (capacitance) relay SET coils, 8 latching relays)
 *     CS2   -> GPIO 13  (maxChip2: C (capacitance) relay RESET coils, same 8 relays)
 *     CS3   -> GPIO 14  (maxChip3: L (inductance) relay SET coils, 8 latching relays)
 *     CS4   -> GPIO 21  (maxChip4: L (inductance) relay RESET coils, same 8 relays)
 *     CS5   -> GPIO 1   (maxChip5: KN non-latching relays, 7 relays)
 *   
 *   Control Pins:
 *     RESET -> GPIO 42  (shared by all chips)
 *     SET   -> Not connected
 * 
 * Relay Types:
 *   - KEMET EE2-3NU:  Non-latching relays (7 on maxChip5)
 *   - KEMET EE2-3TNU: Double coil latching relays (16 total)
 *     - KC1-KC8: SET coils on maxChip1, RESET coils on maxChip2
 *     - KL1-KL8: SET coils on maxChip3, RESET coils on maxChip4
 * 
 * Serial Commands (9600 baud):
 *   C[1-8]+   - SET capacitance relay (close contacts)
 *   C[1-8]-   - RESET capacitance relay (open contacts)
 *   L[1-8]+   - SET inductance relay (close contacts)
 *   L[1-8]-   - RESET inductance relay (open contacts)
 *   N[1-7]    - Pulse non-latching relay for 1 second
 *   N[1-7]+   - Turn non-latching relay ON (persistent)
 *   N[1-7]-   - Turn non-latching relay OFF
 *   C=XX      - Block set all C relays (XX = hex pattern)
 *   L=XX      - Block set all L relays (XX = hex pattern)
 *   ?         - Show command help
 *   S         - Show relay status
 *   R         - Reset all latching relays to OPEN
 * 
 * Timing per KEMET EE2-3TNU datasheet:
 *   - Pulse width: > 10ms required
 *   - Operating time: ~2ms
 *   - Default pulse: 15ms
 */

#include <Arduino.h>
#include <SPI.h>
#include <MAX4820.h>

// =============================================================================
// Pin Definitions
// =============================================================================

// SPI pins (ESP32-S3)
#define PIN_MOSI    11
#define PIN_SCLK    12

// Chip Select pins for each MAX4820
#define PIN_CS1     10    // C (capacitance) SET coils
#define PIN_CS2     13    // C (capacitance) RESET coils
#define PIN_CS3     14    // L (inductance) SET coils
#define PIN_CS4     21    // L (inductance) RESET coils
#define PIN_CS5      1    // KN non-latching relays

// Shared RESET pin (active low, all outputs OFF)
#define PIN_RESET   42

// =============================================================================
// Timing Constants (per KEMET EE2 datasheet)
// =============================================================================

// Pulse width for latching relays (>10ms required, using 15ms)
#define LATCH_PULSE_MS      15

// Duration for non-latching relay actuation
#define NON_LATCH_DURATION_MS  1000

// Latching relay operation constants (for readability)
const bool SET   = true;
const bool RESET = false;

// =============================================================================
// MAX4820 Chip Instances
// =============================================================================

// maxChip1: C (capacitance) relay SET coils (latching relay "set" position)
MAX4820 maxChip1_C_SET(PIN_CS1, PIN_RESET);

// maxChip2: C (capacitance) relay RESET coils (latching relay "reset" position)
MAX4820 maxChip2_C_RESET(PIN_CS2, PIN_RESET);

// maxChip3: L (inductance) relay SET coils (latching relay "set" position)
MAX4820 maxChip3_L_SET(PIN_CS3, PIN_RESET);

// maxChip4: L (inductance) relay RESET coils (latching relay "reset" position)
MAX4820 maxChip4_L_RESET(PIN_CS4, PIN_RESET);

// maxChip5: KN non-latching relays (7 relays: KN1-KN7)
MAX4820 maxChip5_KN(PIN_CS5, PIN_RESET);

// =============================================================================
// Latching Relay Contact State Tracking
// =============================================================================
// Track mechanical contact position for latching relays
// C = capacitance relays, L = inductance relays
// true = SET (contacts closed), false = RESET (contacts open)
// Per KEMET datasheet: "Latch type relays should be initialized to a 
// known position before using."

bool C_contactState[8] = {false, false, false, false, false, false, false, false};
bool L_contactState[8] = {false, false, false, false, false, false, false, false};
bool N_relayState[7] = {false, false, false, false, false, false, false};

// =============================================================================
// Serial Input Buffer
// =============================================================================
// Accumulates characters until newline is received
String inputBuffer = "";
#define MAX_INPUT_LENGTH 32

// =============================================================================
// Function Prototypes
// =============================================================================

void processCommand(String cmd);
void showHelp();
void showStatus();
void actuateNonLatching(int relayNum);
void pulseLatchingRelay(const char* label, MAX4820& chip, int relayNum, bool isSet, bool* contactState);
void blockSetLatchingRelays(const char* label, MAX4820& setChip, MAX4820& resetChip, uint8_t pattern, bool* contactState);
void resetAll();

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(9600);
    delay(2000);  // Give serial monitor time to connect
    
    Serial.println();
    Serial.println("================================================");
    Serial.println("MAX4820 Relay Board Tester");
    Serial.println("ESP32-S3-DevKitC - 5 chips, 40 relay outputs");
    Serial.println("================================================");
    Serial.println();
    Serial.println("Relay Configuration:");
    Serial.println("  N1-N7:  Non-latching (KEMET EE2-3NU)");
    Serial.println("  C1-C8:  Capacitance latching (KEMET EE2-3TNU)");
    Serial.println("  L1-L8:  Inductance latching (KEMET EE2-3TNU)");
    Serial.println();
    
    // Initialize SPI bus with explicit pin assignment
    SPI.begin(PIN_SCLK, -1, PIN_MOSI, -1);  // SCK, MISO (unused), MOSI, SS (unused)
    
    // Initialize all MAX4820 chips
    maxChip1_C_SET.begin();
    maxChip2_C_RESET.begin();
    maxChip3_L_SET.begin();
    maxChip4_L_RESET.begin();
    maxChip5_KN.begin();
    
    Serial.println("All MAX4820 chips initialized");
    Serial.println("All relay outputs OFF");
    Serial.println();
    Serial.println("Type '?' for command list");
    Serial.println();
    Serial.print("> ");
}

// =============================================================================
// Main Loop - Serial Command Processing (Character-by-Character)
// =============================================================================

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        
        // Handle newline (command termination) and process command
        if (c == '\n' || c == '\r' ) {                       
            if (inputBuffer.length() > 0) {
                Serial.println();  // Move to new line
                inputBuffer.trim();
                inputBuffer.toUpperCase();
                processCommand(inputBuffer);
                inputBuffer = "";
                Serial.print("> ");
            }
        }
        // Handle backspace (ASCII 8 or 127)
        else if (c == '\b' || c == 127) {
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                Serial.print("\b \b");  // Erase character on screen
            }
        }
        // Normal character - echo and accumulate
        else if (c >= 32 && c < 127) {  // Printable ASCII only
            if (inputBuffer.length() < MAX_INPUT_LENGTH) {
                Serial.print(c);  // Echo immediately
                inputBuffer += c;
            }
            // Ignore if buffer is full
        }
        // Ignore other control characters
    }
}

// =============================================================================
// Command Processing
// =============================================================================

void processCommand(String cmd) {
    // ? - Help
    if (cmd == "?" || cmd == "HELP") {
        showHelp();
        return;
    }
    
    // S - Status
    if (cmd == "S" || cmd == "STATUS") {
        showStatus();
        return;
    }
    
    // R - Reset all relays
    if (cmd == "R" || cmd == "RESET") {
        resetAll();
        return;
    }
    
    // C[1-8]+ or C[1-8]- : Capacitance relay SET/RESET
    if (cmd.length() == 3 && cmd.charAt(0) == 'C') {
        int relayNum = cmd.charAt(1) - '0';
        char op = cmd.charAt(2);
        if (relayNum >= 1 && relayNum <= 8 && (op == '+' || op == '-')) {
            bool isSet = (op == '+');
            pulseLatchingRelay("C", isSet ? maxChip1_C_SET : maxChip2_C_RESET, relayNum, isSet, C_contactState);
            return;
        }
    }
    
    // L[1-8]+ or L[1-8]- : Inductance relay SET/RESET
    if (cmd.length() == 3 && cmd.charAt(0) == 'L') {
        int relayNum = cmd.charAt(1) - '0';
        char op = cmd.charAt(2);
        if (relayNum >= 1 && relayNum <= 8 && (op == '+' || op == '-')) {
            bool isSet = (op == '+');
            pulseLatchingRelay("L", isSet ? maxChip3_L_SET : maxChip4_L_RESET, relayNum, isSet, L_contactState);
            return;
        }
    }
    
    // N[1-7] : Non-latching relay pulse (momentary, 1 second)
    if (cmd.length() == 2 && cmd.charAt(0) == 'N') {
        int relayNum = cmd.charAt(1) - '0';
        if (relayNum >= 1 && relayNum <= 7) {
            actuateNonLatching(relayNum);
            return;
        }
    }
    
    // N[1-7]+ or N[1-7]- : Non-latching relay ON/OFF (persistent)
    if (cmd.length() == 3 && cmd.charAt(0) == 'N') {
        int relayNum = cmd.charAt(1) - '0';
        char op = cmd.charAt(2);
        if (relayNum >= 1 && relayNum <= 7 && (op == '+' || op == '-')) {
            int relayIndex = relayNum - 1;
            bool turnOn = (op == '+');
            maxChip5_KN.setRelay(relayIndex, turnOn);
            N_relayState[relayIndex] = turnOn;
            Serial.printf("N%d: %s\n", relayNum, turnOn ? "SET" : "OPEN");
            return;
        }
    }
    
    // C=XX : Block set C relays with hex pattern
    if (cmd.length() == 4 && cmd.startsWith("C=")) {
        uint8_t pattern = (uint8_t)strtol(cmd.c_str() + 2, NULL, 16);
        blockSetLatchingRelays("C", maxChip1_C_SET, maxChip2_C_RESET, pattern, C_contactState);
        return;
    }
    
    // L=XX : Block set L relays with hex pattern
    if (cmd.length() == 4 && cmd.startsWith("L=")) {
        uint8_t pattern = (uint8_t)strtol(cmd.c_str() + 2, NULL, 16);
        blockSetLatchingRelays("L", maxChip3_L_SET, maxChip4_L_RESET, pattern, L_contactState);
        return;
    }
    
    // Unknown command
    Serial.println("ERROR: Unknown command. Type '?' for help.");
}

// =============================================================================
// Help Display
// =============================================================================

void showHelp() {
    Serial.println();
    Serial.println("=== Command Reference ===");
    Serial.println();
    Serial.println("Capacitance Relays (C1-C8):");
    Serial.println("  C1+ - C8+    SET relay (close contacts)");
    Serial.println("  C1- - C8-    RESET relay (open contacts)");
    Serial.println("  C=XX         Block set all C relays (XX = hex)");
    Serial.println();
    Serial.println("Inductance Relays (L1-L8):");
    Serial.println("  L1+ - L8+    SET relay (close contacts)");
    Serial.println("  L1- - L8-    RESET relay (open contacts)");
    Serial.println("  L=XX         Block set all L relays (XX = hex)");
    Serial.println();
    Serial.println("Non-Latching Relays (N1-N7):");
    Serial.println("  N1 - N7      Pulse relay for 1 second");
    Serial.println("  N1+ - N7+    Turn relay ON (persistent)");
    Serial.println("  N1- - N7-    Turn relay OFF");
    Serial.println();
    Serial.println("Block Commands (C=XX, L=XX):");
    Serial.println("  XX is 2-digit hex (00-FF), bit 0=relay 1");
    Serial.println("  1=SET, 0=RESET. Ex: L=F0 sets 5-8, resets 1-4");
    Serial.println();
    Serial.println("Utility:");
    Serial.println("  ?    Help");
    Serial.println("  S    Status");
    Serial.println("  R    Reset all latching relays to OPEN");
    Serial.println();
}

// =============================================================================
// Status Display
// =============================================================================

void showStatus() {
    Serial.println();
    Serial.println("=== Relay Board Status ===");
    Serial.println();
    
    // C (capacitance) Latching Relays - contact position
    Serial.println("C Latching Relays (capacitance):");
    Serial.print("  ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("C%d:%s ", i + 1, C_contactState[i] ? "SET " : "OPEN");
        if (i == 3) {
            Serial.println();
            Serial.print("  ");
        }
    }
    Serial.println("\n");
    
    // L (inductance) Latching Relays - contact position
    Serial.println("L Latching Relays (inductance):");
    Serial.print("  ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("L%d:%s ", i + 1, L_contactState[i] ? "SET " : "OPEN");
        if (i == 3) {
            Serial.println();
            Serial.print("  ");
        }
    }
    Serial.println("\n");
    
    // Non-latching relays
    Serial.println("N Non-Latching Relays:");
    Serial.print("  ");
    for (int i = 0; i < 7; i++) {
        Serial.printf("N%d:%s ", i + 1, N_relayState[i] ? "SET " : "OPEN");
        if (i == 3) {
            Serial.println();
            Serial.print("  ");
        }
    }
    Serial.println("\n");
}

// =============================================================================
// Non-Latching Relay Actuation (KN1-KN7)
// =============================================================================

void actuateNonLatching(int relayNum) {
    // relayNum is 1-7, convert to 0-6 for library
    int relayIndex = relayNum - 1;
    
    Serial.printf("N%d: Pulsing for %dms...\n", relayNum, NON_LATCH_DURATION_MS);
    
    // Turn on relay
    maxChip5_KN.setRelay(relayIndex, true);
    
    // Wait for actuation duration
    delay(NON_LATCH_DURATION_MS);
    
    // Turn off relay
    maxChip5_KN.setRelay(relayIndex, false);
    
    Serial.printf("N%d: Done\n", relayNum);
}

// =============================================================================
// Latching Relay Control (KC1-KC8, KL1-KL8)
// =============================================================================

/**
 * @brief Pulse a latching relay coil (SET or RESET)
 * 
 * @param label       Relay group label for print statements ("KC" or "KL")
 * @param chip        MAX4820 chip controlling the coil
 * @param relayNum    Relay number (1-8)
 * @param isSet       true for SET operation, false for RESET operation
 * @param contactState Pointer to contact state array to update
 */
void pulseLatchingRelay(const char* label, MAX4820& chip, int relayNum, bool isSet, bool* contactState) {
    // relayNum is 1-8, convert to 0-7 for library
    int relayIndex = relayNum - 1;
    const char* operation = isSet ? "SET" : "RESET";
    const char* result = isSet ? "contacts closed" : "contacts open";
       
    // Pulse the appropriate coil
    chip.pulseRelay(relayIndex, LATCH_PULSE_MS);
    
    // Update contact state tracking
    contactState[relayIndex] = isSet;
    
    Serial.printf("%s%d: %s\n", label, relayNum, operation);
}

// =============================================================================
// Block Set Latching Relays (BC/BL commands)
// =============================================================================

/**
 * @brief Block set all 8 latching relays at once using a bit pattern
 * 
 * Bits that are 1 will be SET, bits that are 0 will be RESET.
 * Both operations are pulsed simultaneously for efficiency.
 * 
 * @param label        Relay group label for print statements ("C" or "L")
 * @param setChip      MAX4820 chip controlling the SET coils
 * @param resetChip    MAX4820 chip controlling the RESET coils
 * @param pattern      Bit pattern: 1=SET, 0=RESET (bit 0=relay 1, bit 7=relay 8)
 * @param contactState Pointer to contact state array to update
 */
void blockSetLatchingRelays(const char* label, MAX4820& setChip, MAX4820& resetChip, 
                            uint8_t pattern, bool* contactState) {
    // Pulse SET coils for bits that are 1
    if (pattern != 0) {
        setChip.pulseRelays(pattern, LATCH_PULSE_MS);
    }
    
    // Small delay between operations
    delay(5);
    
    // Pulse RESET coils for bits that are 0 (inverted pattern)
    uint8_t resetPattern = ~pattern;
    if (resetPattern != 0) {
        resetChip.pulseRelays(resetPattern, LATCH_PULSE_MS);
    }
    
    // Update contact state array
    for (int i = 0; i < 8; i++) {
        contactState[i] = (pattern >> i) & 0x01;
    }
    
    // Print binary representation of new state
    char binaryStr[9];
    for (int i = 7; i >= 0; i--) {
        binaryStr[7 - i] = ((pattern >> i) & 0x01) ? '1' : '0';
    }
    binaryStr[8] = '\0';
    
    Serial.printf("%s relays updated, new state: %s\n", label, binaryStr);
}

// =============================================================================
// Reset All Relays
// =============================================================================

void resetAll() {
    Serial.println("Resetting all relays...");
    Serial.println();
    
    // Reset all C (capacitance) relays (pulse all RESET coils on maxChip2)
    Serial.println("C relays: pulsing RESET coils...");
    maxChip2_C_RESET.pulseRelays(0xFF, LATCH_PULSE_MS);
    for (int i = 0; i < 8; i++) {
        C_contactState[i] = false;
    }
    Serial.println("C1-C8: RESET complete");
    
    // Small delay between chip operations
    delay(10);
    
    // Reset all L (inductance) relays (pulse all RESET coils on maxChip4)
    Serial.println("L relays: pulsing RESET coils...");
    maxChip4_L_RESET.pulseRelays(0xFF, LATCH_PULSE_MS);
    for (int i = 0; i < 8; i++) {
        L_contactState[i] = false;
    }
    Serial.println("L1-L8: RESET complete");
    
    // Small delay between chip operations
    delay(10);
    
    // Turn off all N (non-latching) relays
    Serial.println("N relays: turning OFF...");
    maxChip5_KN.setRelays(0x00);
    for (int i = 0; i < 7; i++) {
        N_relayState[i] = false;
    }
    Serial.println("N1-N7: OFF complete");
    
    Serial.println();
    Serial.println("All 23 relays now OPEN/OFF");
}
