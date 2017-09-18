# i3detroit-Laser-Cutter-Usage-Timer
This project is based on the Dallas Makerspace laser usage timer (https://github.com/cottjr/DMS-Laser-Cutter-Usage-Timer) with many software and hardware modifications to work on the laser cutters (G.Weike 150W) at i3Detroit.

ESP8266-serial branch intends to use serial communication between the timer and an ESP8266 which will publish timer stats via MQTT to be logged by our openhab2 instance.

Hardware:
- A timer board that contains a few components and an arduino (nano v3 used here) connected to an LCD, indicator LEDs, and a buttton. 
- An interface board, which sits inside the laser next to the HV power supply, passes the laser state from the power supply to the timer.
- See Documentation for schematics
