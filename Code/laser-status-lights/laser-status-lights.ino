// Eventually merge this with esp8266SerialEcho

#include <Adafruit_NeoPixel.h>
#include <component.h>
#include "mqtt-wrapper.h"

#define SHOPAIRVALVEPIN       0  //D3
#define EXHAUSTGATEPIN        2  //D4 
#define EXHAUSTFANPIN         -1 //sensed via mqtt, no real pin
#define SHOPAIRCOMPRESSORPIN  14 //D5
#define PIXELPIN              4 //D2

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

//List of components with physical switches
component componentList[] = {ShopAirValve, ShopAirCompressor, ExhaustGate};
int componentListLen = 3;

// Ready light when all components are ready
component Ready = {0, 8, 9, 0}; //only need LED numbers, so pin and state are set to zero

char buf[1024];
int i = 0;
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
  client->publish("tele/i3/laserZone/bumblebee_status_lights/INFO2", buf);
  client->subscribe("stat/i3/laserZone/ventFan/POWER");
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

}
void loop() {
  Ready.State = 0;
  ledOn(Ready);
  for (int j = 0; j < componentListLen; j++) {
    componentList[j].State = digitalRead(componentList[j].Pin);
    if(componentList[j].State == 0){
      ledOn(componentList[j]);
    }
    else{
      ledOff(componentList[j]);
      Ready.State = 1;
      ledOff(Ready);
    }
  }
  if (ExhaustFan.State == 0) {
    ledOn(ExhaustFan);
  }
  else {
    ledOff(ExhaustFan);
    Ready.State = 1;
    ledOff(Ready);
  }
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