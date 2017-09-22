char timerInputBuffer[20];
char outputBuffer[20];
int j = 0;

bool isTime = 0;
bool isOther = 0;
String tubeTime;
String other;

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
}

void loop() {
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
}