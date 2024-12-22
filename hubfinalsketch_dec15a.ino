//HUB CODE 
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
// Wi-Fi credentials
const char* ssid = "V";               // Replace with your Wi-Fi SSID
const char* password = "123456789";   // Replace with your Wi-Fi password

WebServer server(80); // Create an HTTP server on port 80

// Threshold values
const float TEMP_THRESHOLD = 30.0;
const float HUMIDITY_THRESHOLD = 70.0;
const int GAS_THRESHOLD = 3000;

// Data for Master-Slave and Slaves
String masterSlaveData = "{}";
String slave01Data = "{}";
String slave02Data = "{}";

// Function to generate the HTML webpage
String generateWebPage() {
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Mine Monitoring System</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #282c34; color: #f1f1f1; text-align: center; margin: 0; padding: 0; }";
  html += "h1 { color: #61dafb; margin-top: 20px; }";
  html += ".card { background-color: #20232a; margin: 20px auto; padding: 20px; border-radius: 8px; max-width: 400px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }";
  html += ".alert { color: red; font-weight: bold; }";
  html += ".data { text-align: left; margin: 10px auto; }";
  html += ".footer { margin-top: 20px; color: #888; }";
  html += "</style></head><body>";

  html += "<h1>Mine Monitoring System</h1>";

  // Master-Slave Data Card
  html += "<div class='card'><h2>Master-Slave Module</h2>";
  html += "<div class='data'>" + masterSlaveData + "</div></div>";

  // Slave 1 Data Card
  html += "<div class='card'><h2>Slave 1 Module</h2>";
  html += "<div class='data'>" + slave01Data + "</div></div>";

  // Slave 2 Data Card
  html += "<div class='card'><h2>Slave 2 Module</h2>";
  html += "<div class='data'>" + slave02Data + "</div></div>";

  html += "<div class='footer'>Updated in real-time | Powered by ESP32</div>";
  html += "</body></html>";
  return html;
}

// Root route for serving the webpage
void handleRoot() {
  server.send(200, "text/html", generateWebPage());
}

// Handle incoming data from Master-Slave
void handleData() {
  if (server.hasArg("plain")) {
    String receivedData = server.arg("plain");
    Serial.println("Data received:");
    Serial.println(receivedData);

    // Parse the JSON data
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, receivedData);
    if (!error) {
      // Process Master-Slave data
      if (doc.containsKey("masterSlave")) {
        String ms = "";
        float temp = doc["masterSlave"]["temperature"];
        float hum = doc["masterSlave"]["humidity"];
        int gas = doc["masterSlave"]["gasLevel"];
        ms += "Temperature: " + String(temp) + "°C<br>";
        ms += "Humidity: " + String(hum) + "%<br>";
        ms += "Gas Level: " + String(gas);
        if (temp > TEMP_THRESHOLD || hum > HUMIDITY_THRESHOLD || gas > GAS_THRESHOLD) {
          ms += "<br><span class='alert'>ALERT: Threshold Exceeded!</span>";
        }
        masterSlaveData = ms;
      }

      // Process Slave 1 data
      if (doc.containsKey("slave1")) {
        String s1 = "";
        float temp = doc["slave1"]["temperature"];
        float hum = doc["slave1"]["humidity"];
        int gas = doc["slave1"]["gasLevel"];
        s1 += "Temperature: " + String(temp) + "°C<br>";
        s1 += "Humidity: " + String(hum) + "%<br>";
        s1 += "Gas Level: " + String(gas);
        if (temp > TEMP_THRESHOLD || hum > HUMIDITY_THRESHOLD || gas > GAS_THRESHOLD) {
          s1 += "<br><span class='alert'>ALERT: Threshold Exceeded!</span>";
        }
        slave01Data = s1;
      }

      // Process Slave 2 data
      if (doc.containsKey("slave2")) {
        String s2 = "";
        float temp = doc["slave2"]["temperature"];
        float hum = doc["slave2"]["humidity"];
        int gas = doc["slave2"]["gasLevel"];
        s2 += "Temperature: " + String(temp) + "°C<br>";
        s2 += "Humidity: " + String(hum) + "%<br>";
        s2 += "Gas Level: " + String(gas);
        if (temp > TEMP_THRESHOLD || hum > HUMIDITY_THRESHOLD || gas > GAS_THRESHOLD) {
          s2 += "<br><span class='alert'>ALERT: Threshold Exceeded!</span>";
        }
        slave02Data = s2;
      }
    } else {
      Serial.println("JSON parsing failed!");
    }

    server.send(200, "text/plain", "Data received and processed.");
  } else {
    server.send(400, "text/plain", "No data received.");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up HTTP server routes
  server.on("/", handleRoot);       // Serve the webpage
  server.on("/data", HTTP_POST, handleData); // Handle incoming data

  // Start the server
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient(); // Handle HTTP requests
}
