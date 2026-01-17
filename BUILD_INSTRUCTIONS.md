# PlatformIO ESP32 Project (CLI Only instructions)

This project uses **PlatformIO** and can be built and flashed **entirely from the command line**.  
**VS Code is NOT required.**

PlatformIO will automatically download all required toolchains, frameworks, and flashing tools.

---

## Supported Platforms

- ✅ Windows 11
- ✅ Ubuntu 20.04 / 22.04 / 24.04
- ✅ macOS (similar steps, not documented here)

---

## Prerequisites

### Required
- Python 3.8+
- Git
- USB cable for ESP32

### Not Required
- ❌ VS Code
- ❌ Arduino IDE
- ❌ ESP-IDF manual install

---

## Windows 11 Setup

### 1. Install Python

Download from:  
https://www.python.org/downloads/windows/

⚠ During installation, **check**:
```

☑ Add Python to PATH

````

Verify:
```powershell
python --version
pip --version
````

---

### 2. Install PlatformIO Core (CLI)

Open **PowerShell**:

```powershell
pip install --user platformio
```

Restart PowerShell, then verify:

```powershell
pio --version
```

Expected:

```
PlatformIO Core 6.x.x
```

---

### 3. USB Driver (ESP32)

Most ESP32 boards use one of these USB chips:

* CP210x
* CH340
* FTDI

Windows 11 usually installs drivers automatically.
If flashing fails, install the correct driver for your board.

---

## Ubuntu Setup

### 1. Install prerequisites

```bash
sudo apt update
sudo apt install -y python3 python3-pip python3-venv git curl
```

---

### 2. Install PlatformIO Core (recommended)

```bash
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py | python3
```

Add PlatformIO to PATH:

```bash
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Verify:

```bash
pio --version
```

---

### 3. USB permissions (important)

```bash
sudo usermod -a -G dialout $USER
```

⚠ Log out and log back in after running this.

---

## Clone the Project

```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>
```

---

## Build the Project

```bash
pio run
```

---

## Flash (Upload) to ESP32

Plug in the ESP32 via USB, then run:

```bash
pio run -t upload
```

If multiple boards are connected:

```bash
pio run -t upload --upload-port /dev/ttyUSB0
```

(Windows example: `COM3`)

---

## Serial Monitor (Optional)

```bash
pio device monitor
```

Default baud rate is defined in `platformio.ini`.

---

## Project Structure

```
.
├── platformio.ini
├── src/
├── include/
├── lib/
└── README.md
```

⚠ The `.pio/` directory is generated automatically and should **NOT** be committed.

---

## Common Troubleshooting

### `pio: command not found`

* Ensure PlatformIO is in your PATH
* Restart terminal after installation

### Permission denied on `/dev/ttyUSB0`

```bash
sudo usermod -a -G dialout $USER
```

Log out and log back in.

### Flashing fails

* Try a different USB cable
* Hold the BOOT button during upload (some ESP32 boards)
* Specify port manually

---

## One-Command Summary

```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>
pio run -t upload
```

---

## Maintainer Notes

This project is tested with:

* PlatformIO Core 6.x
* ESP32 Arduino framework
* CLI-only workflow (no IDE dependency)

If you have issues, please include:

* OS
* `pio --version`
* Full error output

```
