/*
 * Based on https://github.com/i3detroit/mqtt-wrapper/commit/753bf3e4c900d1b0f58bdae45f233a4cac133e14#diff-722036d88083221fb60ee22839fe48f1
 *
 */
#include "mqtt-wrapper.h"

const char* host_name = "bumblebee_timer";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

char buf[1024];
const int bufferSize = 500;
char timerInputBuffer[bufferSize];
char outputBuffer[bufferSize];
int i = 0;
int j = 0;
bool isTime = 0;
bool isOnline = 0;
String tubeTime;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
void connectSuccess(PubSubClient* client, char* ip) {
  Serial.println("connect success");
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/laserZone/bumblebee_timer/INFO2", buf);
}
void setup() {
  Serial.begin(115200);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name);
  isOnline = 1;
}
// Do the mqtt stuff:
void connectedLoop(PubSubClient* client) {
  if (isTime == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/tube-time", outputBuffer);
  }
    if (isOnline == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/status", "Laser_Timer_Online");
  }
  isTime = 0;
  isOnline = 0;
}
bool isNumeric(String str, int index) {
  for (byte i = index; i < str.length(); i++) {
    if(!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void loop() {
  // Watch serial for various values sent from timer
  // Serial read code borrowed from http://robotic-controls.com/learn/arduino/arduino-arduino-serial-communication
  j = 0;
  String timerInput = "\0";
  
  if (Serial.available()){
    delay(100);
    while(Serial.available() && j < bufferSize) {
      timerInputBuffer[j++] = Serial.read();
    }
    timerInputBuffer[j++] = '\0';
    timerInput = String(timerInputBuffer);
    /*for (byte j = 0; j<timerInput.length(); j++) {
      Serial.print("\nCharacter\n");
      Serial.println(timerInput.charAt(j)); 
      Serial.print("\nValue\n");
      Serial.print(timerInput.charAt(j));
    }*/
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
    else if (timerInput.charAt(0) == '#') {
      isOnline = 1;
      Serial.print("Laser timer online\n");
    }
    else Serial.print("(other) Input not recognized\n");
  }
  // send mqtt stuff
  loop_mqtt();
}
