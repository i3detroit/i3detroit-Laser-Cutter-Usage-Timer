#include <stdio.h>

//Pin Definitions
int analogPin = 0;      // input voltage connected to analog pin 0 from interface board

// Kept threshold values from DMS timer but swapped the logic in loop()
// because our laser uses active low instead of active high
int analogVal = 0;      // variable to store the analog value read
int anaLowThreshold = 300;   // if analog goes above this value its considered ON
int anaHighThreshold = 324; // if analog value goes below this value its considered OFF

unsigned long millisOnLast = 0;
unsigned long millisOffLast = 0;
unsigned long millisTemp = 0;
unsigned long millisDiff = 0;
boolean lastLaserOn = false;
unsigned long userMillis = 0;
unsigned long lastPublishTime = 0;

String timeChar = "&";
String onlineChar = "#";

void setup() {
  Serial.begin(115200);
  Serial.println(onlineChar);
}

void loop() {
  // do a tight loop on checking the laser and keeping track of on/off times  
  for (int i=0; i <= 100; i++) {
    // read the input pin
    analogVal = analogRead(analogPin);
    
    // laser has been off, turning on
    if ((analogVal <  anaLowThreshold) && !lastLaserOn) {
      lastLaserOn = true;
      millisOnLast = (unsigned long) millis();
      millisDiff = millisOnLast - millisOffLast;
    }
    // laser has been on, continuing on
    else if ((analogVal < anaLowThreshold) && lastLaserOn) {
      lastLaserOn = true;
      millisTemp = (unsigned long) millis();
      millisDiff = millisTemp-millisOnLast;
      millisOnLast = millisTemp;
    }
    // laser has been on, turning off
    else if ((analogVal > anaHighThreshold) && lastLaserOn) {
      lastLaserOn = false;
      millisOffLast = (unsigned long) millis();
    }
    // laser has been off, staying off
    else {
      lastLaserOn = false;
      millisOffLast = (unsigned long) millis();
    }
    userMillis = userMillis + millisDiff;
    millisDiff = 0;
  }
  // Check if it's time to publish
  millisTemp = (unsigned long) millis();
  if ((userMillis > 0) && (lastPublishTime - millisTemp > 30000 )) { 
    Serial.println(timeChar+userMillis);
    lastPublishTime = millisTemp;
    userMillis = 0;
   }  
}
