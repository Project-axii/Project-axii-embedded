# Project AXII Embedded

A comprehensive IoT home automation system built with ESP8266 microcontrollers. This project provides intelligent control of various devices through a centralized API, featuring three main components for lamp control, PC power management, and a multi-button control nexus.

## 🌟 Features

- **Remote Device Control**: Control devices via HTTP API with MySQL database integration
- **Real-time Status Monitoring**: Continuous device status checking with 5-second intervals
- **OTA Updates**: Over-The-Air firmware updates for easy maintenance
- **Multi-Device Support**: Control lamps, PCs, projectors, and air conditioning
- **State Change Detection**: Intelligent monitoring and response to device state changes
- **WiFi Connectivity**: Reliable WiFi connection with automatic reconnection

## 📋 Table of Contents

- [Components](#components)
- [Hardware Requirements](#hardware-requirements)
- [Software Dependencies](#software-dependencies)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [API Integration](#api-integration)
- [OTA Updates](#ota-updates)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## 🧩 Components

### 1. AX-LM (Lamp Controller)
Located in `/AX-LM/`

Controls a lamp through a relay based on device status from the API. The lamp turns ON when the device status is "offline" and OFF when "online".

**Features:**
- Relay control for lamp switching
- Status-based automatic control
- Last connection timestamp updates
- Circuit diagram included (circuito.pdf)

**Device ID:** 7  
**GPIO Pin:** D2  
**OTA Hostname:** ESP8266-Lampada

### 2. Power Link (PC Power Controller)
Located in `/Power Link/`

Manages PC power by sending pulse signals to the power button. Detects state changes and automatically triggers power button pulses.

**Features:**
- State change detection
- 500ms pulse generation for power button simulation
- PC power on/off automation
- Circuit diagrams included (circuito.png, Power link.png)

**Device ID:** 6  
**GPIO Pin:** D2  
**OTA Hostname:** ESP8266-PowerPC

### 3. Nexus (Multi-Button Control Hub)
Located in `/Nexus/`

A central control hub with 4 physical buttons for controlling multiple devices. Each button can toggle a different device's status.

**Features:**
- 4 independent button inputs with debouncing
- Direct device status toggling (online/offline)
- LED indicator for WiFi status and button presses
- 3D printable enclosure (model 3d/AXII Nexus.stl)
- Circuit diagram and visual reference (circuit.pdf, nexus.png)

**Button Configuration:**
- Button 1 (D5) - PC Lab (Device ID: 7)
- Button 2 (D6) - AC Lab (Device ID: 6)
- Button 3 (D7) - Projector (Device ID: 4)
- Button 4 (D8) - Lighting (Device ID: 5)

**OTA Hostname:** ESP8266-Controle

## 🔧 Hardware Requirements

### For Each Component:
- **Microcontroller**: ESP8266 (WeMos D1 Mini or equivalent)
- **Power Supply**: 5V USB power adapter
- **WiFi Network**: 2.4GHz WiFi access point

### Component-Specific Hardware:

**AX-LM:**
- 1x Relay module (5V compatible)
- Connecting wires
- Lamp/light fixture

**Power Link:**
- Connecting wires to PC power button
- Optocoupler or relay (recommended for isolation)

**Nexus:**
- 4x Push buttons
- 4x 10kΩ pull-up resistors (if not using internal pull-ups)
- LED (built-in on ESP8266)
- 3D printed enclosure (optional, STL file provided)

## 📚 Software Dependencies

### Arduino IDE Libraries:
```
- ESP8266WiFi (ESP8266 Core)
- ESP8266HTTPClient (ESP8266 Core)
- ArduinoJson (v6.x)
- ArduinoOTA (ESP8266 Core)
```

### Installation via Arduino Library Manager:
1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries**
3. Search and install:
   - "ArduinoJson" by Benoit Blanchon (version 6.x)

### ESP8266 Board Support:
1. Go to **File** → **Preferences**
2. Add to "Additional Board Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to **Tools** → **Board** → **Boards Manager**
4. Search for "esp8266" and install

## 🚀 Installation

### 1. Clone the Repository
```bash
git clone https://github.com/Project-axii/Project-axii-embedded.git
cd Project-axii-embedded
```

### 2. Hardware Setup
- Connect your ESP8266 to your computer via USB
- Wire the relay/buttons according to the circuit diagrams provided in each component folder
- Ensure proper power supply connections

### 3. Arduino IDE Setup
- Install Arduino IDE (version 1.8.x or 2.x)
- Install required libraries (see Software Dependencies)
- Select your board: **Tools** → **Board** → **ESP8266 Boards** → **WeMos D1 R1**
- Select the correct COM port: **Tools** → **Port**

### 4. Upload Firmware
- Open the desired component's `.ino` file in Arduino IDE
- Configure your settings (see Configuration section)
- Click **Upload** button
- Wait for compilation and upload to complete

## ⚙️ Configuration

### WiFi Configuration
In each `.ino` file, update the WiFi credentials:
```cpp
const char* ssid = "Your_WiFi_Name";
const char* password = "Your_WiFi_Password";
```

### API Configuration
Set your API endpoint:
```cpp
const char* apiBaseUrl = "http://your-api-server.com/api/devices";
```

### Device ID Configuration
Each component has a specific device ID. Update if needed:
```cpp
const int idDispositivo = 7;  // Change to your device ID
```

### OTA Configuration
OTA updates are secured with passwords. Default password is `admin123` (AX-LM, Power Link) or `admin` (Nexus). Update in the code:
```cpp
ArduinoOTA.setPassword("your_password");
```

### Nexus Button Mapping
To change button assignments in Nexus, modify the `botoes[]` array:
```cpp
Botao botoes[] = {
  {D5, deviceID, HIGH, 0, false, "Button Name"},
  // Add more buttons...
};
```

## 💡 Usage

### Initial Power-Up
1. Power on the ESP8266
2. The device will attempt to connect to WiFi
3. LED indicators will show connection status
4. Serial monitor (115200 baud) displays debug information

### AX-LM Operation
- Device automatically queries API every 5 seconds
- Lamp turns ON when device status is "offline"
- Lamp turns OFF when device status is "online"

### Power Link Operation
- Monitors device status every 5 seconds
- Detects status changes (online ↔ offline)
- Sends 500ms pulse to PC power button on state change

### Nexus Operation
- Press any button to toggle the corresponding device
- LED blinks when button is pressed
- Device status switches between "online" and "offline"

### Monitoring
Connect to Serial Monitor at 115200 baud to see:
- WiFi connection status
- API responses
- Device state changes
- Error messages

## 📁 Project Structure

```
Project-axii-embedded/
├── AX-LM/
│   ├── wemos_lamp.ino        # Lamp controller firmware
│   └── circuito.pdf           # Circuit diagram
├── Power Link/
│   ├── wemos_pc.ino           # PC power controller firmware
│   ├── circuito.png           # Circuit diagram
│   └── Power link.png         # Visual reference
├── Nexus/
│   ├── wemos-nexus.ino        # Multi-button controller firmware
│   ├── circuit.pdf            # Circuit diagram
│   ├── nexus.png              # Visual reference
│   └── model 3d/
│       └── AXII Nexus.stl     # 3D printable enclosure
├── LICENSE                     # MIT License
├── .gitignore                 # Git ignore rules
└── README.md                   # This file
```

## 🔌 API Integration

### Expected API Endpoints

**GET Device Status:**
```
GET {apiBaseUrl}?id={deviceId}
```

**Response Format (Option 1):**
```json
{
  "success": true,
  "data": {
    "id": 7,
    "nome": "Device Name",
    "status": "online",
    "ativo": true
  }
}
```

**Response Format (Option 2):**
```json
{
  "id": 7,
  "nome": "Device Name",
  "status": "online",
  "ativo": true
}
```

**POST Update Device Status (Nexus):**
```
POST {apiBaseUrl}
Content-Type: application/json

{
  "id": 7,
  "status": "offline"
}
```

**GET Update Last Connection:**
```
GET {apiBaseUrl}?action=update&id={deviceId}
```

## 🔄 OTA Updates

### Updating via Arduino IDE

1. Ensure device is on the same network
2. After first upload, device appears in **Tools** → **Port**
3. Select network port (e.g., "ESP8266-Lampada at 192.168.1.100")
4. Enter OTA password when prompted
5. Upload new firmware

### OTA Hostnames
- AX-LM: `ESP8266-Lampada`
- Power Link: `ESP8266-PowerPC`
- Nexus: `ESP8266-Controle`

### Security
Always change default OTA passwords in production environments!

## 🔍 Troubleshooting

### WiFi Connection Issues
- Verify SSID and password are correct
- Check if WiFi is 2.4GHz (ESP8266 doesn't support 5GHz)
- Ensure WiFi signal strength is adequate
- Check serial monitor for connection attempts

### API Connection Failures
- Verify API URL is accessible from the device network
- Check if API returns expected JSON format
- Monitor serial output for HTTP response codes
- Test API endpoint with tools like Postman or curl

### Relay/Button Not Responding
- Check GPIO pin connections
- Verify power supply is adequate
- Test with a simple LED blink sketch first
- Review serial monitor for debug messages

### OTA Update Fails
- Verify device is powered on and connected to WiFi
- Check OTA password is correct
- Ensure sufficient flash memory space
- Try power cycling the device

### Device Not Appearing in Network Ports
- Wait 30-60 seconds after device connects to WiFi
- Check if device and computer are on same network
- Verify ArduinoOTA.begin() is called in setup()
- Check firewall settings

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Copyright (c) 2025 AXII

## 👥 Authors

**AXII Team**

## 🙏 Acknowledgments

- ESP8266 Community
- Arduino Community
- ArduinoJson library by Benoit Blanchon

## 📧 Contact

For questions or support, please open an issue on GitHub.

---

**⚠️ Safety Warning:** When working with mains electricity (for lamp control), ensure proper safety measures and consult with a qualified electrician. Always disconnect power before making any electrical connections.