# Joy Kid 

This project started as an exercise of passing and receiving parameters using CALL function, but evolved to a hardware prototype using a microcontroller that sends data through the MSX joystick port in order to emulate the 
[Sensor Kid](https://www.msx.org/wiki/Micomsoft_Sensor_Kid) 

## Protocol

The circuit uses lightweight protocol that transfers sensor data using nibbles. The transfer is commanded by pin 8 (pulse) and the data transfer is paced by pin 7 (trigger B). Pin 6 (trigger A) is used both by the microcontroller to provide a 'ready' signal to the MSX and also to get PORT0/1 data.

![Picture](/docs/Joykid.png)

![Picture](/docs/Joykid_Capture.png)

