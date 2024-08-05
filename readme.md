# Multi-Sensor Data Visualization and Analysis

This project demonstrates how to use the Seeed Studio XIAO ESP32C3 to gather and visualize data from multiple sensors (MCP9808, SHT45, BME680) and a battery voltage measurement using a voltage divider.

## Features

- **WiFi Connectivity**: Connects to a specified WiFi network.
- **Web Server**: Hosts a web server on port 80 to serve a webpage with sensor data visualization.
- **Sensor Data**: Reads data from MCP9808 (temperature), SHT45 (temperature and humidity), and BME680 (temperature, humidity, pressure, and gas resistance).
- **Battery Voltage Measurement**: Reads battery voltage using a voltage divider circuit.
- **Calibration Phase**: Calibrates BME680 sensor during startup to ensure accurate readings.

## Hardware Requirements

- Seeed Studio XIAO ESP32C3
- MCP9808 Temperature Sensor
- SHT45 Temperature and Humidity Sensor
- BME680 Environmental Sensor
- Voltage Divider (100kΩ and 60kΩ resistors)
- Breadboard and jumper wires
- USB Type-C cable

## Circuit Diagram

### Voltage Divider for Battery Measurement
- **Resistor 1**: 100kΩ
- **Resistor 2**: 60kΩ

Connect the resistors in series between the battery positive terminal and ground. Connect the midpoint of the resistors to GPIO3 (A0) on the XIAO ESP32C3.

```
Battery Positive --- 100kΩ ---+--- 60kΩ --- Ground
                              |
                            GPIO3
```

## Software Setup

### Prerequisites

- Arduino IDE with ESP32 support installed.
- Required libraries: `WiFi`, `WebServer`, `Wire`, `Adafruit_MCP9808`, `Adafruit_SHT4x`, `Adafruit_BME680`, `ArduinoJson`.

### Installation

1. **Download and Install Libraries**: Ensure the following libraries are installed in your Arduino IDE.
   - `WiFi`
   - `WebServer`
   - `Wire`
   - `Adafruit_MCP9808`
   - `Adafruit_SHT4x`
   - `Adafruit_BME680`
   - `ArduinoJson`

2. **Clone or Download the Repository**: Download the source code from this repository.

3. **Configure WiFi Credentials**: Update the `ssid` and `password` variables in the code with your WiFi network credentials.

4. **Upload the Code**: Connect your XIAO ESP32C3 to your computer using a USB Type-C cable and upload the code using the Arduino IDE.

## Usage

1. **Power the XIAO ESP32C3**: Connect the XIAO ESP32C3 to a power source via USB or battery.

2. **Access the Web Interface**: Once the device is connected to WiFi, it will print the IP address to the Serial Monitor. Open a web browser and navigate to the provided IP address to view the sensor data and battery voltage.

## Code Explanation

### Calibration Phase

During the setup, the BME680 sensor undergoes a calibration phase to stabilize the readings. This ensures accurate environmental data before normal operation begins.

```cpp
void calibrateBME680() {
  Serial.println("Calibrating BME680...");

  // Wait for the BME680 to stabilize
  for (int i = 0; i < 10; i++) {
    if (bme680.performReading()) {
      Serial.print("Temperature: ");
      Serial.print(bme680.temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(bme680.humidity);
      Serial.print(" %, Pressure: ");
      Serial.print(bme680.pressure / 100.0);
      Serial.print(" hPa, Gas: ");
      Serial.print(bme680.gas_resistance / 1000.0);
      Serial.println(" KΩ");
    } else {
      Serial.println("Failed to perform reading :(");
    }
    delay(1000); // Wait 1 second between readings
  }

  Serial.println("BME680 calibration complete.");
}
```

### Battery Voltage Measurement

The battery voltage is measured using a voltage divider circuit connected to GPIO3. The voltage is then used to calculate the battery percentage.

```cpp
float readBatteryVoltage() {
  const float attenuation = 2.4706;
  int raw = analogRead(3);
  float voltage = (raw / 4095.0) * 3.3 * attenuation;
  return voltage;
}

float calculateBatteryPercentage(float voltage) {
  if (voltage > 4.2) return 100.0;
  if (voltage < 3.0) return 0.0;
  return (voltage - 3.0) / (4.2 - 3.0) * 100.0;
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.