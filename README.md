# ESP32-Quad-Relay
ESP32 Quad Relay Controller with Dynamic Configuration
# Quad Relay Controller with Dynamic Configuration

**Author:** Jim (M1Musik.com)  
**Hardware:** [ESP32 4-Channel Relay Module](https://amzn.eu/d/8gHN66b) (Model: LC-Relay-ESP32-4R-A2)

## Overview

This project is a full-featured **Quad Relay Controller** for an ESP32 board, designed to provide dynamic GPIO configuration, NTP integration, and MQTT-based Home Assistant discovery. It enables seamless management of relay states and allows for flexible setup via a web interface and MQTT.

### Key Features
- **Dynamic GPIO Configuration**: Configure GPIO pins for relays easily.
- **NTP Integration**: Ensures accurate timekeeping through NTP servers.
- **MQTT Integration**: Enables state publishing and command handling.
- **Home Assistant Discovery**: Automatically integrates with Home Assistant for smooth operation.
- **Web Interface**: User-friendly relay controls and WiFi management.
- **Deep Reset Functionality**: Enter configuration mode with a single button press.

## Hardware Requirements
- **Microcontroller**: ESP32 4-Channel Relay Module (LC-Relay-ESP32-4R-A2)
- **Connectivity**: WiFi access point for setup and MQTT broker.

## Software Requirements
- **Arduino IDE** (or equivalent ESP32 development environment)
- Required Libraries:
  - [WiFiManager](https://github.com/tzapu/WiFiManager)
  - [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
  - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
  - [AsyncMqttClient](https://github.com/marvinroger/async-mqtt-client)
  - [Preferences](https://github.com/espressif/arduino-esp32)

## Configuration Parameters
- **WiFi**: Configurable SSID and password via WiFiManager.
- **MQTT**: Host, username, password, and device name.
- **Relay Pins**: Defaults are GPIO 25, 26, 32, 33.
- **NTP Servers**: Defaults to `0.de.pool.ntp.org` and `1.de.pool.ntp.org`.
- **Timezone**: Example: `CET-1CEST,M3.5.0,M10.5.0/3`.

## Setup Instructions
1. **Flash the ESP32**:
   - Open the sketch in Arduino IDE.
   - Configure the necessary libraries and board settings.
   - Upload the sketch to your ESP32 device.

2. **WiFi Configuration**:
   - On first boot or after a reset, connect to the configuration portal.
   - Enter your WiFi details, MQTT credentials, and desired settings.

3. **Integration with Home Assistant**:
   - The device will automatically publish its configuration for discovery.
   - Check Home Assistant to see the new entities for relay control.

4. **Web Interface**:
   - Access the ESP32's IP address in a browser.
   - Use the interactive dashboard to toggle relays and monitor states.

## Usage
- Control relays via the web interface, MQTT commands, or Home Assistant.
- Modify configuration dynamically by accessing the web interface or resetting the device into configuration mode.

## Future Improvements
- Additional features for temperature or motion-based automation.
- Support for more relay channels or external devices.

## License
This project is released under the **MIT License**.

---

Happy tinkering! If you encounter any issues or have suggestions, feel free to contribute or reach out.
