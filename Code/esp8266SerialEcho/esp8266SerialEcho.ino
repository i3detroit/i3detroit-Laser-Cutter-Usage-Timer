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
char timerInputBuffer[20];
char outputBuffer[20];
int i = 0;
int j = 0;
bool isTime = 0;
bool isOther = 0;
String tubeTime;
String other;

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
  Serial.println("win");
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/laserZone/bumblebee_timer/INFO2", buf);
}
void setup() {
  Serial.begin(115200);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name);
}
// Do the mqtt stuff:
void connectedLoop(PubSubClient* client) {
  if (isTime == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/laserTubeTime", outputBuffer);
  }
  if (isOther == 1){
    client->publish("stat/i3/laserZone/bumblebee_timer/status", outputBuffer);
  }

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
  // Watch serial for values sent from timer
  // Code borrowed and modified from http://robotic-controls.com/learn/arduino/arduino-arduino-serial-communication
  
  j = 0;
  String timerInput;
  
  if (Serial.available()){
    delay(100);
    while(Serial.available() && j < 50) {
      timerInputBuffer[j++] = Serial.read();
    }
    timerInputBuffer[j++] = '\0';
    timerInput = String(timerInputBuffer);
    
    //Serial.println(timerInput);
    
    isTime = 0;
    isOther = 0;
    
    if (timerInput.charAt(0) == '&') {
      if (isNumeric(timerInput,1)) {
        isTime = 1;
        tubeTime = timerInput.substring(1);
        tubeTime.toCharArray(outputBuffer,20);
        Serial.print("Time: ");
        Serial.println(tubeTime);
      }
      else Serial.print("Input not recognized\n");
    }
    else if (timerInput.charAt(0) == '^') {
      // substitute a different logical test below for other data
      //if (isNumeric(timerInput,1)) {
        isOther = 1;
        other = timerInput.substring(1);
        other.toCharArray(outputBuffer,20);
        Serial.print("Other: ");
        Serial.println(other);
      //}
      //else Serial.print("Input not recognized\n");
    }
    else Serial.print("Input not recognized\n");
  }
  
  loop_mqtt();
}