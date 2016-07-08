
/*
 Spoka
 Copyright (c) 2016 Andy Bateman.  All right reserved.
 Created by Andy Bateman, June 2016.
 
 Serious bit of hacking of an Ikea SPÖKA to create a Glo Clock clone for my little boy.
 (http://www.ikea.com/us/en/catalog/products/30150984/)

*/

#include <ArduinoOTA.h>        // Also calls ESP8266WiFi.h and WiFiUdp.h
#include <WiFiManager.h>       // Also calls ESP8266WiFi.h, ESP8266WebServer.h, DNSServer.h and WiFiUdp.h

#include <ESP8266mDNS.h>       // Also calls ESP8266WiFi.h and WiFiUdp.h
#include <Adafruit_NeoPixel.h> // Also calls Arduino.h

const char* OTAName = "esp12e-spoka-dev";        // Name of device as displayed in Arduino
const char* OTAPassword = "cf506a3aa";           // Password for Arduino to proceed with upload

#define buttonPin 13            // Define pin for mode switch (pulled up)
#define pixelPin 4              // Define pin that the data line for first NeoPixel
#define pixelCount 3            // How many NeoPixels are you using?

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(pixelCount, pixelPin, NEO_GRB + NEO_KHZ800);

int buttonState;                 // the current reading from the input pin
int lastButtonState = LOW;       // the previous reading from the input pin
long lastDebounceTime = 0;       // the last time the output pin was toggled
long debounceDelay = 50;         // the debounce time; increase if the output flickers

byte colourPos = 150;            // Default Colour (NB: 30 is Yellow)

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFiManager wifiManager;
  wifiManager.autoConnect();

  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  pinMode(buttonPin, INPUT_PULLUP);
  pixels.begin(); // This initializes the NeoPixel library.

  for(uint16_t i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, Wheel(colourPos & 255));
  }
  pixels.show();
}

void loop() {
  ArduinoOTA.handle();

  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      if (buttonState = reading == HIGH) {
        rainbow(255);
      }
    }
  }
  lastButtonState = reading;
}

void rainbow(uint8_t colourMove) {
  for(uint16_t j=0; j<=colourMove; j++) {

    colourPos++;
    if (colourPos>=255) { colourPos = 0; }
    
    for(uint16_t i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(colourPos & 255));
    }
    pixels.show();
    delay(15);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
