#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <FS.h>
#include <SPIFFS.h>
#include "html1.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h>
#define SWITCH_PIN 12
#define RED_LED_PIN 13

float lastAccelMagnitude = 0;
bool waitingForCancel = false;
unsigned long fallTriggerTime = 0;
bool fallCanceledViaWeb = false;

const char* ssid = ".x";
const char* password = "kashyap1";

// Twilio credentials
const char* twilio_sid = "";
const char* twilio_auth  = "";
String from_number      = "";  // Twilio phone number
String to_number        = "";  // Your mobile number

// Web server on port 80
WebServer server(80);
String lat = "17.4931";              // Set dynamically if you have a GPS
String lon = "78.386";
// ADXL345 Accelerometer
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Variables for sensor readings
float roll = 0.0, pitch = 0.0, yaw = 0.0;

// Fall detection variables
bool fallDetected = false;
unsigned long lastFallTime = 0;
const unsigned long FALL_COOLDOWN = 30000; // 30 seconds cooldown

void calculateAngles(float x, float y, float z) {
    roll = atan2(y, z) * 180.0 / PI;
    pitch = atan2(-x, sqrt(y * y + z * z)) * 180.0 / PI;
    yaw = atan2(z, sqrt(x * x + y * y)) * 180.0 / PI;
}

float calculateAccelMagnitude(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

void sendTwilioSMS(String message, float impact) {

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String twilio_url = "https://api.twilio.com/2010-04-01/Accounts/" + String(twilio_sid) + "/Messages.json";
        http.begin(twilio_url);

        String auth = String(twilio_sid) + ":" + String(twilio_auth);
        String encodedAuth = base64::encode(auth);
        http.addHeader("Authorization", "Basic " + encodedAuth);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Hardcoding latitude and longitude directly into the Google Maps URL
        String mapsLink = "https://maps.app.goo.gl/Gj5HDdDfKFyjZyUw9"; // Replace these with your actual values

        String smsMessage = "⚠️ Fall Detected!\n📍 Location:\n" + mapsLink + "\n💥 Impact: " + String(impact, 2) + " g";


        String postData = "To=" + String(to_number) + "&From=" + String(from_number) + "&Body=" + smsMessage;
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
    lastAccelMagnitude = accelMagnitude;

    bool currentFallState = (abs(event.acceleration.x) > 4.0 || 
                              abs(event.acceleration.y) > 4.0 || 
                              abs(event.acceleration.z) < 7.0);

    if (currentFallState && !fallDetected && (millis() - lastFallTime > FALL_COOLDOWN)) {
        fallDetected = true;
        lastFallTime = millis();
        fallTriggerTime = millis();
        waitingForCancel = true;
        Serial.println("🚨 Fall Detected! Waiting 5 seconds for manual cancel...");
    }

    if (waitingForCancel) {
    if (millis() - fallTriggerTime >= 5000) {
    bool switchStillPressed = true;

    // Check switch state over 500ms (10 samples)
    for (int i = 0; i < 10; i++) {
        if (digitalRead(SWITCH_PIN) == HIGH) {
            switchStillPressed = false;
        }
        delay(50);
    }

    if (switchStillPressed || fallCanceledViaWeb) {
        Serial.println("✅ Fall canceled by switch.");
        digitalWrite(RED_LED_PIN, LOW);  // OFF by default
    } else {
        String fallMessage = "🚨 FALL DETECTED!\nNo response in 5 seconds.\nSending alert now!";
        sendTwilioSMS(fallMessage, lastAccelMagnitude);
        Serial.println("✅ SMS sent with impact.");
        
        digitalWrite(RED_LED_PIN, HIGH);  // OFF by default

    }

    waitingForCancel = false;
    fallCanceledViaWeb = false;
}

}


    if (!currentFallState && (millis() - lastFallTime > 5000)) {
        fallDetected = false;
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
    server.on("/getLocation", HTTP_GET, []() {
    StaticJsonDocument<100> doc;
    doc["lat"] = 17.5406;
    doc["lng"] = 78.386;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
});

//});

}

void setup() {
    Serial.begin(9600);
    pinMode(SWITCH_PIN, INPUT_PULLUP); // Switch ON = LOW
    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);  // OFF by default


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

    // 🔽 Add this line below the other routes
    server.on("/resetFall", HTTP_GET, []() {
    fallCanceledViaWeb = true;  // Add this line
    fallDetected = false;
    waitingForCancel = false;
    //digitalWrite(RED_PIN, LOW); // Turn OFF RED LED
    //digitalWrite(buzz, LOW);
    server.send(200, "text/plain", "Fall reset");
});


    server.begin();
    Serial.println("System Ready!");
}


void loop() {
    checkForFall();
    server.handleClient();
    delay(100);
}
