/*********
  Name:Phiraphong A/L A Watt
  Matric.No:288584
  EEPROM with servo
*********/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Servo.h>

#define EEPROM_SIZE 512
#define SERVO_PIN D4

// EEPROM addresses for storing data
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASS_ADDR 64
#define EEPROM_DEVICE_ID_ADDR 128
#define EEPROM_SERVO_POS_ADDR 192
#define EEPROM_FLAG_ADDR 256

Servo servo;
ESP8266WebServer server(80); // Web server on port 80

// Read a string from EEPROM
String readStringFromEEPROM(int addr, int length) {
  char data[length + 1];
  for (int i = 0; i < length; i++) {
    data[i] = EEPROM.read(addr + i);
  }
  data[length] = '\0';
  return String(data);
}

// Write a string to EEPROM
void writeStringToEEPROM(int addr, String data) {
  int length = data.length();
  for (int i = 0; i < length; i++) {
    EEPROM.write(addr + i, data[i]);
  }
  for (int i = length; i < 32; i++) {
    EEPROM.write(addr + i, 0);
  }
  EEPROM.commit();
}

// Read an integer from EEPROM
int readIntFromEEPROM(int addr) {
  int value;
  EEPROM.get(addr, value);
  return value;
}

// Write an integer to EEPROM
void writeIntToEEPROM(int addr, int value) {
  EEPROM.put(addr, value);
  EEPROM.commit();
}

// Set up the Wi-Fi Access Point (AP mode)
void setupAP() {
  WiFi.softAP("PigeonNodeMCU");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

// Handle root URL, serve the configuration page
void handleRoot() {
  String page = "<html><head>";
  page += "<style>";
  page += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }";
  page += "h1 { color: #333; }";
  page += "form { margin-top: 20px; }";
  page += "label { display: block; margin-bottom: 5px; }";
  page += "input[type='text'], input[type='number'] { width: 100%; padding: 10px; margin-bottom: 20px; border: 1px solid #ccc; border-radius: 5px; }";
  page += "input[type='submit'] { padding: 10px 20px; border: none; border-radius: 5px; background-color: #5cb85c; color: white; font-size: 16px; cursor: pointer; }";
  page += "input[type='submit']:hover { background-color: #4cae4c; }";
  page += "</style>";
  page += "</head><body>";
  page += "<h1>ESP8266 Configuration</h1>";
  page += "<form action='/save' method='POST'>";
  page += "<label for='ssid'>SSID:</label>";
  page += "<input type='text' id='ssid' name='ssid' required><br>";
  page += "<label for='password'>Password:</label>";
  page += "<input type='text' id='password' name='password' required><br>";
  page += "<label for='device_id'>Device ID:</label>";
  page += "<input type='text' id='device_id' name='device_id' required><br>";
  page += "<label for='servo_pos'>Servo Position (0-180):</label>";
  page += "<input type='number' id='servo_pos' name='servo_pos' min='0' max='180' required><br>";
  page += "<input type='submit' value='Save'>";
  page += "</form>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

// Handle form submission, save configuration to EEPROM
void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String device_id = server.arg("device_id");
  int servo_pos = server.arg("servo_pos").toInt();

  writeStringToEEPROM(EEPROM_SSID_ADDR, ssid);
  writeStringToEEPROM(EEPROM_PASS_ADDR, password);
  writeStringToEEPROM(EEPROM_DEVICE_ID_ADDR, device_id);
  writeIntToEEPROM(EEPROM_SERVO_POS_ADDR, servo_pos);
  EEPROM.write(EEPROM_FLAG_ADDR, 1); // Set flag to indicate configuration is saved
  EEPROM.commit();

  servo.write(servo_pos);

  // Show configuration saved page with button to return to main page
  String page = "<html><body>";
  page += "<style>";
  page += "h1 { color: #333; }";
  page += "</style>";
  page += "<h1>Saved configuration! Restarting...</h1>";
  page += "<button onclick=\"window.location.href='/'\">Go Back to Main Page</button>";
  page += "</body></html>";
  server.send(200, "text/html", page);

  delay(2000);
  ESP.restart();
}

// Initialize EEPROM and set up the servo
void initializeSystem() {
  EEPROM.begin(EEPROM_SIZE);
  servo.attach(SERVO_PIN);

  if (EEPROM.read(EEPROM_FLAG_ADDR) == 1) { // Check if configuration is saved in EEPROM
    String ssid = readStringFromEEPROM(EEPROM_SSID_ADDR, 32);
    String password = readStringFromEEPROM(EEPROM_PASS_ADDR, 32);
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) { // 20 seconds timeout
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      int servo_pos = readIntFromEEPROM(EEPROM_SERVO_POS_ADDR);
      servo.write(servo_pos);
      Serial.print("Servo position set to: ");
      Serial.println(servo_pos);
    } else {
      Serial.println("Failed to connect to WiFi, starting AP mode.");
      setupAP();
    }
  } else {
    Serial.println("No configuration found, starting AP mode.");
    setupAP();
  }
}

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.println("Initializing system...");
  
  initializeSystem();

  server.on("/", handleRoot); // Handle the root URL
  server.on("/save", HTTP_POST, handleSave); // Handle form submission
  server.begin();
  Serial.println("HTTP server started");

  // Print the IP address after server initialization
  Serial.print("ESP8266 Web Server's IP address: ");
  Serial.println(WiFi.localIP());
}

// Loop function
void loop() {
  server.handleClient(); // Handle client requests
}
