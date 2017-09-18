#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <Adafruit_NeoPixel.h>

//Pin Definitions
int userResetPin = 7;		// pin used for resetting user laser time	(Nano D7)
int analogPin = 0;			// input voltage connected to analog pin 0 from interface board
int pixelPin = 6;			// Neopixel data pin 
//analog input is on pin A0.
//serial communication for LCD is on pins A4 & A5


// Display config
#define MAX_OUT_CHARS 17  //max nbr of characters to be sent on any one serial command,
// plus the terminating character sprintf needs to prevent it from overwriting following
//memory locations, yet still send the complete 16 character string to the LCD

// Setup for the NXP PCF8574A I2C-to-Digital I/O Port
#define I2C_ADDR    0x27  // Define PCF8574A's I2C Address

// Parameters to define the size of the LCD array
#define lineLen 16
#define numLines 2
// LCD backlight state
#define LED_ON  1
#define LED_OFF  0

// Instantiate the I2C LCD object
LiquidCrystal_I2C lcd(I2C_ADDR,lineLen, numLines);

// Initialize neopixels
int numPixels = 3;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numPixels, pixelPin, NEO_GRB + NEO_KHZ800);
int errorLED = 2;
int laserOnLED = 1;
int activeSessionLED = 0;
uint32_t red = pixels.Color(200,0,0);
uint32_t blue = pixels.Color(0,0,200);
uint32_t green = pixels.Color(0,200,0);
uint32_t off = pixels.Color(0,0,0);

// Time constants
const unsigned long second = 1000;
const unsigned long minute = 60000;		// number of millis in a minute
const unsigned long hour = 3600000;		// number of millis in an hour

// Kept threshold values from DMS timer but swapped the logic in loop()
// because our laser uses active low instead of active high
int analogVal = 0;			// variable to store the analog value read
int anaLowThreshold = 300;	 // if analog goes above this value its considered ON
int anaHighThreshold = 324;	// if analog value goes below this value its considered OFF
int cursorPos = 0;
//Delay after seeing laser is on
int minLaserTime = 500; //effectively the shortest time the laser can be recorded as being active.

unsigned long millisOnLast = 0;
unsigned long millisOffLast = 0;
unsigned long millisTemp = 0;
unsigned long millisDiff = 0;
boolean lastLaserOn = false;
unsigned long userMillis = 0;
int userHours = 0;
int userMinutes = 0;				// number of minutes user has used the laser (resettable when button pressed)
int userSeconds = 0;
int tubeHours = 0;
int tubeMinutes = 0;				// number of minutes tube has been used (not resettable)
int tubeSeconds = 0;
unsigned long tubeMillis = 0;				
unsigned long lastWriteToEEPROMMillis = 0;	 // number of millis that the EEPROM was last written to

char   buffer[MAX_OUT_CHARS];  //buffer used to format a line (+1 is for trailing 0)
char   buffer2[MAX_OUT_CHARS];  //buffer used to format a line (+1 is for trailing 0)
char   costbuffer[10];			//buffer used to hold user cost as a string

float costPerMin = 0.20;

const unsigned int ThisCurrentVersion = 4;	// version number for this program.	simply counting releases
// did not increment version when forking for I2C LCD

//Pins for debugging loop timing
//int TestOutPin = 13;
//int TestOutPin2 = 8;
//boolean TestOutToggle2 = true;
//boolean TestOutToggle = true;

// Arduino EEPROM good for ~ 100,000 writes
// Arduino EEPROM good for ~ 100,000 writes
// only lasts ~ 1 year if write every 5 min, 24 x 7
// and have ~ 12 people hitting reset every day
struct config_t
{
	unsigned long seconds;				// tube seconds
	unsigned long uSeconds;				// user seconds
	unsigned long EEPROMwriteCount;		// EEPROM write cycle count
	unsigned int thisVersion;			// version number of this software
} laserTime;

void setup() {
	int addr = 0;
	pinMode(userResetPin, INPUT);
	//pinMode(TestOutPin, OUTPUT);
	//pinMode(TestOutPin2, OUTPUT);
	EEPROM_readAnything(addr,laserTime);
	Serial.begin(115200);
	tubeMillis = laserTime.seconds*1000;
	userMillis = laserTime.uSeconds*1000;
		// Initialize the version number in EEPROM if this is the first load after a reflash
	if ( laserTime.thisVersion == 0 )
	{
		laserTime.thisVersion = ThisCurrentVersion;
		laserTime.EEPROMwriteCount = laserTime.EEPROMwriteCount + 1;
		EEPROM_writeAnything(0, laserTime);
		// replace with Xerial.write()
		Serial.println(laserTime.seconds);
		
		//addr = ROUND_ROBIN_EEPROM_write(laserTime);
	}
		
	// Briefly show Arduino status
	sprintf(buffer, "Version: %02d", laserTime.thisVersion);
	sprintf(buffer2, "Writes: %06d", laserTime.EEPROMwriteCount);
		
	lcd.begin ();
	lcd.setBacklight(LED_ON);
	lcd.setCursor(0,0);
	lcd.print (buffer);
	lcd.setCursor(0,1);
	lcd.print (buffer2);
	
	//Start the neopixels
	pixels.begin(); // This initializes the NeoPixel library.
	pixels.show();
	
	//Delay before fully starting up
	lcd.begin ();
	lcd.setCursor(0,0);
	lcd.print (" LaserTimer 1.0 ");
	lcd.setCursor(0,1);
	lcd.print ("   Loading...   ");
	
	//turn on all pixels to test
	pixels.setPixelColor(errorLED, green);
	pixels.setPixelColor(laserOnLED, green);
	pixels.setPixelColor(activeSessionLED, green);
	pixels.show();

	delay(2000);	// cheap debouncing trick	

	/* Serial.print("Values stored in EEPROM address ");
	Serial.println(addr);


	Serial.print("	laserTime.seconds ie tube: ");
	Serial.println(laserTime.seconds);
	
	Serial.print("	laserTime.uSeconds ie user: ");
	Serial.println(laserTime.uSeconds);
	
	Serial.print("	laserTime.EEPROMwriteCount: ");
	Serial.println(laserTime.EEPROMwriteCount);

	Serial.print("	laserTime.thisVersion: ");
	Serial.println(laserTime.thisVersion);

	Serial.println("setup Complete");
	Serial.println(""); */
	
		//turn off the pixels
	pixels.setPixelColor(errorLED, off);
	pixels.setPixelColor(laserOnLED, off);
	pixels.setPixelColor(activeSessionLED, off);
	pixels.show();
}

void loop() {
	int addr = 0;
	//debug output to test loop timing
	/*if ( TestOutToggle2 ) {
	// nominally keep disabled, since Nano D13 annoyingly toggles an LED
		digitalWrite(TestOutPin2, HIGH);
	}
	else {
		digitalWrite(TestOutPin2, LOW);
	}
	TestOutToggle2 = !TestOutToggle2;*/
	// do a tight loop on checking the laser and keeping track of on/off times	
	for (int i=0; i <= 100; i++) {
		//reset the error led
		pixels.setPixelColor(errorLED, off);
		// read the input pin
		analogVal = analogRead(analogPin);
		
			// laser has been off, laser turning on here
		if ((analogVal <	anaLowThreshold) && !lastLaserOn) {
			lastLaserOn = true;
			millisOnLast = (unsigned long) millis();
			millisDiff = millisOnLast - millisOffLast;
			// set laser LED on
			pixels.setPixelColor(laserOnLED, red);
			delay(minLaserTime);
		}
			// laser has been on here, continuing on
		else if ((analogVal <	anaLowThreshold) && lastLaserOn) {
			lastLaserOn = true;

			millisTemp = (unsigned long) millis();
			millisDiff = millisTemp-millisOnLast;
			millisOnLast = millisTemp;
			delay(minLaserTime);
		}
			// laser has been on, turning off
		else if ((analogVal > anaHighThreshold) && lastLaserOn) {
			lastLaserOn = false;
			millisOffLast = (unsigned long) millis();
				//turn off laser LED
			pixels.setPixelColor(laserOnLED, off);
		}
			// laser has been off, staying off
		else {
			lastLaserOn = false;
			millisOffLast = (unsigned long) millis();
		}
			// check to set active session led
		if (userMillis > 0) {
			pixels.setPixelColor(activeSessionLED, blue);
		}
		int userReset = digitalRead(userResetPin);
			//User presses button when laser isn't firing
		if ((userReset == LOW) && !lastLaserOn && userMillis > 0) {
				//	allow reset and writing once every 10 seconds, but no faster
				//	write values to EPROM every time user hits reset
			if (millis() > (lastWriteToEEPROMMillis+10000)) {
				userMillis = 0;
				laserTime.seconds = tubeMillis/1000;
				laserTime.uSeconds = userMillis/1000;
				laserTime.EEPROMwriteCount = laserTime.EEPROMwriteCount + 1;
				laserTime.thisVersion = ThisCurrentVersion;
				//int addr = ROUND_ROBIN_EEPROM_write(laserTime);
				EEPROM_writeAnything(0, laserTime);
				lastWriteToEEPROMMillis = millis();
				
				// replace with Xerial.write()
				Serial.println(laserTime.seconds);
				
				/* Serial.println("User hit reset & Wrote to EEPROM");

				Serial.print("	EEPROM address: ");
				Serial.println(addr);

				Serial.print("	laserTime.seconds ie tube: ");
				Serial.println(laserTime.seconds);

				Serial.print("	laserTime.uSeconds ie user: ");
				Serial.println(laserTime.uSeconds);
		
				Serial.print("	laserTime.EEPROMwriteCount: ");
				Serial.println(laserTime.EEPROMwriteCount);
			
				Serial.print("	laserTime.thisVersion: ");
				Serial.println(laserTime.thisVersion); */
			}
				// display laser cost on screen for 5 sec
				//Create screen output
			sprintf(buffer,  "User    %02d:%02d:%02d", userHours,  userMinutes, userSeconds);
			float userCost = (userSeconds/60.0 + userMinutes + userHours*60.0)*costPerMin;
			
			dtostrf(userCost,7,2,costbuffer);
			//sprintf(buffer2, "Cost    $%s", costbuffer);
			sprintf(buffer2, "Cost        $TBD", costbuffer);
			
			//print to lcd
			lcd.setCursor(0,0);
			lcd.print(buffer);
			lcd.setCursor(0,1);
			lcd.print(buffer2);
			delay(5000);
			
				// turn off active session LED when button is pressed	
			pixels.setPixelColor(activeSessionLED, off);
		}
			//User presses button while laser is still firing
		else if ((userReset == LOW) && lastLaserOn){
				//Turn on the error LED
			pixels.setPixelColor(errorLED, red);
		}
			//User presses button with no time accrued -> do nothing, flash error
		else if ((userReset == LOW) && (userMillis == 0)) {
			pixels.setPixelColor(errorLED, red);
			pixels.show();
			delay(50);
			pixels.setPixelColor(errorLED, off);
		}
		userMillis = userMillis + millisDiff;
		tubeMillis = tubeMillis + millisDiff;
		millisDiff = 0;
			//finally change pixel colors
		pixels.show();	
	}

	tubeHours = tubeMillis/hour;
	tubeMinutes = (tubeMillis-tubeHours*hour)/minute;
	tubeSeconds = (tubeMillis-tubeHours*hour-tubeMinutes*minute)/second;
	userHours = userMillis/hour;
	userMinutes = (userMillis-userHours*hour)/minute;
	userSeconds = (userMillis-userHours*hour-userMinutes*minute)/second;
	
	//Create screen output
	sprintf(buffer,  "User    %02d:%02d:%02d", userHours,  userMinutes, userSeconds);
	//sprintf(buffer, "%12d", userMillis); //show raw milliseconds for debugging
	sprintf(buffer2, "Tube %05d:%02d:%02d", tubeHours,  tubeMinutes, tubeSeconds);
	
		// Only write to EEPROM if the current value is more than 5 minutes from the previous EEPROM value
		// to reduce the number of writes to EEPROM, since any one location is only good for ~ 100,000 writes
	EEPROM_readAnything(addr, laserTime);
	//int addr = ROUND_ROBIN_EEPROM_read(laserTime);
	unsigned long laserSeconds = laserTime.seconds;
	
		// note - it appears that only one of the following If statements is required	
	if ((laserSeconds+300) < (tubeMillis/1000)) {	
		/* Serial.print("LaserSeconds:");
		Serial.print(laserSeconds);
		Serial.print("adjTubeSecs:");
		Serial.println(((tubeMillis/1000)+300)); */
		laserTime.seconds = tubeMillis/1000;
		laserTime.uSeconds = userMillis/1000;
		laserTime.EEPROMwriteCount = laserTime.EEPROMwriteCount + 1;
		laserTime.thisVersion = ThisCurrentVersion;
		EEPROM_writeAnything(0, laserTime);
		//addr = ROUND_ROBIN_EEPROM_write(laserTime);
		lastWriteToEEPROMMillis = millis();
		
		// replace with Xerial.write()
		Serial.println(laserTime.seconds);
		
		/* Serial.println("Wrote to EEPROM - tube has another 5 minutes of use");
		
		Serial.print("	EEPROM address: ");
		Serial.println(addr);

		Serial.print("	laserTime.seconds ie tube: ");
		Serial.println(laserTime.seconds);
		
		Serial.print("	laserTime.uSeconds ie user: ");
		Serial.println(laserTime.uSeconds);
		
		Serial.print("	laserTime.EEPROMwriteCount: ");
		Serial.println(laserTime.EEPROMwriteCount);
		 
		Serial.print("	laserTime.thisVersion: ");
		Serial.println(laserTime.thisVersion); */
	 }	
/* 	if ((millis() > (lastWriteToEEPROMMillis+300000)) && ((laserSeconds+1)*1000 < tubeMillis)) {
		// ie. if it has been 5 mins since last write and the value has changed, write now
		laserTime.seconds = tubeMillis/1000;
		laserTime.uSeconds = userMillis/1000;
		laserTime.EEPROMwriteCount = laserTime.EEPROMwriteCount + 1;
		laserTime.thisVersion = ThisCurrentVersion;
		//addr = ROUND_ROBIN_EEPROM_write(laserTime);
		lastWriteToEEPROMMillis = millis();
		Serial.println("Wrote to EEPROM - value has changed in last 5 minutes");

		Serial.print("	EEPROM address: ");
		Serial.println(addr);

		Serial.print("	laserTime.seconds ie tube: ");
		Serial.println(laserTime.seconds);
		
		Serial.print("	laserTime.uSeconds ie user: ");
		Serial.println(laserTime.uSeconds);
		
		Serial.print("	laserTime.EEPROMwriteCount: ");
		Serial.println(laserTime.EEPROMwriteCount);

		Serial.print("	laserTime.thisVersion: ");
		Serial.println(laserTime.thisVersion);
	} */
		
	//Print user and tube times to screen
	lcd.setCursor(0,0);
	lcd.print(buffer);

	lcd.setCursor(0,1);
	lcd.print(buffer2);
}

