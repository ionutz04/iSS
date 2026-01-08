/**
 * ESP8266 IoT Power Meter Client
 * Reads ACS37800 sensor data and sends to Telegraf HTTPS API
 * 
 * Data is sent in InfluxDB line protocol format to the bucket
 */

#include "SparkFun_ACS37800_Arduino_Library.h"
#include <Wire.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

// ==================== CONFIGURATION ====================
// WiFi credentials
const char* WIFI_SSID = "iiap2g";
const char* WIFI_PASSWORD = "ionutqwerty";

// Telegraf server configuration
const char* TELEGRAF_HOST = "192.168.88.55";  // e.g., "192.168.1.100"
const int TELEGRAF_PORT = 8443;
const char* TELEGRAF_ENDPOINT = "/telegraf";

// Authentication (must match .env on server)
const char* CUSTOMER_ID = "1";
const char* API_SECRET = "changeme_supersecretkey123";

// Device identification
const char* DEVICE_ID = "esp01";

// Data send interval (milliseconds)
const unsigned long SEND_INTERVAL = 5000;  // 5 seconds

// TLS Certificate fingerprint (SHA1 - required for ESP8266)
// Get from: docker exec tig_customer_telegraf openssl x509 -in /etc/telegraf/certs/cert.pem -noout -fingerprint -sha1
// Set to empty string "" to skip verification (INSECURE - dev only!)
const char* CERT_FINGERPRINT = "26:3C:71:94:A5:C8:93:A8:BE:24:E9:60:86:57:F0:BF:F9:B9:7E:C3";

// ==================== GLOBALS ====================
ACS37800 mySensor;
WiFiClientSecure wifiClient;
unsigned long lastSendTime = 0;

// ==================== FUNCTIONS ====================

/**
 * Connect to WiFi network
 */
void connectWiFi() {
  Serial.print(F("\nConnecting to WiFi: "));
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\nWiFi connected!"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nWiFi connection failed!"));
  }
}

/**
 * Initialize the ACS37800 sensor
 */
bool initSensor() {
  Wire.begin(2, 0);  // SDA=GPIO2, SCL=GPIO0
  
  if (mySensor.begin(0x61) == false) {
    Serial.println(F("ACS37800 not detected. Check connections."));
    return false;
  }
  
  // Configure sensor
  mySensor.setBypassNenable(false, true);
  mySensor.setDividerRes(4e6);
  mySensor.setSenseRes(2490);
  mySensor.setCurrentRange(90);
  mySensor.setI2Caddress(0x61);
  
  Serial.println(F("ACS37800 sensor initialized!"));
  return true;
}

/**
 * Create HTTP Basic Auth header value
 */
String createAuthHeader() {
  String credentials = String(CUSTOMER_ID) + ":" + String(API_SECRET);
  return "Basic " + base64::encode(credentials);
}

/**
 * Send sensor data to Telegraf
 * Data format: InfluxDB line protocol
 * Example: power,device=esp01,customer=1 voltage=220.5,current=1.2,pactive=264.6
 */
bool sendDataToTelegraf(float voltage, float current, float pactive, 
                        float preactive, float papparent, float pfactor) {
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi not connected, reconnecting..."));
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      return false;
    }
  }
  
  // Configure TLS - MUST be done before each connection
  wifiClient.setTimeout(15000);  // 15 second timeout
  
  if (strlen(CERT_FINGERPRINT) > 0) {
    // Use certificate fingerprint for verification (SHA1)
    wifiClient.setFingerprint(CERT_FINGERPRINT);
    Serial.println(F("Using certificate fingerprint verification"));
  } else {
    // INSECURE: Skip certificate verification (dev only!)
    wifiClient.setInsecure();
    Serial.println(F("WARNING: TLS certificate verification disabled!"));
  }
  
  // First, test raw connection
  Serial.print(F("Connecting to "));
  Serial.print(TELEGRAF_HOST);
  Serial.print(F(":"));
  Serial.println(TELEGRAF_PORT);
  
  if (!wifiClient.connect(TELEGRAF_HOST, TELEGRAF_PORT)) {
    Serial.println(F("TLS connection failed!"));
    Serial.print(F("Last SSL error: "));
    Serial.println(wifiClient.getLastSSLError());
    return false;
  }
  Serial.println(F("TLS connection established!"));
  
  HTTPClient https;
  
  // Build URL
  String url = "https://" + String(TELEGRAF_HOST) + ":" + String(TELEGRAF_PORT) + String(TELEGRAF_ENDPOINT);
  
  Serial.print(F("Sending to: "));
  Serial.println(url);
  
  if (!https.begin(wifiClient, url)) {
    Serial.println(F("HTTPClient begin failed"));
    return false;
  }
  
  // Set timeout
  https.setTimeout(10000);
  
  // Set headers
  https.addHeader("Content-Type", "text/plain");
  https.addHeader("Authorization", createAuthHeader());
  
  // Build InfluxDB line protocol payload
  // Format: measurement,tag1=value1,tag2=value2 field1=value,field2=value
  String payload = "power,device=" + String(DEVICE_ID) + ",customer=" + String(CUSTOMER_ID);
  payload += " voltage=" + String(voltage, 3);
  payload += ",current=" + String(current, 3);
  payload += ",power_active=" + String(pactive, 3);
  payload += ",power_reactive=" + String(preactive, 3);
  payload += ",power_apparent=" + String(papparent, 3);
  payload += ",power_factor=" + String(pfactor, 3);
  
  Serial.print(F("Payload: "));
  Serial.println(payload);
  
  // Send POST request
  int httpCode = https.POST(payload);
  
  if (httpCode > 0) {
    Serial.print(F("HTTP Response: "));
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_NO_CONTENT || httpCode == HTTP_CODE_OK) {
      Serial.println(F("Data sent successfully!"));
      https.end();
      return true;
    } else {
      Serial.print(F("Server error: "));
      Serial.println(https.getString());
    }
  } else {
    Serial.print(F("HTTP error: "));
    Serial.println(https.errorToString(httpCode));
  }
  
  https.end();
  return false;
}

/**
 * Read sensor and send data
 */
void readAndSendSensorData() {
  float volts = 0.0;
  float amps = 0.0;
  float pactive = 0.0;
  float preactive = 0.0;
  float papparent = 0.0;
  float pfactor = 0.0;
  bool posangle = 0;
  bool pospf = 0;

  // Read sensor values
  mySensor.readRMS(&volts, &amps);
  mySensor.readPowerActiveReactive(&pactive, &preactive);
  mySensor.readPowerFactor(&papparent, &pfactor, &posangle, &pospf);

  // Print to Serial for debugging
  Serial.println(F("\n========== Sensor Reading =========="));
  Serial.print(F("Voltage [V]: "));
  Serial.println(volts, 3);
  Serial.print(F("Current [A]: "));
  Serial.println(amps, 3);
  Serial.print(F("Active Power [W]: "));
  Serial.println(pactive, 3);
  Serial.print(F("Reactive Power [VAR]: "));
  Serial.println(preactive, 3);
  Serial.print(F("Apparent Power [VA]: "));
  Serial.println(papparent, 3);
  Serial.print(F("Power Factor: "));
  Serial.println(pfactor, 3);
  Serial.println(F("====================================="));

  // Send data to Telegraf
  sendDataToTelegraf(volts, amps, pactive, preactive, papparent, pfactor);
}

// ==================== MAIN ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println(F("\n\n========================================"));
  Serial.println(F("  ESP8266 IoT Power Meter Client"));
  Serial.println(F("  ACS37800 -> Telegraf -> InfluxDB"));
  Serial.println(F("========================================\n"));
  
  // Connect to WiFi
  connectWiFi();
  
  // Initialize sensor
  if (!initSensor()) {
    Serial.println(F("Sensor init failed! Halting..."));
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println(F("\nSetup complete! Starting data collection...\n"));
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send data at specified interval
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = currentTime;
    readAndSendSensorData();
  }
  
  // Handle WiFi reconnection if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi disconnected, attempting reconnect..."));
    connectWiFi();
  }
  
  delay(100);
}
