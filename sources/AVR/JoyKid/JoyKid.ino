/*.
     ____.              ____  __.__    .___
    |    | ____ ___.__.|    |/ _|__| __| _/
    |    |/  _ <   |  ||      < |  |/ __ | 
/\__|    (  <_> )___  ||    |  \|  / /_/ | 
\________|\____// ____||____|__ \__\____ | 
                \/             \/       \/ 
  Sensor Kid for the Joystick port 
  Danjovic 2024 
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
#include "stdbool.h"

//         _               _
//    _ __(_)_ _  ___ _  _| |_
//   | '_ \ | ' \/ _ \ || |  _|
//   | .__/_|_||_\___/\_,_|\__|
//   |_|

//                Arduino Nano
//                    ____
//               +---|    |---+
//               |O  |____|  O|
//              =|D13      D12|= MSX TRG B  7
//  +--[100R]-- =|3V3      D11|= MSX RIGHT  4
//  +---------- =|VREF     D10|= MSX LEFT   3
// Light Sensor =|A0/D14    D9|= MSX DOWN   2
// Temp. Sensor =|A1/D15    D8|= MSX UP     1
// Sound Sensor =|A2/D16    D7|=
//              =|A3/D17    D6|= PORT1
//              =|A4/D18    D5|= PORT0
//              =|A5/D19    D4|=
//              =|A6/D20    D3|= MSX TRG A  6
//              =|A7/D21    D2|= MSX PULSE  8
//    MSX VCC 5 =|5V       GND|= MSX GND    9
//              =|RST      RST|=
//              =|GND       RX|=
//              =|V1N       TX|=
//               |O          O|
//               +------------+
//




//       _      __ _      _ _   _
//    __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___
//   / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-<
//   \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/
//

// port B
#define MSXup 0        //
#define MSXdown 1      //
#define MSXleft 2      //
#define MSXright 3     //
#define MSXtriggerB 4  //

//  port C
#define LIGHT_CHANNEL A0  //
#define TEMP_CHANNEL A1   //
#define SOUND_CHANNEL A2  //

//  port D
#define MSXpulse 2     //
#define MSXtriggerA 3  //
#define PORT_0 6      //
#define PORT_1 5       //


// Registers associated with pins

#define PIN_PIN6 PIND
#define DDR_PIN6 DDRD
#define PORT_PIN6 PORTD

#define PIN_PIN7 PINB
#define DDR_PIN7 DDRB
#define PORT_PIN7 PORTB

#define PIN_PIN8 PIND
#define DDR_PIN8 DDRD
#define PORT_PIN8 PORTD


#define PIN_PIN1TO4 PINB
#define DDR_PIN1TO4 DDRB
#define PORT_PIN1TO4 PORTB


//
//    _ __  __ _ __ _ _ ___ ___
//   | '  \/ _` / _| '_/ _ (_-<
//   |_|_|_\__,_\__|_| \___/__/
//
#define Pin8isLow() (PIN_PIN8 & (1 << MSXpulse)) == 0
#define Pin8isHigh() (PIN_PIN8 & (1 << MSXpulse)) != 0

#define Pin7isLow() (PIN_PIN7 & (1 << MSXtriggerB)) == 0
#define Pin7isHigh() (PIN_PIN7 & (1 << MSXtriggerB)) != 0

#define Pin6isLow() (PIN_PIN6 & (1 << MSXtriggerA)) == 0
#define Pin6isHigh() (PIN_PIN6 & (1 << MSXtriggerA)) != 0

//#define Pin8()   PIN_PIN8&(1<<MSXpulse)!=0    // get the state of pin 8
//#define Pin7()   PIN_PIN7&(1<<MSXtriggerB)!=0    // get the state of pin 8
//#define Pin6()   PIN_PIN6&(1<<MSXtriggerA)!=0    // get the state of pin 8


#define DropPin6() DDRD |= (1 << MSXtriggerA)
#define ReleasePin6() DDRD &= ~(1 << MSXtriggerA)
#define ReleaseOutputs() DDRB &= ~((1 << MSXup) | (1 << MSXdown) | (1 << MSXleft) | (1 << MSXright))


//                 _      _    _
//   __ ____ _ _ _(_)__ _| |__| |___ ___
//   \ V / _` | '_| / _` | '_ \ / -_|_-<
//    \_/\__,_|_| |_\__,_|_.__/_\___/__/
//
bool Port0, Port1;
uint8_t lightSensor, temperatureSensor, soundSensor;

//                 _       _
//    _ __ _ _ ___| |_ ___| |_ _  _ _ __  ___ ___
//   | '_ \ '_/ _ \  _/ _ \  _| || | '_ \/ -_|_-<
//   | .__/_| \___/\__\___/\__|\_, | .__/\___/__/
//   |_|                       |__/|_|

void transferData(void);

//               _
//    _ __  __ _(_)_ _
//   | '  \/ _` | | ' \ 
//   |_|_|_\__,_|_|_||_|
//


void setup() {
  Serial.begin(115200);
  // initialize pins
  pinMode(MSXup, INPUT);        //
  pinMode(MSXdown, INPUT);      //
  pinMode(MSXleft, INPUT);      //
  pinMode(MSXright, INPUT);     //
  pinMode(MSXtriggerB, INPUT);  //
  pinMode(MSXtriggerA, INPUT);  //
  pinMode(PORT_0, OUTPUT);      //
  pinMode(PORT_1, OUTPUT);      //

  digitalWrite(MSXup, LOW);        // No pull-ups, use MSX internal pullups
  digitalWrite(MSXdown, LOW);      //
  digitalWrite(MSXleft, LOW);      //
  digitalWrite(MSXright, LOW);     //
  digitalWrite(MSXtriggerB, LOW);  //
  digitalWrite(MSXtriggerA, LOW);  //
  digitalWrite(PORT_0, LOW);       // Powerup state
  digitalWrite(PORT_1, LOW);       // Powerup state

  //analogReference(EXTERNAL);   // required for the sound amplifier
  ReleasePin6();
  Serial.print("wait...");
  // wait initially for Pin8 be unactive
  while (Pin8isHigh()) {};
  Serial.println("Go!");
}

void loop() {
  // - MSX rise pin 8 to marks the beginning of a transfer
  // - Joykid regognizes the ATT request, ...
  while (Pin8isLow()) {};  // wait for the ATT
  transferData();
  ReleasePin6();
  ReleaseOutputs();
  //    Serial.print("@");
}



void transferData(void) {
  cli();
  volatile uint8_t tempDDR = DDR_PIN1TO4 & ~((1 << MSXup) | (1 << MSXdown) | (1 << MSXleft) | (1 << MSXright));
  // ..., reads the values of its analog sensors and then drop pin 6
  // Read ADC values ( ~130us to read the three channels )
  lightSensor = analogRead(LIGHT_CHANNEL) >> 2;
  temperatureSensor = analogRead(TEMP_CHANNEL) >> 2;
  soundSensor = analogRead(SOUND_CHANNEL) >> 2;
  DropPin6();

  // - MSX drops pin 7 to mark the start of a data frame.
  // - JoyKid recognize the sof mark, writes 1st lower nibble and release pin 6
  while (Pin7isHigh()) { if (Pin8isLow()) return; };
  DDR_PIN1TO4 = (tempDDR | (~lightSensor & 0x0f));  // invert the bits to write to DDR
  ReleasePin6();


  // - MSX reads the 1st lower nibble, then rise pin 7
  // - Joykid recognize the change and write the 1st higher nibble
  while (Pin7isLow()) { if (Pin8isLow()) return; };
  DDR_PIN1TO4 = (tempDDR | ((~lightSensor >> 4) & 0x0f));

  // - MSX reads the 1st higher nibble, then drop pin 7 and write PORT0 value
  // - Joykid recognize the change, read PORT0 value and and write the 2nd lower nibble
  while (Pin7isHigh()) { if (Pin8isLow()) return; };
  DDR_PIN1TO4 = (tempDDR | (~temperatureSensor & 0x0f));
  Port0 = (Pin6isHigh() ? true : false);


  // - MSX reads the 2nd lower nibble, then rise pin 7
  // - Joykid recognize the change and write the 2nd higher nibble
  while (Pin7isLow()) { if (Pin8isLow()) return; };
  DDR_PIN1TO4 = (tempDDR | ((~temperatureSensor >> 4) & 0x0f));


  // - MSX reads the 1st higher nibble, then drop pin 7 and write PORT1 value
  // - Joykid recognize the change, read PORT1 value and and write the 3rd lower nibble
  while (Pin7isHigh()) { if (Pin8isLow()) return; };
  DDR_PIN1TO4 = (tempDDR | (~soundSensor & 0x0f));
  Port1 = (Pin6isHigh() ? true : false);


  // - MSX reads the 3rd lower nibble, then rise pin 7
  // - Joykid recognize the change and write the 3rd higher nibble
  while (Pin7isLow()) { if (Pin8isLow()) return; };
  DDR_PIN1TO4 = (tempDDR | ((~soundSensor >> 4) & 0x0f));


  // - MSX reads the 3rd higher nibble, then drop pin 8 to finalize the transfer
  // - Joykid recognize the change, release the data lines populate the outputs and wait for the start of a new transfer
  while (Pin8isHigh()) {};  // wait pin 8 to fall, data lines released right after return
  digitalWrite(PORT_0, Port0);
  digitalWrite(PORT_1, Port1);
  sei();
  //    Serial.print("%");
  //
}
