
/*
 Spoka
 Copyright (c) 2016 Andy Bateman.  All right reserved.
 Created by Andy Bateman, June 2016.
 
 Serious bit of hacking of an Ikea SPÖKA to create a Glo Clock clone for my little boy.
 (http://www.ikea.com/us/en/catalog/products/30150984/)

*/

#include <ArduinoOTA.h>        // Also calls ESP8266WiFi.h and WiFiUdp.h
#include <WiFiManager.h>       // Also calls ESP8266WiFi.h, ESP8266WebServer.h, DNSServer.h and WiFiUdp.h

#include <ESP8266mDNS.h>          // Also calls ESP8266WiFi.h and WiFiUdp.h
#include <Adafruit_NeoPixel.h>    // Also calls Arduino.h

const char* OTAName = "esp12e-spoka";        // Name of device as displayed in Arduino
const char* OTAPassword = "cf506a3aa";       // Password for Arduino to proceed with upload

#define buttonPin 13              // Define pin for mode switch (pulled up)
#define pixelPin 4                // Define pin that the data line for first NeoPixel
#define pixelCount 3              // How many NeoPixels are you using?

byte colourPos = 30;             // Default Colour (NB: 30 is Yellow, 150 is a nice Blue)

int timeZone = +12;               // Time zone offset (12 for New Zealand, need to do something about DST)
const long interval = 60000;      // How often to check the time (60000 = every minute)

int buttonState;                  // the current reading from the input pin
int lastButtonState = LOW;        // the previous reading from the input pin
long lastDebounceTime = 0;        // the last time the output pin was toggled
long debounceDelay = 50;          // the debounce time; increase if the output flickers
unsigned long previousMillis = 0; // will store last time time was checked

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(pixelCount, pixelPin, NEO_GRB + NEO_KHZ800);

IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

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

  udp.begin(2390);

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
        rainbow(254);
      }
    }
  }
  lastButtonState = reading;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    WiFi.hostByName(ntpServerName, timeServerIP); 
        
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    delay(1000);
    
    int cb = udp.parsePacket();
    if (cb) {
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;
      
      int hours = ((epoch % 86400L) / 3600);   // get the hour (86400 equals secs per day)
      hours += timeZone;                       // Calculate the time zone offset
      if (hours < 0) { hours = 24 + hours; }
      if (hours > 23) { hours = hours - 23; }
      
      Serial.println(hours);
      Serial.println(colourPos);
      
      if (hours >= 6 && hours < 18) {
        if (colourPos == 150) { rainbow(134); }
      }
      if (hours < 6 || hours >= 18) {
        if (colourPos == 30) { rainbow(119); }
      }
    } 
  }
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
int32_t Wheel(byte WheelPos) {
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

unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
