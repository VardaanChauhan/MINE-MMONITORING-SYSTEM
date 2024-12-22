//Slave
#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

// Define sensor pins and types
#define DHT_PIN 22          // DHT22 pin
#define MQ2_PIN 35          // MQ2 sensor pin
#define BUZZER_PIN 23       // Buzzer pin
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

// Threshold values
const float TEMP_THRESHOLD = 30.0;
const float HUMIDITY_THRESHOLD = 70.0;
const int GAS_THRESHOLD = 3000;

// Data structure to send
typedef struct {
  char id[10];         // Device ID
  float temperature;   // Temperature in °C
  float humidity;      // Humidity in %
  int gasLevel;        // Gas level (analog value)
} SensorData;

SensorData dataToSend;

// Master-Slave MAC address (replace with actual MAC)
uint8_t masterSlaveMAC[] = {0x8c, 0x4f, 0x00, 0x11, 0xf2, 0x14}; // Replace with the correct Master-Slave MAC

// ESP-NOW send callback
void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully!" : "Data send failed.");
}

// Function to check thresholds and activate buzzer
void checkThresholdsAndActivateBuzzer(float temperature, float humidity, int gasLevel) {
  if (temperature > TEMP_THRESHOLD || humidity > HUMIDITY_THRESHOLD || gasLevel > GAS_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
    Serial.println("Buzzer activated: Threshold exceeded!");
  } else {
    digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize sensors and buzzer
  dht.begin();
  pinMode(MQ2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize Wi-Fi in Station mode for ESP-NOW
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }
  esp_now_register_send_cb(onDataSent);

  // Add Master-Slave module as a peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, masterSlaveMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Slave module setup completed.");
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gasLevel = analogRead(MQ2_PIN);

  // Validate sensor readings
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(5000); // Retry after a delay
    return;
  }

  // Log sensor readings
  Serial.println("Sensor Data:");
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Gas Level: "); Serial.println(gasLevel);

  // Check thresholds and activate buzzer
  checkThresholdsAndActivateBuzzer(temperature, humidity, gasLevel);

  // Populate the data structure
  strcpy(dataToSend.id, "SLAVE02"); // Unique ID for this Slave
  dataToSend.temperature = temperature;
  dataToSend.humidity = humidity;
  dataToSend.gasLevel = gasLevel;

  // Send data to Master-Slave module via ESP-NOW
  esp_now_send(masterSlaveMAC, (uint8_t*)&dataToSend, sizeof(dataToSend));

  // Delay before the next loop iteration
  delay(5000); // Send data every 5 seconds
}
