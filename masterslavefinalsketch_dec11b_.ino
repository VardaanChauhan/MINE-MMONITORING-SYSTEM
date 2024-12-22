#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Sensor and pin definitions
#define DHT_PIN 22             // GPIO 22 for the DHT22 sensor
#define MQ2_PIN 35             // GPIO 35 for the MQ2 gas sensor
#define BUZZER_PIN 23          // GPIO 23 for the buzzer
#define DHT_TYPE DHT22         // DHT22 sensor type

DHT dht(DHT_PIN, DHT_TYPE);

// Threshold values
const float TEMP_THRESHOLD = 30.0;    // Temperature threshold in °C
const float HUMIDITY_THRESHOLD = 70.0; // Humidity threshold in %
const int GAS_THRESHOLD = 3000;       // Gas sensor threshold value (analog reading)

// Wi-Fi credentials
const char* ssid = "V";       // Replace with your Wi-Fi SSID
const char* password = "123456789"; // Replace with your Wi-Fi password

// Hub URL
const char* hub_url = "http://192.168.157.135/data"; // Replace with your Hub's IP address

// Data structure to store and send sensor data
typedef struct {
  char id[10];         // Device ID
  float temperature;   // Temperature in °C
  float humidity;      // Humidity in %
  int gasLevel;        // Gas level (analog value)
} SensorData;

SensorData masterSlaveData;        // Local sensor data
SensorData slave1Data = {"SLAVE01", 0, 0, 0}; // Initialize Slave 1 data
SensorData slave2Data = {"SLAVE02", 0, 0, 0}; // Initialize Slave 2 data

// Function to check thresholds and activate buzzer
void checkThresholdsAndActivateBuzzer(float temp, float hum, int gas, String source) {
  if (temp > TEMP_THRESHOLD || hum > HUMIDITY_THRESHOLD || gas > GAS_THRESHOLD) {
    Serial.println("ALERT: Threshold exceeded in " + source + "! Activating buzzer.");
    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
  } else {
    Serial.println("All readings within safe limits for " + source + ".");
    digitalWrite(BUZZER_PIN, LOW);  // Turn off buzzer
  }
}

// ESP-NOW callback function
void onDataReceive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
  Serial.print("Debug");
  SensorData receivedData;
  memcpy(&receivedData, data, sizeof(receivedData));

  // Store data based on Slave ID
  if (strcmp(receivedData.id, "SLAVE01") == 0) {
    slave1Data = receivedData;
  } else if (strcmp(receivedData.id, "SLAVE02") == 0) {
    slave2Data = receivedData;
  }

  // Print received data to Serial Monitor
  Serial.println("Data received via ESP-NOW:");
  Serial.printf("ID: %s, Temperature: %.2f°C, Humidity: %.2f%%, Gas Level: %d\n",
                receivedData.id, receivedData.temperature, receivedData.humidity, receivedData.gasLevel);

  // Check thresholds for received data
  checkThresholdsAndActivateBuzzer(receivedData.temperature, receivedData.humidity, receivedData.gasLevel, receivedData.id);
}


// Function to create JSON payload for Hub
String createJSONPayload() {
  String payload = "{";
  payload += "\"masterSlave\":{\"id\":\"MASTER\",";
  payload += "\"temperature\":" + String(masterSlaveData.temperature) + ",";
  payload += "\"humidity\":" + String(masterSlaveData.humidity) + ",";
  payload += "\"gasLevel\":" + String(masterSlaveData.gasLevel) + "},";

  payload += "\"slave1\":{\"id\":\"SLAVE1\",";
  payload += "\"temperature\":" + String(slave1Data.temperature) + ",";
  payload += "\"humidity\":" + String(slave1Data.humidity) + ",";
  payload += "\"gasLevel\":" + String(slave1Data.gasLevel) + "},";

  payload += "\"slave2\":{\"id\":\"SLAVE2\",";
  payload += "\"temperature\":" + String(slave2Data.temperature) + ",";
  payload += "\"humidity\":" + String(slave2Data.humidity) + ",";
  payload += "\"gasLevel\":" + String(slave2Data.gasLevel) + "}";

  payload += "}";
  return payload;
}

void setup() {
  Serial.begin(115200);

  // Initialize sensors and buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  dht.begin();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }
  esp_err_t result = esp_now_register_recv_cb(onDataReceive);
  if(result == ESP_OK)
    Serial.print("OK\n");
  else
    Serial.print("Err\n");

  Serial.println("Master-Slave module setup completed. Ready to monitor and send data to Hub.");
}

void loop() {
  //Read local sensors
  masterSlaveData.temperature = dht.readTemperature();
  masterSlaveData.humidity = dht.readHumidity();
  masterSlaveData.gasLevel = analogRead(MQ2_PIN);
  strcpy(masterSlaveData.id, "MASTER");

  // Validate sensor readings
  if (isnan(masterSlaveData.temperature) || isnan(masterSlaveData.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(5000); // Wait and retry
    return;
  }

  // Print local sensor data to Serial Monitor
  Serial.printf("Local Sensor Data - Temperature: %.2f°C, Humidity: %.2f%%, Gas Level: %d\n",
                masterSlaveData.temperature, masterSlaveData.humidity, masterSlaveData.gasLevel);

  // Check thresholds for local data
  checkThresholdsAndActivateBuzzer(masterSlaveData.temperature, masterSlaveData.humidity, masterSlaveData.gasLevel, "Local");

  // Create JSON payload for Hub
  String jsonPayload = createJSONPayload();

  // Send data to the Hub via HTTP POST
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(hub_url);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.print("Data sent to Hub, response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending data to Hub: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected. Unable to send data to Hub.");
  }

  delay(5000); // Delay before next iteration
}
