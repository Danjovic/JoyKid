/*
     ____.              ____  __.__    .___
    |    | ____ ___.__.|    |/ _|__| __| _/
    |    |/  _ <   |  ||      < |  |/ __ | 
/\__|    (  <_> )___  ||    |  \|  / /_/ | 
\________|\____// ____||____|__ \__\____ | 
                \/             \/       \/ 
  Sensor Kid for the Joystick port 
  Danjovic 2024 
*/

/* Pinout

   PIC16F688
                 +--___--+ 
           +5V --|1    14|-- GND                          
     TRG-A RA5 --|2    13|-- RA0 LIGHT       \ 
     TRG-B RA4 --|3    12|-- RA1 TEMPERATURE | Sensors  
     PULSE RA3 --|4    11|-- RA2 SOUND       /
     PORT1 RC5 --|5    10|-- RC0 UP
     PORT0 RC4 --|6     9|-- RC1 DOWN                  
     RIGHT RC3 --|7     8|-- RC2 LEFT     
                 +-------+  
*/


/* JoyKid Protocol

 GETTING ATTENTION
 - MSX rise pin 8 to marks the beginning of a transfer and wait for the JoyKid to acknowledge by lowering pin 6 
 - Joykid regognizes the ATT request, reads the values of its analog sensors and then drop pin 6 
 
 Now both MSX and JoyKid are ready to TRANSFERING DATA 
 
 - MSX drops pin 7 to mark the start of a data frame.
 - JoyKid recognize the sof mark, writes 1st lower nibble and release pin 6
 - MSX reads the 1st lower nibble, then rise pin 7
 
 - Joykid recognize the change and write the 1st higher nibble
 - MSX reads the 1st higher nibble, then drop pin 7 and write PORT0 value 
 - Joykid recognize the change, read PORT0 value and and write the 2nd lower nibble
 - MSX reads the 2nd lower nibble, then rise pin 7
 - Joykid recognize the change and write the 2nd higher nibble
 - MSX reads the 1st higher nibble, then drop pin 7 and write PORT1 value 
 - Joykid recognize the change, read PORT1 value and and write the 3rd lower nibble
 - MSX reads the 3rd lower nibble, then rise pin 7
 - Joykid recognize the change and write the 3rd higher nibble
 - MSX reads the 3rd higher nibble, then drop pin 8 to finalize the transfer
 - Joykid recognize the change, release the data lines populate the outputs and wait for the start of a new transfer
 
 
 
 
; Protocol
;        ____________________________________________
;  8 ___/ ATT                                        \_______
;    ________________        ____      ____      ____________
;  7                 \_sof__/    \____/    \____/           
;    _________         __________                     _______ 
;  6          \_ack___/ release  |  PORT 0 |  PORT 1 |
;    
;  1 ------------------| d0 | d4 | d0 | d4 | d0 | d4 |-------
;  2 ------------------| d1 | d5 | d1 | d4 | d5 | d5 |-------
;  3 ------------------| d2 | d6 | d2 | d4 | d6 | d6 |-------
;  4 ------------------| d3 | d7 | d3 | d4 | d7 | d7 |-------
;
*/ 


//    _ _ _                 _        
//   | (_) |__ _ _ __ _ _ _(_)___ ___
//   | | | '_ \ '_/ _` | '_| / -_|_-<
//   |_|_|_.__/_| \__,_|_| |_\___/__/
//                                   


#include <pic16f688.h>
#include <stdint.h>
#include <stdbool.h>

// Fuse setting 
uint16_t __at _CONFIG configWord = _INTRC_OSC_NOCLKOUT & _CPD_OFF &  _CP_OFF &  _MCLRE_OFF & _WDT_OFF & _PWRTE_ON; // watchdog off

//       _      __ _      _ _   _             
//    __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___
//   / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-<
//   \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/
//                                            
#define LIGHT  0  
#define TEMP   1 
#define SOUND  2
 
#define LIGHT_CHANNEL (0<<2)  // masks for ADC
#define TEMP_CHANNEL  (1<<2)  // 
#define SOUND_CHANNEL (2<<2)  //

#define BIT_PORT0 0
#define BIT_PORT1 1

#define PORT0  RC4
#define PORT1  RC5

#define PULSE  RA3
#define TRGB   RA4
#define TRGA   RA5

#define PIN8  RA3
#define PIN7  RA4
#define PIN6  RA5
#define DIR_PIN6 TRISA5

#define Att()    PULSE==0
#define NotAtt() PULSE!=0
  
#define ReleasePin6()   do { DIR_PIN6 = 1;  PIN6 = 1; } while (0)
#define DropPin6()      do { DIR_PIN6 = 0;  PIN6 = 0; } while (0)

#define ReleaseOutputs() PORTC|=0x0f


//                 _      _    _        
//   __ ____ _ _ _(_)__ _| |__| |___ ___
//   \ V / _` | '_| / _` | '_ \ / -_|_-<
//    \_/\__,_|_| |_\__,_|_.__/_\___/__/
//      
bool Port0,Port1;
uint8_t lightSensor,temperatureSensor,soundSensor;

//                 _       _
//    _ __ _ _ ___| |_ ___| |_ _  _ _ __  ___ ___
//   | '_ \ '_/ _ \  _/ _ \  _| || | '_ \/ -_|_-<
//   | .__/_| \___/\__\___/\__|\_, | .__/\___/__/
//   |_|                       |__/|_|
void initHardware(void);
uint8_t readADC (uint8_t channelMask); 
void transferData(void);

//               _      
//    _ __  __ _(_)_ _  
//   | '  \/ _` | | ' \ 
//   |_|_|_\__,_|_|_||_|
//                      


void main (void)
{
    initHardware();
	
	// wait initially for Pin8 be unactive 
	while ( Att() ) {};

// - MSX rise pin 8 to marks the beginning of a transfer 
	for (;;) {
// - Joykid regognizes the ATT request, ...
		while (NotAtt()) { } ; // wait for the ATT
        transferData();
		ReleasePin6();
		ReleaseOutputs();
	}
}





void transferData(void)	{
	
	    uint8_t tempC = PORTC & 0xf0;
// ..., reads the values of its analog sensors and then drop pin 6 		
		// Read ADC values ( ~130us to read the three channels )
		lightSensor = readADC(LIGHT_CHANNEL);  
		temperatureSensor = readADC(TEMP_CHANNEL) ;
		soundSensor = readADC(SOUND_CHANNEL);
		DropPin6();	  
		
// - MSX drops pin 7 to mark the start of a data frame.
// - JoyKid recognize the sof mark, writes 1st lower nibble and release pin 6		
		while (PIN7) { if (NotAtt()) return; };
		PORTC = tempC | (lightSensor & 0x0f);
		ReleasePin6();
		
// - MSX reads the 1st lower nibble, then rise pin 7
// - Joykid recognize the change and write the 1st higher nibble
        while (PIN7==0) { if (NotAtt()) return; };
		PORTC = tempC | (lightSensor>>4);

// - MSX reads the 1st higher nibble, then drop pin 7 and write PORT0 value 
// - Joykid recognize the change, read PORT0 value and and write the 2nd lower nibble
		while (PIN7) { if (NotAtt()) return; };
		PORTC = tempC | (temperatureSensor & 0x0f);
		Port0 = PIN6; //(PIN6? 1: 0);   


// - MSX reads the 2nd lower nibble, then rise pin 7
// - Joykid recognize the change and write the 2nd higher nibble
        while (PIN7==0) { if (NotAtt()) return; };
		PORTC = tempC | (temperatureSensor>>4);


// - MSX reads the 1st higher nibble, then drop pin 7 and write PORT1 value 
// - Joykid recognize the change, read PORT1 value and and write the 3rd lower nibble
		while (PIN7) { if (NotAtt()) return; };
		PORTC = tempC | (soundSensor & 0x0f);
		Port1 = PIN6; //(PIN6? 1: 0);   


// - MSX reads the 3rd lower nibble, then rise pin 7
// - Joykid recognize the change and write the 3rd higher nibble
        while (PIN7==0) { if (NotAtt()) return; };
		PORTC = tempC | (soundSensor>>4);

// - MSX reads the 3rd higher nibble, then drop pin 8 to finalize the transfer
// - Joykid recognize the change, release the data lines populate the outputs and wait for the start of a new transfer
        while ( Att() ) {} ;  // 
		PORT0 = Port0;
		PORT1 = Port1;			 
}

uint8_t readADC (uint8_t channelMask) {
	//ADCON0 = (ADCON0 & 0b11100011) | channelMask ;  // select channel
 	ADCON0 = channelMask | _ADON;
	TMR0 = 216;         // preset for timer register
	TMR0IF = 1;  // clear interrupt flag
	while (!TMR0IF) {}; // wait 20us for sample/hold
	GO_NOT_DONE = 1;          // start the conversion
	while (GO_NOT_DONE) {};      // wait for conversion to end
	return ADRESH;
}

void initHardware(void) {
  // Initialize clock
   IRCF0 = 1 ;  // 8MHz -> IRCF[2..0] = 111 ( default 110 on startup )
   
   // Initialize I/O ports
   CMCON0 = ( _CM2 | _CM1 | _CM0); // analog comparators off, pins as I/O 
  
   TRISC=0b11000000; // RC0-RC5 as outputs
   PORTC=0b00001111; // U/D/L/R unactive(high), PORT0 PORT1 unactive (low)
    
   TRISA=0b11111111; // RA0-RA5 as inputs
   PORTA=0b00000000; // All low   
   
   // Initialize A/D converter
   ANSEL  = (1<<2)|(1<<1)|(1<<0); // Configure analog/digital I/Os (ANSEL)
   ADCON0 = _ADON;      //  ADFM = 0 , ADON = 1
   ADCON1 = 0b01110000;	 //  ADCS1 [2..0] = x11 -> FRC Internal 500KHz oscillator
   //ADON = 1;

}
