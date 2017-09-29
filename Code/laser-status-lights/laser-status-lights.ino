// Eventually merge this with esp8266SerialEcho

#include <Adafruit_NeoPixel.h>
#include <component.h>
#include "mqtt-wrapper.h"

#define PIXELPIN              5  //D1
#define SHOPAIRVALVEPIN       4  //D2
#define EXHAUSTGATEPIN        0  //D3 
#define EXHAUSTFANPIN         -1 //sensed via mqtt, no real pin
#define SHOPAIRCOMPRESSORPIN  14 //D5 - skip D4 since it's TX1
#define READYPIN              -1 //virtual item, no pin


#define NUMPIXELS   12
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

// define some colors for the neopixels
uint32_t red = pixels.Color(200,0,0);
uint32_t blue = pixels.Color(0,0,200);
uint32_t green = pixels.Color(0,255,0);
uint32_t off = pixels.Color(0,0,0);

int delayVal = 50;
int j = 0;

// Define the components that are being monitored
// component {PIN, OffLED, OnLED, State}
// State 0 = on, 1 = off
component ExhaustGate = {EXHAUSTGATEPIN, 0, 1, 1};
component ExhaustFan = {EXHAUSTFANPIN, 2, 3, 1};
component ShopAirValve = {SHOPAIRVALVEPIN, 4, 5, 1};
component ShopAirCompressor = {SHOPAIRCOMPRESSORPIN, 6, 7, 1};
component componentList[] = {ShopAirValve, ShopAirCompressor, ExhaustGate, ExhaustFan};
int componentListLen = 4;
int readyChecksum = 0;
// Ready light when all components are ready
component Ready = {READYPIN, 8, 9, 1};

char buf[1024];
const int bufferSize = 500;
char timerInputBuffer[bufferSize];
char outputBuffer[bufferSize];
int i = 0;
int k = 0;
bool isTime = 0;
bool isWriteCount = 0;
bool isOnline = 0;
String tubeTime;
String writeCount;
const char* host_name = "bumblebee_timer";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.println("Message arrived [");
  Serial.println(topic);
  Serial.println("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  topic += 34;
  topic[3] = '\0';
  uint32_t c;

  if((char)payload[1] == 'N') {
    ExhaustFan.State = 0;
  } else if ((char)payload[1] == 'F') {
    ExhaustFan.State = 1;
  } else {
    Serial.println("NOT A THING FUCK");
  }
}
void connectSuccess(PubSubClient* client, char* ip) {
  Serial.println("win");
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/laserZone/bumblebee_timer/INFO2", buf);
  client->subscribe("stat/i3/laserZone/ventFan/POWER");
  client->publish("cmnd/i3/laserZone/ventFan/POWER", "");
}
void setup() {
  //start serial connection
  Serial.begin(115200);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name);
  isOnline = 1;

  pinMode(ShopAirValve.Pin, INPUT_PULLUP);
  pinMode(ExhaustGate.Pin, INPUT_PULLUP);
  pinMode(ExhaustFan.Pin, INPUT_PULLUP);
  pinMode(ShopAirCompressor.Pin, INPUT_PULLUP);
  
  pixels.begin(); // This initializes the NeoPixel library.
  //Do a color wipe
  for (int j=0; j < NUMPIXELS; j++) {
    pixels.setPixelColor(i,blue);
    pixels.show();
    delay(100);
  }
  for (int j=0; j < NUMPIXELS; j++) {
    pixels.setPixelColor(i,off);
    pixels.show();
    delay(100);
  }
}

void connectedLoop(PubSubClient* client) {
  if (isTime == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/laserTubeTime", outputBuffer);
  }
  if (isWriteCount == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/EEPROMwriteCount", outputBuffer);
  }
    if (isOnline == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/status", "Laser_Timer_Online");
  }
  isTime = 0;
  isWriteCount = 0;
  isOnline = 0;
}

void loop() {
  readyChecksum = 0;  //reset the checksum
  Ready.State = 1; //Assume not ready, check for readiness below
  for (int j = 0; j < componentListLen; j++) {
    checkState(componentList[j]);
    setLED(componentList[j]);
    if (componentList[j].State == 0) {
      readyChecksum++;
    }
  }
  // check if all devices have reported state 0
  if (readyChecksum == componentListLen) {
    Ready.State = 0;
  }
  setLED(Ready);
  pixels.show();
  delay(delayVal);
  readFromTimer();
  loop_mqtt();
}

void checkState(component device) {
  if (device.Pin >=0) {
    device.State = digitalRead(device.Pin);
  }
}
void setLED(component device) {
  if (device.State == 0) {
    ledOn(device);
  }
  else {
    ledOff(device);
  }
}

void ledOn(component device) {
  pixels.setPixelColor(device.OnLED, blue);
  pixels.setPixelColor(device.OffLED, off);
}

void ledOff(component device) {
  pixels.setPixelColor(device.OnLED, off);
  pixels.setPixelColor(device.OffLED, red);
}

bool isNumeric(String str, int index) {
  for (byte i = index; i < str.length(); i++) {
    if(!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void readFromTimer(){
  // Watch serial for various values sent from timer
  // Serial read code borrowed from http://robotic-controls.com/learn/arduino/arduino-arduino-serial-communication
  k = 0;
  String timerInput = "\0";
  if (Serial.available()){
    delay(100);
    while(Serial.available() && k < bufferSize) {
      timerInputBuffer[k++] = Serial.read();
    }
    timerInputBuffer[k++] = '\0';
    timerInput = String(timerInputBuffer);
    Serial.println(timerInput);
    timerInput.trim(); // get rid of tailing carriage return that mucks up isNumeric()
    // Logical checks for trigger characters
    if (timerInput.charAt(0) == '&') {
      if (isNumeric(timerInput,1) && timerInput.length() > 1) {
        isTime = 1;
        tubeTime = timerInput.substring(1);
        tubeTime.toCharArray(outputBuffer,bufferSize);
        Serial.print("Time: ");
        Serial.println(tubeTime);
      }
      else Serial.print("(&) Input not recognized\n");
    }
    else if (timerInput.charAt(0) == '^') {
      // substitute a different logical test below for other data
      if (isNumeric(timerInput,1) && timerInput.length() > 1) {
        isWriteCount = 1;
        writeCount = timerInput.substring(1);
        writeCount.toCharArray(outputBuffer,bufferSize);
        Serial.print("Other: ");
        Serial.println(writeCount);
      }
      else Serial.print("(^) Input not recognized\n");
    }
    else if (timerInput.charAt(0) == '#') {
      isOnline = 1;
      Serial.print("Laser timer online\n");
    }
    else Serial.print("(other) Input not recognized\n");
  }
}