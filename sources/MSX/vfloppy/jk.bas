10 'JOY SENSOR KIT TEST
20 BLOAD"main.bin":DEFUSR=&HC006:A=USR(0)
100 SCREEN 1:WIDTH32:COLOR 15,1,1
105 DEFINT L,T,S,F
110 FORI=0TO7:READA:B$=B$+CHR$(A):NEXT
115 SPRITE$(0)=B$
116 SPRITE$(1)=B$
117 SPRITE$(2)=B$
120 LOCATE8,0:PRINT"= Joy Sensor Kid ="
130 LOCATE  0,4:PRINT"Sensors:"
140 LOCATE 0,7:PRINT"Light:"
145 LOCATE13,7 :PRINT"|---------------|"
150 LOCATE 0,11:PRINT"Temperature:"
155 LOCATE13,11:PRINT"|---------------|"
160 LOCATE 0,15:PRINT"Sound:"
165 LOCATE13,15:PRINT"|---------------|"
170 LOCATE  6,19:PRINT"Port0:"
175 LOCATE 18,19:PRINT"Port1:"
190 ON ERROR GOTO 3000
200 GOSUB 2000'read data
210 GOSUB 1000'put sprites
215 LOCATE9,21:PRINT SPC(12)
220 A$=INKEY$:IFA$=CHR$(32)THEN STOP
230 IF A$="0" THEN GOSUB 2500
235 IF A$="1" THEN GOSUB 2550
240 GOTO 200
990 DATA 24,60,126,255,36,36,66,129
999 STOP
1000 PUT SPRITE0,(104+L\2,64),15,0
1010 PUT SPRITE1,(104+T\2,96 ),15,1
1020 PUT SPRITE2,(104+S\2,128),15,2
1030 RETURN
2000 'read sensor data
2010 CALL LIGHT(L)
2020 CALL TEMP(T)
2030 CALL HEAR(S)
2040 RETURN
2500 'change port0
2510 P=P+1 AND 1
2520 LOCATE 12,19:PRINT P
2530 'call PORT0(p)
2540 RETURN
2550 'change port1
2560 Q=Q+1 AND 1
2570 LOCATE 24,19:PRINT Q
2580 'call PORT1(q)
2590 RETURN
3000 'error
3010 LOCATE 9,21:PRINT"DISCONNECTED"
3020 RESUME 200
