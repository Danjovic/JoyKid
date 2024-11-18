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


; 
; Microcontroller Main Loop
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




;  ___         _                            _      _    _        
; / __|_  _ __| |_ ___ _ __   __ ____ _ _ _(_)__ _| |__| |___ ___
; \__ \ || (_-<  _/ -_) '  \  \ V / _` | '_| / _` | '_ \ / -_|_-<
; |___/\_, /__/\__\___|_|_|_|  \_/\__,_|_| |_\__,_|_.__/_\___/__/
;      |__/                                                      
; 
;INCLUDE "msx_system_vars.inc"
SLTATR: EQU 0FCC9H
PROCNM: EQU 0FD89H 
VALTYP: EQU 0F663H
RS2IQ:  equ 0FAF5H

; Sensor and port variables stored at unused rs232 queue
PORTVALUE: EQU RS2IQ
LIGHTVALUE: EQU RS2IQ+1
TEMPERATUREVALUE: EQU RS2IQ+2
SOUNDVALUE: EQU RS2IQ+3


;  ___ ___ ___  ___           _ _    
; | _ )_ _/ _ \/ __|  __ __ _| | |___
; | _ \| | (_) \__ \ / _/ _` | | (_-<
; |___/___\___/|___/ \__\__,_|_|_/__/
;                                    
; 
CALBAS : EQU 0159H
CHPUT:   EQU 00A2H
CHRGTR : EQU 0010H

;  ___   _   ___ ___ ___    __              _   _             
; | _ ) /_\ / __|_ _/ __|  / _|_  _ _ _  __| |_(_)___ _ _  ___
; | _ \/ _ \\__ \| | (__  |  _| || | ' \/ _|  _| / _ \ ' \(_-<
; |___/_/ \_\___/___\___| |_|  \_,_|_||_\__|\__|_\___/_||_/__/
;                                                             
; 
FRMEVL:  EQU 4C64h
FRESTR:  EQU 67D0h
ERRHAND: EQU 406FH
PTRGET:  EQU 5EA4h
CHKALF:  EQU 64A7h  ; check for uppercase
GETBYT:  EQU 521Ch  ; evaluate expression into a byte
DIOERR:  EQU 73B2H	; device i/o error	

;  ___ _            _   
; / __| |_ __ _ _ _| |_ 
; \__ \  _/ _` | '_|  _|
; |___/\__\__,_|_|  \__|
;                       

ORG 0c004h

START:

    ; Call handler pointer in Bank 3 fixed offset 4
    DW CALLHAND


    LD A,(#F344) ; SlotID of RAM in Bank 3 (#C000-#FFFF)
    ; This variable is ready available only when disk drive is present.
    AND A
    JP M,.SKIP   ; SlotID has SubSlot information
    AND 3
.SKIP
    AND 15
    LD E,A
    RLCA
    RLCA
    RLCA
    RLCA
    OR E
    AND 60
    LD D,0
    LD E,A
    LD HL,SLTATR+3 ; +3 for Bank 3
    ADD HL,DE
    SET 5,(HL)     ; Set bit 5 to enable CALL handler
    RET
 

; Handle for the CALL Command

CALLHAND:
 
    PUSH    HL
    LD    HL,CMDS            ; Table with "_" instructions
.CHKCMD:
    LD    DE,PROCNM
.LOOP:    LD    A,(DE)
    CP    (HL)
    JR    NZ,.TONEXTCMD    ; Not equal
    INC    DE
    INC    HL
    AND    A
    JR    NZ,.LOOP    ; No end of instruction name, go checking
    LD    E,(HL)
    INC    HL
    LD    D,(HL)
    POP    HL        ; routine address
    CALL    GETPREVCHAR
    CALL    .CALLDE        ; Call routine
    AND    A
    RET
 
.TONEXTCMD:
    LD    C,0FFH
    XOR    A
    CPIR            ; Skip to end of instruction name
    INC    HL
    INC    HL        ; Skip address
    CP    (HL)
    JR    NZ,.CHKCMD    ; Not end of table, go checking
    POP    HL
        SCF
    RET
 
.CALLDE:
    PUSH    DE
    RET
 
;---------------------------
CMDS:
 
; List of available instructions (as ASCIIZ) and execute address (as word)
    DEFB    "LIGHT",0
	DEFW    _LIGHT
   
    DEFB    "TEMP",0
	DEFW    _TEMP
	
	DEFB    "HEAR",0
	DEFW    _HEAR
   
	DEFB    "PORT0",0
	DEFW    _PORT0

	DEFB    "PORT1",0
	DEFW    _PORT1
   
 
    DEFB    0               ; No more instructions
 
;---------------------------

_PORT0:
   CALL EVALPARAM
   LD A,E
   OR A              ; test for zero
   LD A,(PORTVALUE)
   RES 0,A           ; reset bit
   JR Z,$+4          ; is it zero
   SET 0,A           ; no, clear relevant bit
   JR PORTCOMMON

   
_PORT1:
   CALL EVALPARAM
   LD A,E
   OR A              ; test for zero
   LD A,(PORTVALUE)
   RES 1,A           ; reset bit
   JR Z,$+4          ; is it zero
   SET 1,A           ; no, set relevant bit
   
PORTCOMMON:
   LD (PORTVALUE),A
   CALL TRANSFER
     ; TODO check for carry (interface timed out)
     LD    IX,DIOERR    ; Device I/O error
     JP c, CALBAS
   RET
   


_LIGHT:
    CALL EVALINTPARAM
	push de
	CALL TRANSFER
	LD A,(LIGHTVALUE)
	JR SENSORCOMMON
	
_TEMP:
    CALL EVALINTPARAM
	push de
	CALL TRANSFER
	LD A,(TEMPERATUREVALUE)
	JR SENSORCOMMON
	
_HEAR:
    CALL EVALINTPARAM
	push de
	CALL TRANSFER
	LD A,(SOUNDVALUE)
;	JR SENSORCOMMON
		

	
SENSORCOMMON:
    pop de
	; TODO check for carry (interface timed out)
       LD    IX,DIOERR    ; Device I/O error
       JP c, CALBAS   	
	LD (DE),A
	LD A,0
	OR A
	RET
	

EVALPARAM:
    ; check opening '('   
    CALL CHKCHAR   
    DB '('

    ; evaluate its value into a byte
    PUSH HL	
    LD IX,GETBYT  ; 64A7H 
    CALL CALBAS
    POP HL
    inc hl
    ; check ending ')' 
    CALL CHKCHAR
    DB ')'
    RET
	
EVALINTPARAM:
    ; check opening '('   
    CALL CHKCHAR   
    DB '('
	
    ; check if it is a variable
    PUSH HL
    LD IX,CHKALF  ; 64A7H 
    CALL CALBAS
    POP HL
    JP C,TYPE_MISMATCH    

    ; evaluate variable and get its type
    PUSH HL
    LD IX,FRMEVL ; 4C64H
    CALL CALBAS
    POP HL
    
    ; check variable type. 
    LD A,(VALTYP)   
    CP 2        ; is it integer ?
    JP NZ,TYPE_MISMATCH ; no, error

    ; get variable address (in DE)
    LD IX, PTRGET ;  5EA4h
    CALL CALBAS
    
    ; check ending ')' 
    CALL CHKCHAR
    DB ')'
    RET

 
CHKCHAR:
    CALL    GETPREVCHAR    ; Get previous basic char
    EX    (SP),HL
    CP    (HL)             ; Check if good char
    JR    NZ,SYNTAX_ERROR    ; No, Syntax error
    INC    HL
    EX    (SP),HL
    INC    HL        ; Get next basic char
 
GETPREVCHAR:
    DEC    HL
    LD    IX,CHRGTR
    JP      CALBAS
 
 
TYPE_MISMATCH:
        LD      E,13
        DB      1
 
SYNTAX_ERROR:
        LD      E,2
    LD    IX,ERRHAND    ; Call the Basic error handler
    JP    CALBAS


TRANSFER:
ld de,PORTVALUE ; first address of data buffer
zz0:
INCLUDE "joykid_drivers.asm"
zz1: