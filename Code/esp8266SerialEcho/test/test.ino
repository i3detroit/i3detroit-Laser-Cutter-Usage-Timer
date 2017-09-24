// Serial communication test code to run on a plain arduino, without all the extra ESP8266 and MQTT stuff from the actual program

const int bufferSize = 500;
char timerInputBuffer[bufferSize];
char outputBuffer[bufferSize];
int j = 0;

bool isTime = 0;
bool isWriteCount = 0;
bool isOnline = 0;
String tubeTime;
String writeCount;

bool isNumeric(String str, int index) {
  for (byte i = index; i < str.length(); i++) {
    if(!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.print("Test Unit Online\n");
}

void loop() {
  j = 0;
  String timerInput;
  
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
    
    isTime = 0;
    isWriteCount = 0;
    isOnline = 0;
    timerInput.trim();
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