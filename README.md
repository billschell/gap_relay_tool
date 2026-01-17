# GAP Relay Tool

A PlatformIO-based firmware for ESP32-S3 that controls 40 relay outputs via five MAX4820 8-channel relay driver chips. Designed for controlling both latching and non-latching relays through a serial command interface.

## Features

- **40 Relay Outputs**: 5 MAX4820 chips providing 8 outputs each
- **Dual Relay Types**: Support for both latching and non-latching relays
  - 16 latching relays (KEMET EE2-3TNU dual-coil)
  - 7 non-latching relays (KEMET EE2-3NU)
- **Serial Command Interface**: Simple text commands at 9600 baud
- **Block Operations**: Set multiple relays simultaneously with hex patterns
- **State Tracking**: Maintains relay contact state in software
- **Custom MAX4820 Library**: Reusable SPI driver with pulse support for latching relays

## Hardware Configuration

### Microcontroller
- **Board**: ESP32-S3-DevKitC-1

### SPI Bus
| Signal | GPIO |
|--------|------|
| MOSI   | 11   |
| SCLK   | 12   |

### Chip Select Pins
| Chip | GPIO | Function |
|------|------|----------|
| CS1  | 10   | Capacitance (C) relay SET coils |
| CS2  | 13   | Capacitance (C) relay RESET coils |
| CS3  | 14   | Inductance (L) relay SET coils |
| CS4  | 21   | Inductance (L) relay RESET coils |
| CS5  | 1    | Non-latching (N) relays |

### Control Pins
| Signal | GPIO | Notes |
|--------|------|-------|
| RESET  | 42   | Shared by all chips (active low) |
| SET    | NC   | Not connected |

### Relay Configuration
- **C1-C8**: Capacitance selection latching relays (KEMET EE2-3TNU)
- **L1-L8**: Inductance selection latching relays (KEMET EE2-3TNU)
- **N1-N7**: Non-latching relays (KEMET EE2-3NU)

## Serial Commands

Connect via serial terminal at **9600 baud**.

### Capacitance Relays (C1-C8)
| Command | Description |
|---------|-------------|
| `C1+` - `C8+` | SET relay (close contacts) |
| `C1-` - `C8-` | RESET relay (open contacts) |
| `C=XX` | Block set all C relays (XX = hex pattern) |

### Inductance Relays (L1-L8)
| Command | Description |
|---------|-------------|
| `L1+` - `L8+` | SET relay (close contacts) |
| `L1-` - `L8-` | RESET relay (open contacts) |
| `L=XX` | Block set all L relays (XX = hex pattern) |

### Non-Latching Relays (N1-N7)
| Command | Description |
|---------|-------------|
| `N1` - `N7` | Pulse relay for 1 second |
| `N1+` - `N7+` | Turn relay ON (persistent) |
| `N1-` - `N7-` | Turn relay OFF |

### Block Commands
The hex pattern `XX` sets individual bits where:
- Bit 0 = Relay 1, Bit 7 = Relay 8
- `1` = SET (contacts closed), `0` = RESET (contacts open)

Example: `L=F0` sets L5-L8 and resets L1-L4

### Utility Commands
| Command | Description |
|---------|-------------|
| `?` | Show command help |
| `S` | Show relay status |
| `R` | Reset all latching relays to OPEN |

## Building and Uploading

### Prerequisites
- [PlatformIO](https://platformio.org/) (CLI or IDE extension)

### Build
```bash
pio run
```

### Upload
```bash
pio run --target upload
```

### Serial Monitor
```bash
pio device monitor
```

## MAX4820 Library

A custom Arduino library for the MAX4820 8-channel relay driver is included in `lib/MAX4820/`.

### Features
- Hardware SPI interface (Mode 0)
- Individual and bulk relay control
- Pulse mode for dual-coil latching relays
- Built-in RESET/SET pin support
- State tracking

### Basic Usage
```cpp
#include <MAX4820.h>

MAX4820 relay(CS_PIN, RESET_PIN);

void setup() {
    SPI.begin();
    relay.begin();
}

void loop() {
    relay.setRelay(0, true);      // Turn on relay 1
    delay(1000);
    relay.setRelay(0, false);     // Turn off relay 1
    delay(1000);
    
    relay.pulseRelay(3, 15);      // Pulse relay 4 for 15ms
    relay.setRelays(0xFF);        // Turn all relays on
    relay.pulseRelays(0x0F, 20);  // Pulse relays 1-4 for 20ms
}
```

## Timing

Per KEMET EE2-3TNU datasheet:
- Minimum pulse width: >10ms
- Operating time: ~2ms
- Default latching pulse: 15ms
- Non-latching pulse duration: 1000ms

## Project Structure

```
gap_relay_tool/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp            # Main firmware
├── lib/
│   └── MAX4820/            # MAX4820 relay driver library
│       ├── MAX4820.cpp
│       ├── MAX4820.h
│       └── examples/
├── include/
└── test/
```

## License

This project is provided as-is for educational and development purposes.
