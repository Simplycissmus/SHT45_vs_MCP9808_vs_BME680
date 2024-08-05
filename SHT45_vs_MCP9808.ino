#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BME680.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Baerchenplatz-IoT";
const char* password = "elegantstar351";

// Web server on port 80
WebServer server(80);

// Sensor objects
Adafruit_MCP9808 mcp9808 = Adafruit_MCP9808();
Adafruit_SHT4x sht45 = Adafruit_SHT4x();
Adafruit_BME680 bme680 = Adafruit_BME680();

// I2C addresses
uint8_t mcp9808Address = 0x18;
uint8_t sht45Address = 0x44;
uint8_t bme680Address = 0x77;

// GPIO pin for battery voltage measurement
const int batteryPin = 3; // GPIO3

// Function prototypes
void handleRoot();
void handleData();
void handleAddresses();
float readBatteryVoltage();
float calculateBatteryPercentage(float voltage);
void calibrateBME680();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize sensors
  if (!mcp9808.begin(mcp9808Address)) {
    Serial.println("MCP9808 not found!");
  }
  if (!sht45.begin()) {
    Serial.println("SHT45 not found!");
  }
  if (!bme680.begin(bme680Address)) {
    Serial.println("BME680 not found!");
  }

  sht45.setPrecision(SHT4X_HIGH_PRECISION);
  sht45.setHeater(SHT4X_NO_HEATER);

  // Set up BME680
  bme680.setTemperatureOversampling(BME680_OS_8X);
  bme680.setHumidityOversampling(BME680_OS_2X);
  bme680.setPressureOversampling(BME680_OS_4X);
  bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme680.setGasHeater(320, 150); // 320°C for 150 ms

  // Calibrate BME680
  calibrateBME680();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Define HTTP server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/addresses", handleAddresses);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Multi-Sensor Data Visualization and Analysis</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/moment@2.24.0"></script>
    <script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-moment@1.0.0"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }
        .container { width: 95%; margin: 10px auto; background-color: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1, h2 { color: #333; text-align: center; margin: 10px 0; }
        .chart-container { height: 300px; margin-bottom: 20px; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; font-size: 0.9em; }
        th { background-color: #f2f2f2; }
        tr:nth-child(even) { background-color: #f9f9f9; }
        .mcp9808 { color: red; }
        .sht45 { color: blue; }
        .bme680 { color: orange; }
    </style>
</head>
<body>
    <h1>Multi-Sensor Data and Analysis</h1>
    <div class="container">
        <h2>Temperature Data</h2>
        <div class="chart-container"><canvas id="tempChart"></canvas></div>
    </div>
    <div class="container">
        <h2>Humidity Data</h2>
        <div class="chart-container"><canvas id="humChart"></canvas></div>
    </div>
    <div class="container">
        <h2>Current Readings</h2>
        <table id="readingsTable">
            <tr><th>Sensor</th><th>Temperature (°C)</th><th>Humidity (%)</th><th>Pressure (hPa)</th><th>Gas Resistance (KΩ)</th><th>Battery Voltage (V)</th><th>Battery Charge (%)</th></tr>
            <tr><td class="mcp9808">MCP9808</td><td id="tempMCP9808"></td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td></tr>
            <tr><td class="sht45">SHT45</td><td id="tempSHT45"></td><td id="humSHT45"></td><td>-</td><td>-</td><td>-</td><td>-</td></tr>
            <tr><td class="bme680">BME680</td><td id="tempBME680"></td><td id="humBME680"></td><td id="presBME680"></td><td id="gasBME680"></td><td id="batVoltage"></td><td id="batCharge"></td></tr>
        </table>
    </div>
    <div class="container">
        <h2>Sensor Addresses</h2>
        <table id="addressesTable">
            <tr><th>Device</th><th>Address</th></tr>
            <tr><td class="mcp9808">MCP9808</td><td id="mcp9808Address"></td></tr>
            <tr><td class="sht45">SHT45</td><td id="sht45Address"></td></tr>
            <tr><td class="bme680">BME680</td><td id="bme680Address"></td></tr>
        </table>
    </div>
    <script>
        let tempChart, humChart;
        const tempData = {
            labels: [],
            datasets: [
                {label: 'MCP9808 Temp', data: [], borderColor: 'red'},
                {label: 'SHT45 Temp', data: [], borderColor: 'blue'},
                {label: 'BME680 Temp', data: [], borderColor: 'orange'}
            ]
        };
        const humData = {
            labels: [],
            datasets: [
                {label: 'SHT45 Humidity', data: [], borderColor: 'green'},
                {label: 'BME680 Humidity', data: [], borderColor: 'purple'}
            ]
        };

        function updateTempChart() {
            if (tempChart) {
                tempChart.destroy();
            }
            const ctx = document.getElementById('tempChart').getContext('2d');
            tempChart = new Chart(ctx, {
                type: 'line',
                data: tempData,
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        x: {type: 'time', time: {unit: 'second'}},
                        y: {type: 'linear', display: true, position: 'left', title: {display: true, text: 'Temperature (°C)'}}
                    }
                }
            });
        }

        function updateHumChart() {
            if (humChart) {
                humChart.destroy();
            }
            const ctx = document.getElementById('humChart').getContext('2d');
            humChart = new Chart(ctx, {
                type: 'line',
                data: humData,
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        x: {type: 'time', time: {unit: 'second'}},
                        y: {type: 'linear', display: true, position: 'left', title: {display: true, text: 'Humidity (%)'}}
                    }
                }
            });
        }

        function fetchDataAndUpdate() {
            fetch('/data')
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`HTTP error! status: ${response.status}`);
                    }
                    return response.json();
                })
                .then(data => {
                    console.log("Received Data:", data);
                    const now = new Date();
                    tempData.labels.push(now);
                    tempData.datasets[0].data.push(data.tempMCP9808);
                    tempData.datasets[1].data.push(data.tempSHT45);
                    tempData.datasets[2].data.push(data.tempBME680);
                    humData.labels.push(now);
                    humData.datasets[0].data.push(data.humSHT45);
                    humData.datasets[1].data.push(data.humBME680);

                    if (tempData.labels.length > 50) {
                        tempData.labels.shift();
                        tempData.datasets.forEach(dataset => dataset.data.shift());
                        humData.labels.shift();
                        humData.datasets.forEach(dataset => dataset.data.shift());
                    }

                    updateTempChart();
                    updateHumChart();

                    document.getElementById('tempMCP9808').textContent = data.tempMCP9808 ? data.tempMCP9808.toFixed(2) : 'N/A';
                    document.getElementById('tempSHT45').textContent = data.tempSHT45 ? data.tempSHT45.toFixed(2) : 'N/A';
                    document.getElementById('humSHT45').textContent = data.humSHT45 ? data.humSHT45.toFixed(2) : 'N/A';
                    document.getElementById('tempBME680').textContent = data.tempBME680 ? data.tempBME680.toFixed(2) : 'N/A';
                    document.getElementById('humBME680').textContent = data.humBME680 ? data.humBME680.toFixed(2) : 'N/A';
                    document.getElementById('presBME680').textContent = data.presBME680 ? data.presBME680.toFixed(2) : 'N/A';
                    document.getElementById('gasBME680').textContent = data.gasBME680 ? data.gasBME680.toFixed(2) : 'N/A';
                    document.getElementById('batVoltage').textContent = data.batVoltage ? data.batVoltage.toFixed(2) : 'N/A';
                    document.getElementById('batCharge').textContent = data.batCharge ? data.batCharge.toFixed(2) : 'N/A';
                })
                .catch(error => console.error('Error fetching data:', error));

            fetch('/addresses')
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`HTTP error! status: ${response.status}`);
                    }
                    return response.json();
                })
                .then(addresses => {
                    console.log("Received Addresses:", addresses);
                    document.getElementById('mcp9808Address').textContent = '0x' + addresses.mcp9808.toString(16).padStart(2, '0');
                    document.getElementById('sht45Address').textContent = '0x' + addresses.sht45.toString(16).padStart(2, '0');
                    document.getElementById('bme680Address').textContent = '0x' + addresses.bme680.toString(16).padStart(2, '0');
                })
                .catch(error => console.error('Error fetching addresses:', error));
        }

        fetchDataAndUpdate();
        setInterval(fetchDataAndUpdate, 5000); // Update every 5 seconds
    </script>
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

void handleAddresses() {
  DynamicJsonDocument doc(256);
  doc["mcp9808"] = mcp9808Address;
  doc["sht45"] = sht45Address;
  doc["bme680"] = bme680Address;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleData() {
  DynamicJsonDocument doc(1024);

  // Read MCP9808
  float tempMCP9808 = mcp9808.readTempC();
  doc["tempMCP9808"] = isnan(tempMCP9808) ? NAN : tempMCP9808;

  // Read SHT45
  sensors_event_t humidity, temp;
  sht45.getEvent(&humidity, &temp);
  doc["tempSHT45"] = temp.temperature;
  doc["humSHT45"] = humidity.relative_humidity;

  // Read BME680
  if (bme680.performReading()) {
    doc["tempBME680"] = bme680.temperature;
    doc["humBME680"] = bme680.humidity;
    doc["presBME680"] = bme680.pressure / 100.0; // Convert to hPa
    doc["gasBME680"] = bme680.gas_resistance / 1000.0; // Convert to KΩ
  } else {
    doc["tempBME680"] = NAN;
    doc["humBME680"] = NAN;
    doc["presBME680"] = NAN;
    doc["gasBME680"] = NAN;
  }

  // Read battery voltage
  float batteryVoltage = readBatteryVoltage();
  doc["batVoltage"] = batteryVoltage;
  doc["batCharge"] = calculateBatteryPercentage(batteryVoltage);

  Serial.println("Data:");
  Serial.print("MCP9808 Temperature: ");
  Serial.println(tempMCP9808);
  Serial.print("SHT45 Temperature: ");
  Serial.println(temp.temperature);
  Serial.print("SHT45 Humidity: ");
  Serial.println(humidity.relative_humidity);
  Serial.print("BME680 Temperature: ");
  Serial.println(bme680.temperature);
  Serial.print("BME680 Humidity: ");
  Serial.println(bme680.humidity);
  Serial.print("BME680 Pressure: ");
  Serial.println(bme680.pressure / 100.0);
  Serial.print("BME680 Gas Resistance: ");
  Serial.println(bme680.gas_resistance / 1000.0);
  Serial.print("Battery Voltage: ");
  Serial.println(batteryVoltage);
  Serial.print("Battery Charge: ");
  Serial.println(calculateBatteryPercentage(batteryVoltage));

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

float readBatteryVoltage() {
  const float attenuation = 2.6667; // Korrekturfaktor für 100kΩ und 60kΩ Widerstände
  int raw = analogRead(batteryPin); // Verwende den definierten batteryPin
  float voltage = (raw / 4095.0) * 3.3 * attenuation;
  return voltage;
}

float calculateBatteryPercentage(float voltage) {
  if (voltage > 4.2) return 100.0;
  if (voltage < 3.0) return 0.0;
  return (voltage - 3.0) / (4.2 - 3.0) * 100.0;
}

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
