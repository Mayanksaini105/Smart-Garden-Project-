#include <Wire.h>
#include <WiFi.h>
#include "secrets.h"
#include <Adafruit_Sensor.h>
#include <DHT.h> // Include the DHT library
#include <BlynkSimpleEsp32.h> // Include the Blynk library

// Pin definitions
const int moistureSensorPin = 15;     // Connect soil moisture sensor module to A0
const int ldrDigitalPin = 5;           // Digital pin for LDR module
const int ledStripRelayPin = 7;        // Digital pin for 12V LED strip relay control (Channel 1)
const int pumpRelayPin = 6;            // Digital pin for 5V DC pump relay control (Channel 2)

const int moistureThreshold = 50; // Adjust this threshold as needed

// DHT sensor setup
#define DHTPIN 4        // Define the pin to which the DHT sensor is connected
#define DHTTYPE DHT11   // Define the type of DHT sensor (DHT11)

DHT dht(DHTPIN, DHTTYPE); // Create a DHT object

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password

// Blynk credentials
char auth[] = SECRET_BLYNK_AUTH; // Replace with your Blynk auth token

BlynkTimer timer;

bool isLedOn = false;
bool isPumpOn = false;

void setup() {
  pinMode(ldrDigitalPin, INPUT);      // Set LDR pin as input
  pinMode(ledStripRelayPin, OUTPUT); // Set LED strip relay control pin as output (Channel 1)
  pinMode(pumpRelayPin, OUTPUT);     // Set pump relay control pin as output (Channel 2)

  digitalWrite(ledStripRelayPin, LOW); // Turn off the LED strip relay initially
  digitalWrite(pumpRelayPin, LOW);     // Turn off the pump relay initially

  Wire.begin();
  Serial.begin(115200);  // Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }

  Blynk.begin(auth, ssid, pass); // Initialize Blynk
  dht.begin(); // Initialize DHT sensor
  
  timer.setInterval(1000L, sendDataToBlynk); // Send sensor data to Blynk every 1 second
}

void sendDataToBlynk() {
  int moistureValue = analogRead(moistureSensorPin);
  int ldrValue = digitalRead(ldrDigitalPin); // Read digital LDR pin

  // Read temperature and humidity from DHT11 sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Convert analog readings to meaningful values
  int moisturePercentage = map(moistureValue, 0, 4095, 0, 100); // Map moistureValue to a percentage

  // Determine LDR status based on digital value
  String ldrStatus = (ldrValue == HIGH) ? "Bright" : "Dark";

  // Control the LED strip based on LDR status using the dual-channel relay
  if (ldrStatus == "Dark") {
    digitalWrite(ledStripRelayPin, HIGH); // Turn on the LED strip relay (Channel 1)
  } else {
    digitalWrite(ledStripRelayPin, LOW); // Turn off the LED strip relay (Channel 1)
  }

  // Control the pump based on moisture level using the dual-channel relay
  if (moisturePercentage > moistureThreshold) {
    digitalWrite(pumpRelayPin, HIGH); // Turn on the pump relay (Channel 2)
  } else {
    digitalWrite(pumpRelayPin, LOW); // Turn off the pump relay (Channel 2)
  }

  // Print the sensor data to the Serial Monitor
  Serial.println("Sensor Data:");
  Serial.print("Moisture Percentage: ");
  Serial.print(moisturePercentage);
  Serial.println("%");
  Serial.print("LDR Status: ");
  Serial.println(ldrStatus);
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  // Send sensor data to Blynk virtual pins
  Blynk.virtualWrite(V0, moisturePercentage);
  Blynk.virtualWrite(V1, ldrStatus);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, humidity);
}

BLYNK_WRITE(V4) {
  int pumpControlValue = param.asInt();
  
  // Control the pump based on Blynk button state
  if (pumpControlValue == HIGH) {
    digitalWrite(pumpRelayPin, HIGH); // Turn on the pump relay (Channel 2)
    isPumpOn = true;
  } else {
    digitalWrite(pumpRelayPin, LOW); // Turn off the pump relay (Channel 2)
    isPumpOn = false;
  }
}

BLYNK_WRITE(V5) {
  int ledControlValue = param.asInt();
  
  // Control the LED based on Blynk button state
  if (ledControlValue == HIGH) {
    digitalWrite(ledStripRelayPin, HIGH); // Turn on the LED strip relay (Channel 1)
    isLedOn = true;
  } else {
    digitalWrite(ledStripRelayPin, LOW); // Turn off the LED strip relay (Channel 1)
    isLedOn = false;
  }
}

void loop() {
  Blynk.run(); // Run Blynk
  timer.run(); // Run the timer
}
