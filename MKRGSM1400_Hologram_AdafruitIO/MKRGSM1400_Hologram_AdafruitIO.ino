// Maker Faire 2018 - Building Cellular IoT Devices
// Don Coleman
//
// Based on ArduinoMKRGSM1400 by Sandeep Mistry
// Uses Joël Gähwiler MQTT Library https://github.com/256dpi/arduino-mqtt
//
// Temperature data is published to Adafruit.io via MQTT
// The LED is controlled by subscribing to MQTT

// Change the names for the topics to make your username
// Create 2 feeds on Adafruit IO led & temperature
// change the code so the feed names include your username

#include <MKRGSM.h>
#include <MQTT.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"

const char pin[]      = "";
const char apn[]      = "hologram";
const char login[]    = "";
const char password[] = "";

#include "arduino_secrets.h" 
const char IO_USERNAME[] = SECRET_IO_USERNAME;
const char IO_KEY[] = SECRET_IO_KEY;

GSM gsm;
GPRS gprs;
GSMSSLClient net;  // secure
MQTTClient mqtt;

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

// 5 minutes in milliseconds
unsigned long publishInterval = 5 * 60 * 1000;
// large number to force publish on start
unsigned long lastMillis = 0-1;

void connectGSM() {
  
  bool connected = false;  
  Serial.print("connecting to cellular network ...");

  // After starting the modem with gsm.begin()
  // attach to the GPRS network with the APN, login and password
  while (!connected) {
    if ((gsm.begin(pin) == GSM_READY) &&
        (gprs.attachGPRS(apn, login, password) == GPRS_READY)) {
      connected = true;
    } else {
      Serial.print(".");
      delay(1000);
    }
  }

}

void connectMQTT() {

  // increase keepAlive to minimze data usage
  // https://github.com/256dpi/arduino-mqtt/blob/master/src/MQTTClient.h#L162
  mqtt.setOptions(300, true, 1000);
  Serial.print("\nconnecting...");
  while (!mqtt.connect("mkr1400", IO_USERNAME, IO_KEY)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  // subscribe to feed that controls the LED
  // TODO change the feed to match your username
  mqtt.subscribe("doncoleman/f/led");

}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (payload == "ON") {
    // turn the LED on
    digitalWrite(LED_BUILTIN, HIGH);
  } else if (payload == "OFF") {
    // turn the LED off
    digitalWrite(LED_BUILTIN, LOW);    
  }
}

void setup() {
  Serial.begin(9600);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // Make sure the sensor is found, you can also pass in a different i2c
  // address with tempsensor.begin(0x19) for example
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
    while (1);
  }

  connectGSM();
  mqtt.begin("io.adafruit.com", 8883, net);
  mqtt.onMessage(messageReceived);
}

void loop() {
  mqtt.loop();

  if (!mqtt.connected()) {
    connectMQTT();
  }

  if (millis() - lastMillis > publishInterval) {
    lastMillis = millis();
    float temp = getTemperature();
    // TODO change the feed to match your username
    mqtt.publish("doncoleman/f/temperature", String(temp));
  }
}

float getTemperature() {
  tempsensor.wake();   // wake up, ready to read!

  // Get the temperaure and convert to *F
  float c = tempsensor.readTempC();
  float f = c * 9.0 / 5.0 + 32;
  Serial.print("Temp: "); Serial.print(c); Serial.print("*C\t"); 
  Serial.print(f); Serial.println("*F");
  
  tempsensor.shutdown(); // shutdown MSP9808 - power consumption ~0.1 micro Ampere
  return f;
}
