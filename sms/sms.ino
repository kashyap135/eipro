#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ADXL345_U.h>
#include <FS.h>
#include <SPIFFS.h>
#include "html1.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h>

// WiFi credentials
const char* ssid = ".x";
const char* password = "kashyap1";

// Twilio Credentials
const char* twilio_sid = "AC029725055d9622864711d9e60c7c11f7";  
const char* twilio_auth = "9fe3a2c83d2cf67565ad19395ae16c9d";  
const char* to_number = "+918247567314";
const char* from_number = "+18289038683";

// Web server on port 80
WebServer server(80);

// DHT sensor configuration
#define DHTPIN 27
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

// OLED display setup 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ADXL345 Accelerometer
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Variables for sensor readings
float roll = 0.0, pitch = 0.0, yaw = 0.0;
float humidity = 0.0, temperature = 0.0;

// Variables for fall detection
bool fallDetected = false;
unsigned long lastFallTime = 0;
const unsigned long FALL_COOLDOWN = 30000; // 30 seconds cooldown between alerts

void calculateAngles(float x, float y, float z) {
    roll = atan2(y, z) * 180.0 / PI;
    pitch = atan2(-x, sqrt(y * y + z * z)) * 180.0 / PI;
    yaw = atan2(z, sqrt(x * x + y * y)) * 180.0 / PI;
}

float calculateAccelMagnitude(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

void sendTwilioSMS(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String twilio_url = "https://api.twilio.com/2010-04-01/Accounts/" + String(twilio_sid) + "/Messages.json";
        http.begin(twilio_url);

        String auth = String(twilio_sid) + ":" + String(twilio_auth);
        String encodedAuth = base64::encode(auth);
        http.addHeader("Authorization", "Basic " + encodedAuth);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "To=" + String(to_number) + "&From=" + String(from_number) + "&Body=" + message;

        int httpResponseCode = http.POST(postData);
        
        if (httpResponseCode == 201) {
            Serial.println("✅ Fall Alert SMS Sent Successfully!");
        } else {
            Serial.println("❌ Error Sending SMS. Response Code: " + String(httpResponseCode));
        }

        http.end();
    }
}

void checkForFall() {
    sensors_event_t event;
    accel.getEvent(&event);
    
    float accelMagnitude = calculateAccelMagnitude(
        event.acceleration.x,
        event.acceleration.y,
        event.acceleration.z
    );
    
    // Fall detection logic - using moderate thresholds
    bool currentFallState = (abs(event.acceleration.x) > 4.0 || 
                           abs(event.acceleration.y) > 4.0 || 
                           abs(event.acceleration.z) < 7.0);
    
    // Check if this is a new fall and enough time has passed since the last alert
    if (currentFallState && !fallDetected && (millis() - lastFallTime > FALL_COOLDOWN)) {
        fallDetected = true;
        lastFallTime = millis();
        
        String fallMessage = "🚨 FALL DETECTED!\n"
                           "Acceleration: " + String(accelMagnitude, 1) + "g\n"
                           "Location: [Add location if needed]\n"
                           "Please check immediately!";
        
        sendTwilioSMS(fallMessage);
        
        // Update display with fall alert
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println("FALL");
        display.println("DETECTED!");
        display.setTextSize(1);
        display.setCursor(0, 40);
        display.print("ACC: ");
        display.print(accelMagnitude, 1);
        display.print("g");
        display.display();
        delay(2000);
    }
    else if (!currentFallState) {
        if (millis() - lastFallTime > 5000) { // Reset after 5 seconds of normal readings
            fallDetected = false;
        }
    }
}

void MainPage() {
    server.send(200, "text/html", html_page);
}

void ADXL345html() {
    sensors_event_t event;
    accel.getEvent(&event);
    
    calculateAngles(event.acceleration.x, event.acceleration.y, event.acceleration.z);
    
    String status = fallDetected ? "Fall Detected" : "Normal";
    
    String data = "[" + String(roll, 2) + "," + 
                      String(pitch, 2) + "," + 
                      String(yaw, 2) + ",\"" + 
                      status + "\"]";
                      
    server.send(200, "application/json", data);
}

void DHT_Data() {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    
    if (isnan(humidity) || isnan(temperature)) {
        humidity = 0.0;
        temperature = 0.0;
    }
    
    String data = "[" + String(temperature, 2) + "," + String(humidity, 2) + "]";
    server.send(200, "application/json", data);
}

void displayData() {
    sensors_event_t event;
    accel.getEvent(&event);
    
    float accelMagnitude = calculateAccelMagnitude(
        event.acceleration.x,
        event.acceleration.y,
        event.acceleration.z
    );

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("ACC: ");
    display.print(accelMagnitude, 1);
    display.print("g");
    
    display.setCursor(0, 10);
    display.print("Temp: ");
    display.print(temperature);
    display.print("C");
    
    display.setCursor(0, 20);
    display.print("X: ");
    display.print(event.acceleration.x, 1);
    
    display.setCursor(0, 30);
    display.print("Y: ");
    display.print(event.acceleration.y, 1);
    
    display.setCursor(0, 40);
    display.print("Z: ");
    display.print(event.acceleration.z, 1);
    
    display.setCursor(0, 50);
    display.print("Status: ");
    display.print(fallDetected ? "FALL!" : "Normal");
    
    display.display();
}

void setup() {
    Serial.begin(9600);
    dht.begin();
    
    Wire.begin();
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("Initializing...");
    display.display();
    
    if (!accel.begin()) {
        Serial.println("ADXL345 not detected!");
        while (1);
    }
    accel.setRange(ADXL345_RANGE_16_G);
    
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    server.on("/", MainPage);
    server.on("/readADXL345", ADXL345html);
    server.on("/readDHT", DHT_Data);
    server.begin();
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("System Ready!");
    display.display();
    delay(2000);
}

void loop() {
    checkForFall();
    displayData();
    server.handleClient();
    delay(100);
}