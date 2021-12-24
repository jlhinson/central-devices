// Libraries to import
#include <WiFi.h>
#include <MQTT.h>
#include <Adafruit_LC709203F.h>
#include <Adafruit_AHTX0.h>
#include <ArduinoJson.h>

// Deep sleep settings
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 30 // Time to sleep in seconds

// Initialize components
WiFiClient net;
MQTTClient client;
Adafruit_AHTX0 aht;
Adafruit_LC709203F lc;

// Define variables
float TempF = 0.0;
StaticJsonDocument<150> doc;
JsonObject sensor = doc.createNestedObject("sensor");
JsonObject battery = doc.createNestedObject("battery");
String data = "";
int count = 0;

// Enter WiFi and broker details
const char ssid[] = "ssid";
const char pass[] = "pass";
IPAddress broker(192, 168, 8, 102);
const String topic = "test";

void setup() {
  
	// Enable I2C STEMMA QT power
	pinMode(7, OUTPUT);
	digitalWrite(7, LOW);

	// Let things stabalize coming out of deep sleep
	delay(1000);
	
  // initialize USB serial converter so we have a port created
  Serial.begin(115200);

	// Configure wakeup source
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	
	// Setup battery monitor, sensor, WiFi, and MQTT
  lc.begin();
	lc.setPackSize(LC709203F_APA_2000MAH);
  aht.begin();
	WiFi.begin(ssid, pass);
	client.begin(broker, net);
	
	// Get sensor data and format temperature
	sensors_event_t humidity, temp;
	aht.getEvent(&humidity, &temp);
	TempF = (temp.temperature*1.8)+32;
	
	// Create JSON object with data
	sensor["temp"] = String(TempF, 1); 
	sensor["humidity"] = String(humidity.relative_humidity, 1);
	battery["voltage"] = String(lc.cellVoltage(), 2);
	battery["percent"] = String(lc.cellPercent(), 1);

	// Empty out data string and serialize JSON
	data = "";
	serializeJson(doc, data);

	// Connect to WiFi
	while (WiFi.status() != WL_CONNECTED && count < 30) {
    count++;
		delay(500);
	}
  count = 0;

	// Connect to MQTT broker
	while (!client.connect("clientID") && count < 10) {
    count++;
		delay(500);
	}
  count = 0;

	// Publish to MQTT broker
	client.publish(topic, data, false, 1);
	
	// Disconnect from MQTT broker and WiFi
	client.disconnect();
	WiFi.disconnect();
	
	// Enter deep sleep
	esp_deep_sleep_start();
}

void loop() {
	// Not used because of deep sleep
}