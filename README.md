# i3detroit-Laser-Cutter-Usage-Timer
This project is based on the Dallas Makerspace laser usage timer (https://github.com/cottjr/DMS-Laser-Cutter-Usage-Timer) with many software and hardware modifications. This implementation serves onlyto log usage data over MQTT with no user interface.

Hardware:
- The timer board that contains a few components and an arduino (nano v3 used here).
- An interface board, which sits inside the laser next to the HV power supply, passes the laser state from the power supply to the timer.
- An esp8266 connected via serial to the arduino to transmit usage time via MQTT.
- See Documentation for schematics
