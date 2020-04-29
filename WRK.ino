/*
Arduino-MAX30100 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <U8g2lib.h>


#define REPORTING_PERIOD_MS     1000
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation

const char* ssid = "TP-Link_085A";
const char* password = "71135151";

//creating AsyncWebServer object-port 80
AsyncWebServer server(80);
//create new sensor instance
PulseOximeter pox;

uint32_t tsLastReport = 0;
uint32_t last_beat=0;
int beat = 0;
int scanCount = 10;
float totalbpm = 0;
float totBPM = 0;
float currentSpO2 = 0;
float currentBPM = 0;
String bpmS = String(25.2);
String spo2S = String(25.2);
// Callback (registered below) fired when a pulse is detected


void onBeatDetected()
{
    Serial.println("Beat!");
}



void setup()
{
    //initialization 
    Serial.begin(115200);
    u8g2.begin();
    u8g2.setFont(u8g2_font_cursor_tf);
    u8g2.setCursor(8,15);
    u8g2.print("^");
    u8g2.sendBuffer();
    
    // Initialize SPIFFS
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }

    // Print ESP32 Local IP Address
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html");
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", bpmS.c_str());
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", spo2S.c_str());
    });
      //start server
    server.begin();

    // The default current for the IR LED is 50mA and it could be changed
    //   by uncommenting the following line. Check MAX30100_Registers.h for all the
    //   available options.
    // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);

}
void loop()
{
  delay(100);

  //calling sensor detection
  pox.update();
  Serial.println(pox.getHeartRate());
  totBPM += pox.getHeartRate();
  Serial.print(">>>");
  Serial.print(totBPM);
  
  //onboard graphics
  u8g2.setFont(u8g2_font_victoriamedium8_8u);
  u8g2.setCursor(24,12);
  u8g2.print("BPM:  ");
  u8g2.print(currentBPM);
  u8g2.setCursor(24,24);
  u8g2.print("SPO2:  ");
  u8g2.print(currentSpO2);
  

  totalbpm ++;
  currentBPM = pox.getHeartRate();
  currentSpO2 = pox.getSpO2();
  bpmS = String(currentBPM);
  spo2S = String(currentSpO2);
  
  u8g2.setCursor(0,32);
  u8g2.setFont(u8g2_font_profont10_mr);
  u8g2.print("OK        ");
  u8g2.sendBuffer();

}
