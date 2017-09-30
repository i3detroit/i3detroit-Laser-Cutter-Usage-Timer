// Eventually merge this with esp8266SerialEcho

#include <Adafruit_NeoPixel.h>
#include <component.h>
#include "mqtt-wrapper.h"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define PIXELPIN              5  //D1
#define SHOPAIRVALVEPIN       4  //D2 //skip d3
#define EXHAUSTGATEPIN        12  //D6
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

struct component ExhaustGate = {EXHAUSTGATEPIN, 0, 1, 1,""};
struct component ExhaustFan = {EXHAUSTFANPIN, 2, 3, 1,"stat/i3/laserZone/ventFan/POWER"};
struct component ShopAirValve = {SHOPAIRVALVEPIN, 4, 5, 1,""};
struct component ShopAirCompressor = {SHOPAIRCOMPRESSORPIN, 6, 7, 1,""};

//List of components with physical switches
struct component physicalComponents[] = {ShopAirValve, ShopAirCompressor, ExhaustGate};
struct component *mqttComponents[] = {&ExhaustFan};
struct component Ready = {READYPIN, 8, 9, 0};
int numPhysicalComponents = ARRAY_SIZE(physicalComponents);
int numMqttComponents = ARRAY_SIZE(mqttComponents);
int numComponents = numPhysicalComponents + numMqttComponents;
int readyChecksum = 0;

char buf[1024];
int i = 0;
const int bufferSize = 500;
char timerInputBuffer[bufferSize];
char outputBuffer[bufferSize];
int k = 0;
bool isTime = 0;
bool isWriteCount = 0;
bool isOnline = 0;
String tubeTime;
String writeCount;
const char* host_name = "bumblebee_status_lights";
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
  uint32_t c;
  for (int j = 0; j < numMqttComponents; j++) {
  	Serial.println(topic);
  	Serial.println(mqttComponents[j]->mqttTopic);
  	if (strcmp(topic, mqttComponents[j]->mqttTopic) == 0){
  		Serial.print("yay");
		  if((char)payload[1] == 'N') {
		    mqttComponents[j]->State = 0;
		  } else if ((char)payload[1] == 'F') {
		    mqttComponents[j]->State = 1;
		  } else {
		    Serial.println("NOT A THING FUCK");
		  }
		}
	}
}
void connectSuccess(PubSubClient* client, char* ip) {
  Serial.println("win");
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/laserZone/bumblebee_status_lights/INFO2", buf);
  for (int j = 0; j < numMqttComponents; j++) {
		client->subscribe(mqttComponents[j]->mqttTopic);
  }
  client->publish("cmnd/i3/laserZone/ventFan/POWER", "");
}
void setup() {
  //start serial connection
  Serial.begin(115200);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name);

  pinMode(ShopAirValve.Pin, INPUT_PULLUP);
  pinMode(ExhaustGate.Pin, INPUT_PULLUP);
  pinMode(ExhaustFan.Pin, INPUT_PULLUP);
  pinMode(ShopAirCompressor.Pin, INPUT_PULLUP);
  
  pixels.begin(); // This initializes the NeoPixel library.
  for (int i=0; j < NUMPIXELS; j++) {
    //pixels.setPixelColor(i,red);
  }
  pixels.show();
  for (int i=0; j < NUMPIXELS; j++) {
    // pixels.setPixelColor(i,blue);
  }
  pixels.show();
  for (int i=0; j < NUMPIXELS; j++) {
    // pixels.setPixelColor(i,off);
  }
  pixels.show();
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
	readyChecksum = 0;
  Ready.State = 1;
  ledOff(Ready);
  for (int j = 0; j < numPhysicalComponents; j++) {
		physicalComponents[j].State = digitalRead(physicalComponents[j].Pin);
    if(physicalComponents[j].State == 0){
      ledOn(physicalComponents[j]);
      readyChecksum++;
    }
    else {
      ledOff(physicalComponents[j]);
    }
  }
  for (int j = 0; j < numMqttComponents; j++) {
	  //Serial.println(mqttComponents[j].State);
    //delay(200);
    if(mqttComponents[j]->State == 0){
      ledOn(*mqttComponents[j]);
      readyChecksum++;
    }
    else {
      ledOff(*mqttComponents[j]);
    }
  }
  if (readyChecksum == numComponents) {
  	ledOn(Ready);
  }
  readFromTimer();
  loop_mqtt();
  pixels.show();
  delay(delayVal);
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

void readFromTimer() {
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
    /*for (byte k = 0; k<timerInput.length(); k++) {
      Serial.print("\nCharacter\n");
      Serial.println(timerInput.charAt(k)); 
      Serial.print("\nValue\n");
      Serial.print(timerInput.charAt(k));
    }*/
    Serial.println(timerInput);
    //resetting these here doesn't seem to work right - does loop_mqtt() call connectedLoop() more than once?
    //isTime = 0;
    //isWriteCount = 0;
    //isOnline = 0;
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