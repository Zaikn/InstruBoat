extern "C" {
#include "user_interface.h"  // Required for wifi_station_connect() to work
}
#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/* Define declination of location from where measurement going to be done.
  e.g. here we have added declination from location Pune city, India.
  we can get it from http://www.magnetic-declination.com */

#define Declination       -0.00669
#define hmc5883l_address  0x1E
#define FPM_SLEEP_MAX_TIME 0xFFFFFFF

ESP8266WebServer server(80);

unsigned long previousMillis = 0;
unsigned long previousMillisLED = 0;
const long intervalboot = 100;
const long intervalstby = 500;
int ledState = LOW;

void setup() {
  WiFiOff();
  delay(10);
  pinMode(15, OUTPUT);
  while (millis() <= 60000) {
    if (millis() - previousMillis >= intervalboot) {
      previousMillis = millis();
      if (ledState == LOW)
        ledState = HIGH;  // Note that this switches the LED *off*
      else
        ledState = LOW;   // Note that this switches the LED *on*
      digitalWrite(15, ledState);
      delay(1);
    }
  }
  Wire.begin(D5, D6); /* join i2c bus with SDA=D6 and SCL=D5 of NodeMCU */
  hmc5883l_init();
  WiFiOn();
  previousMillis = millis();
  previousMillisLED = millis();
}

void loop() {



  /*-----------------------------SERIAL--------------------------------*/

  server.handleClient(); //Handling of incoming requests
  delay(0);

  /*-----------------------------FANCY LED--------------------------------*/

  if (((unsigned long)(millis() - previousMillisLED) >= intervalstby) && (WiFi.softAPgetStationNum() == 0)) {
    previousMillisLED = millis();
    if (ledState == LOW)
      ledState = HIGH;  // Note that this switches the LED *off*
    else
      ledState = LOW;   // Note that this switches the LED *on*
    digitalWrite(15, ledState);
  }
  if (WiFi.softAPgetStationNum() != 0) {
    ledState = LOW;
    digitalWrite(15, ledState);
    previousMillis = millis();
  }

  /*--------------------------ECO ENERGIE-------------------------------*/

  if (((unsigned long)(millis() - previousMillis) >= 60000) && (WiFi.softAPgetStationNum() == 0)) {
    previousMillis = millis();
    WiFiOff();
    while ((unsigned long)(millis() - previousMillis) <= 300000) {
      for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
        // sets the value (range from 0 to 255):
        analogWrite(15, fadeValue);
        // wait for 30 milliseconds to see the dimming effect
        delay(30);
      }

      // fade out from max to min in increments of 5 points:
      for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
        // sets the value (range from 0 to 255):
        analogWrite(15, fadeValue);
        // wait for 30 milliseconds to see the dimming effect
        delay(30);
      }
      delay(5000);

    }
    WiFiOn();
    previousMillis = millis();
  }


}

void hmc5883l_init() {  /* Magneto initialize function */
  Wire.beginTransmission(hmc5883l_address);
  Wire.write(0x00);
  Wire.write(0x70); //8 samples per measurement, 15Hz data output rate, Normal measurement
  Wire.write(0xA0); //
  Wire.write(0x00); //Continuous measurement mode
  Wire.endTransmission();
  delay(500);
}

void handleBody() { //Handler for the body path
  String message = "{\"angle\":";
  message += hmc5883l_GetHeading();
  message += ",\"pression\":";
  message += analogRead(A0);
  message += "}";
  server.send(200, "application/json", message);
}

int hmc5883l_GetHeading() {
  int16_t x, y, z;
  double Heading;
  Wire.beginTransmission(hmc5883l_address);
  Wire.write(0x03);
  Wire.endTransmission();
  /* Read 16 bit x,y,z value (2's complement form) */
  Wire.requestFrom(hmc5883l_address, 6);
  x = (((int16_t)Wire.read() << 8) | (int16_t)Wire.read());
  z = (((int16_t)Wire.read() << 8) | (int16_t)Wire.read());
  y = (((int16_t)Wire.read() << 8) | (int16_t)Wire.read());

  Heading = atan2((double)y, (double)x) + Declination;
  if (Heading > 2 * PI) /* Due to declination check for >360 degree */
    Heading = Heading - 2 * PI;
  if (Heading < 0)  /* Check for sign */
    Heading = Heading + 2 * PI;
  return (Heading * 180 / PI); /* Convert into angle and return */
}

/* Uncomment below function for reading status register */
//uint8_t readStatus(){
//  Wire.beginTransmission(hmc5883l_address);
//  Wire.write(0x09);
//  Wire.endTransmission();
//  Wire.requestFrom(hmc5883l_address, 1);
//  return (uint8_t) Wire.read();
//}

void WiFiOn() {
  WiFi.forceSleepWake();
  WiFi.softAP("ESP_Wind");
  server.on("/", handleBody); //Associate the handler function to the path
  server.begin(); //Start the server
}


void WiFiOff() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();


}
