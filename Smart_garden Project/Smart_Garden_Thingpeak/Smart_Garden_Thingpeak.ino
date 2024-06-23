#include <Wire.h>
#include <WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h"
#include <Adafruit_Sensor.h>
#include <DHT.h> // Include the DHT library

// Pin definitions
const int moistureSensorPin = 10;     // Connect non-corrosive soil moisture sensor to GPIO33_ADC
const int ldrDigitalPin = 5;           // Digital pin for LDR module
const int ledStripRelayPin = 7;        // Digital pin for 12V LED strip relay control (Channel 1)
const int pumpRelayPin = 6;            // Digital pin for 5V DC pump relay control (Channel 2)
const int buckConverterRelayPin = 11;  // Digital pin for 12V buck converter relay control

const int moistureThreshold = 50; // Adjust this threshold as needed

// DHT sensor setup
#define DHTPIN 4        // Define the pin to which the DHT sensor is connected
#define DHTTYPE DHT11   // Define the type of DHT sensor (DHT11)

DHT dht(DHTPIN, DHTTYPE); // Create a DHT object

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

void setup() {
  pinMode(ldrDigitalPin, INPUT);      // Set LDR pin as input
  pinMode(ledStripRelayPin, OUTPUT); // Set LED strip relay control pin as output (Channel 1)
  pinMode(pumpRelayPin, OUTPUT);     // Set pump relay control pin as output (Channel 2)
  pinMode(buckConverterRelayPin, OUTPUT); // Set 12V buck converter relay control pin as output

  digitalWrite(ledStripRelayPin, LOW); // Turn off the LED strip relay initially
  digitalWrite(pumpRelayPin, LOW);     // Turn off the pump relay initially
  digitalWrite(buckConverterRelayPin, LOW); // Turn off the buck converter relay initially

  Wire.begin();
  Serial.begin(115200);  // Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  dht.begin(); // Initialize DHT sensor
}

void loop() {

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

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

  // Control the 12V buck converter based on a condition (e.g., temperature)
  // For example, let's turn on the buck converter if the temperature is above 25°C
  if (temperature > 25.0) {
    digitalWrite(buckConverterRelayPin, HIGH); // Turn on the 12V buck converter relay
  } else {
    digitalWrite(buckConverterRelayPin, LOW); // Turn off the 12V buck converter relay
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

  // Update ThingSpeak channel with sensor data
  ThingSpeak.setField(1, temperature); // Field 1 for moisture percentage
  ThingSpeak.setField(2, humidity); // Field 2 for LDR value
  ThingSpeak.setField(3, moisturePercentage); // Field 3 for temperature
  ThingSpeak.setField(4, ldrStatus); // Field 4 for humidity

  // Write the data to ThingSpeak
  int writeResult = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  // Check if the write was successful
  if (writeResult == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(writeResult));
  }

  delay(1000); // Wait for 15 seconds (ThingSpeak allows updates every 15 seconds)
}
