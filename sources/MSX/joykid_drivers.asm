; 
;      ____.              ____  __.__    .___
;     |    | ____ ___.__.|    |/ _|__| __| _/
;     |    |/  _ <   |  ||      < |  |/ __ | 
; /\__|    (  <_> )___  ||    |  \|  / /_/ | 
; \________|\____// ____||____|__ \__\____ | 
;                 \/             \/       \/ 
;   Sensor Kid for the Joystick port 
;   Danjovic 2024 
; 
; Protocol
;      ____________________________________________
; 8___/ ATT                                        \_______
;  ________________        ____      ____      ____________
; 7                \_sof__/    \____/    \____/           
;  _________         __________                     _______ 
; 6         \_ack___/ release  |  PORT 0 |  PORT 1 |
;
; 1------------------| d0 | d4 | d0 | d4 | d0 | d4 |-------
; 2------------------| d1 | d5 | d1 | d4 | d5 | d5 |-------
; 3------------------| d2 | d6 | d2 | d4 | d6 | d6 |-------
; 4------------------| d3 | d7 | d3 | d4 | d7 | d7 |-------
;
;  MSX: 
;  1 issue ATT (pin 8 high)
;  2 wait for ack. If timeout, goto step 17     
;  3 drop clock (pin 7 low, SOF)
;  4 read lower nibble
;  5 rise clock (pin 7)
;  6 read higher nibble 
;
;  7 write PORT0 value
;  8-11 repeat steps 3..6
; 
;  12 write PORT1 value
;  13-16 repeat steps 3..6
;
;  17 drop pin 8, rise pins 6 and 7


; APPENDIX: 
;
; MICROCONTROLLER MAIN LOOP
;
; Wait for ATT (pin 8 low)
; restart variables
; read ADC channels
; drop ack line
; wait for pin7 low (SOF). If pin 8 high, restart 
; release ack line
;
; write first lower nibble
; wait for pin 7 high. If pin 8 high, restart 
; write first high nibble
; 
; wait for pin 7 low. If pin 8 high, restart 
; read PORT 0 value
; write second lower nibble
; wait for pin 7 high. If pin 8 high, restart 
; write second high nibble
; 
; wait for pin 7 low. If pin 8 high, restart 
; read PORT 1 value
; write third lower nibble
; wait for pin 7 high. If pin 8 high, restart 
; write third  high nibble
; 
; wait for pin 8 high
; release all pins
; update PORT 0/1 output pins 
; do it all again



psgaddr: equ 0a0h
psgwr  : equ 0a1h
psgrd  : equ 0a2h

TRGA   : equ 4  ; bit 4 of register 14
PIN6   : equ 2  ; bit 2 of register 15
PIN7   : equ 3  ; bit 3 of register 15
PIN8   : equ 5  ; bit 5 of register 15


TIMEOUT: EQU 200;

; TRANSFER
; Transfer a block of data using port B
;
; inputs: 
; DE = address of block
;     
; Changes:
; AF,DE
; Port B pin 8=LOW, 6,7=HIGH (float)
;  
push hl
push bc

call JKDTRANSFER

pop bc
pop hl
ret


JKDTRANSFER:
	di               ; 5
	ld a,15          ; 8
	out (psgaddr),a  ; 12
	in a,(psgrd)     ; 12 
	
;  1 issue ATT (pin 8 high)	
    or  01101100b    ; 8  set ATT bit (pin 8 pulse), release triggers and select port b
    out (psgwr), a        ; 12 

;  2 wait for ack. If timeout, goto step 17  
   ld a,14
   out (psgaddr),a  ; 12
   ld HL,TIMEOUT    ; 11
   
.waitack
   in a,(psgrd)     ; 12 
   bit TRGA,a       ; 10 Did device acked ?    
   jr z, .acked     ; 13/8 2yes, continue
   dec hl           ; 7 no, check time 
   ld a,l           ; 5
   or h             ; 5 timed out ?
   jr nz,.waitack   ; 13/8 not yet, keep waiting
  ; 11+(12+10+8+7+5+5+13)*200-5 
  
  ld a,15          ;    ; timed out exit
  out (psgaddr),a  ; 
  in a,(psgrd)     ;  
  
  scf              ;    set carry flag to indicate I/O error
    
  jr JKstep17
  
.acked   

  ld a,(de) ; 8 get PORT value
  ld h,a    ; 5
  inc de    ; 7 advance pointer to first sensor value
  
  ld a,15          ; 8
  out (psgaddr),a  ; 12
  in a,(psgrd)     ; 12    ;  
  
  call JKgetData   ; steps (3..6)  ;  
                                   ;  
  ;  7 write PORT0 value  (l.0)
  bit 0,h
  res PIN6,a
  jr z,$+4
  set PIN6,a
 
;  8-11 repeat steps 3..6 
  call JKgetData
  
;  12 write PORT1 value
  bit 1,h
  res PIN6,a
  jr z,$+4
  set PIN6,a

;  13-16 repeat steps 3..6
  call JKgetData
  
  and a ; clear carry flag to indicate success

JKstep17
;  17 drop pin 8, rise pin 6,7
  set PIN6,a
  set PIN7,a
  res PIN8,a      
  out (psgwr), a   ; 12 
  
ret  
  
; Get two nibbles on the joystick port
; drop pin 7 and read the first nibble
; then rise pin 7 and read the second
; store the read byte in DE and increment DE  
; return with reg15 state in A 

; get data
JKgetData:  
;  3 drop clock (pin 7 low, SOF)
    res PIN7,a       ; 10 pin 7 LOW  
 	ld c,a           ; 5   c = reg 15 with pin 7 LOW 
	out (psgwr), a   ; 12   pins 6,7=low mark low byte 
	EX (SP),IY       ; 25  delay some cycles
	EX (SP),IY       ; 25  delay some cycles	
	
;  4 read lower nibble	
	ld a,14          ; 8
	out (psgaddr),a  ; 12  select register 14 
	in a,(psgrd)     ; 12  x x x x 3 2 1 0      ;  
                     ; from drop pin 7 to read: 25+25+8+12+12=82cycles = 22,9us
	and 0fh          ; 8   mask upper bits
	ld b,a           ; 5  save do..d3

;  5 rise clock (pin 7)					 ;   
	ld a,15          ; 8  select register 15
	out (psgaddr),a  ; 12
	ld a,c           ; 5  restore state of reg 15 
	set PIN7,a       ; 10  pin 7 = high mark high byte
	out (psgwr), a   ; 12 
	EX (SP),IY       ; 25  delay some cycles
	EX (SP),IY       ; 25  delay some cycles
	
;  6 read higher nibble 	     = 8+12+12 = 32 + 50 = 82cycles = 22,9us
	ld a,14          ; 8
	out (psgaddr),a  ; 12  select register 14 	
	in a,(psgrd)	 ; 12  x x x x 7 6 5 4 
                     ; from drop pin 7 to read: 25+25+8+12+12=82cycles = 22,9us
	                  ;   a = 7 6 5 4 3 2 1 0 
	rlca              ; 5     6 5 4 3 2 1 0 7   
	rlca              ; 5     5 4 3 2 1 0 7 0  
	rlca              ; 5     4 3 2 1 0 7 6 5
	rlca              ; 5     3 2 1 0 7 6 5 4 
	and 0f0h          ; 8     3 2 1 0 _ _ _ _ 
	or b              ; 5     assemble byte

; store value read    
	ld (de),a         ; 8
    inc de            ; 11	
; get reg 15 state
	ld a,15          ; 8  
	out (psgaddr),a  ; 12
	in a,(psgrd)     ;
    ret	



	



