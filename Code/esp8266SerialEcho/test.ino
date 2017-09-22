char laserTime[20];
int i = 0;

bool isNumeric(String str) {
  for (byte i = 0; i < str.length(); i++) {
    if(!isDigit(str.charAt(i))) {
	  Serial.print("Non-numeric: ");
	  Serial.println(str.charAt(i));
	  return false;
	}
    Serial.print("Numeric: ");
	Serial.println(str.charAt(i));
  }
  return true;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  i = 0;
  if (Serial.available()){
	delay(100);
    while(Serial.available() && i < 20) {
      laserTime[i++] = Serial.read();
    }
    laserTime[i++] = '\0';
	Serial.print("Input: ");
    Serial.println(laserTime);
	isNumeric(laserTime);
  }
}