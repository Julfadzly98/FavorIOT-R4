#include <WiFiS3.h>            // WiFi library for Arduino R4
#include <Wire.h>              // I2C communication library
#include <LiquidCrystal_I2C.h>  // LCD display library
#include <SimpleDHT.h>          // DHT11 sensor library

// Favoriot API configurations
#define APIKEY "your_api_key_here" // Replace with your actual Favoriot API key
#define DEVICE_DEV_ID "AgriR4_1_device@einstronics" // Replace with your device ID

const char* ssid = "Einstronic-2.4G";     // Replace with your WiFi SSID
const char* password = "*Eins1014#";      // Replace with your WiFi password

// Pin assignments
#define MAINTENANCE_PIN 12
#define RELAY_PIN 11
#define LED_GREEN 7
#define LED_YELLOW 6
#define LED_RED 5
#define DHT11_PIN 10
#define SOIL_MOISTURE_PIN A0

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT11 setup
SimpleDHT11 dht11(DHT11_PIN);

// Variables for sensor readings and state
bool maintenanceMode = false;
int soilMoistureLevel = 0;
int temperature = 0;
int humidity = 0;

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(MAINTENANCE_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  // Connect to Wi-Fi
  connectToWiFi();

  // Initial Favoriot API request
  sendFavoriotData(0, 0, 0);
}

void loop() {
  // Check for maintenance button press
  if (digitalRead(MAINTENANCE_PIN) == LOW) {
    delay(200); // Debounce delay
    maintenanceMode = !maintenanceMode; // Toggle maintenance mode
  }

  // If in maintenance mode, disable system
  if (maintenanceMode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Maintenance Mode");
    turnOffSystem();
    delay(1000);
    return;
  }

  // Read the sensors
  soilMoistureLevel = analogRead(SOIL_MOISTURE_PIN);
  readDHT11();

  // Control LEDs based on soil moisture
  controlLEDs(soilMoistureLevel);

  // Display values on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(temperature);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Soil:");
  lcd.print(soilMoistureLevel);

  // Send data to Favoriot
  sendFavoriotData(temperature, humidity, soilMoistureLevel);

  delay(5000); // Delay between readings
}

// Function to read DHT11 sensor
void readDHT11() {
  byte temp = 0;
  byte hum = 0;
  dht11.read(&temp, &hum, NULL);
  temperature = (int)temp;
  humidity = (int)hum;
}

// Function to control LEDs based on soil moisture
void controlLEDs(int moisture) {
  if (moisture >= 90) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
  } else if (moisture >= 50 && moisture < 90) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
  }
}

// Function to turn off all outputs (for maintenance mode)
void turnOffSystem() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(RELAY_PIN, LOW);
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");
}

// Function to send data to Favoriot platform using WiFiClient
void sendFavoriotData(int temp, int hum, int soil) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    const char* server = "apiv2.favoriot.com";
    const int port = 80;

    // API endpoint URL
    String url = "/v2/streams";

    // Construct the JSON payload
    String payload = "{\"device_developer_id\":\"" + String(DEVICE_DEV_ID) + "\",";
    payload += "\"data\":{\"temperature\":" + String(temp) + ",";
    payload += "\"humidity\":" + String(hum) + ",";
    payload += "\"soilMoisture\":" + String(soil) + "}}";

    // Connect to the server
    if (client.connect(server, port)) {
      // Construct the HTTP request
      client.println("POST " + url + " HTTP/1.1");
      client.println("Host: " + String(server));
      client.println("Content-Type: application/json");
      client.println("apikey: " + String(APIKEY));
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.println(payload);

      // Wait for response from server
      while (client.connected() && !client.available()) {
        delay(100);
      }

      // Read and print the response from the server
      String response = client.readString();
      Serial.println(response);

      // Close the connection
      client.stop();
    } else {
      Serial.println("Connection to server failed.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}
