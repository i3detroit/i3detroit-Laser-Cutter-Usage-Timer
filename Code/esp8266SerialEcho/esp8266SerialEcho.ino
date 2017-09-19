/*
 * Based on https://github.com/i3detroit/mqtt-wrapper/commit/753bf3e4c900d1b0f58bdae45f233a4cac133e14#diff-722036d88083221fb60ee22839fe48f1
 *
 */
#include "mqtt-wrapper.h"

const char* host_name = "bumblebee_timer";
//const char* ssid = "i3detroit-wpa";
//const char* password = "i3detroit";
const char* ssid = "Pleiades";
const char* password = "Volleyball19APotassium514Larsen974";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

char buf[1024];
char laserTime[20];

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  // Watch serial for values sent from timer
  // Code borrowed from http://robotic-controls.com/learn/arduino/arduino-arduino-serial-communication
  int i = 0;
  if (Serial.available()){
	delay(100);
    while(Serial.available() && i < 20) {
      laserTime[i++] = Serial.read();
    }
    laserTime[i++] = '\0';
  }
  // If a message was received, print and publish
  if(i>0) {
    Serial.println(laserTime);
    client->publish("stat/i3/laserZone/bumblebee_timer/status", laserTime);
  }
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
void connectedLoop(PubSubClient* client) {
}
void loop() {
  loop_mqtt();
}