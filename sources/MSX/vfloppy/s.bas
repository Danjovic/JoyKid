5 GOSUB 1000
10 CLS
15 LOCATE 15,0: PRINT"== Joy Kid =="
20 LOCATE 10,3:PRINT"Light:";
30 LOCATE 10,8:PRINT"Sound:";
40 LOCATE 10,13:PRINT"Temp.:";
50 LOCATE 10,18: PRINT"Port0:"
60 LOCATE 25,18: PRINT"Port1:"
100 DEFINT A-C
110 '
120 ON INTERVAL = 60-10*(PEEK(&H2B)\128) GOSUB 500:INTERVAL ON
125 ON ERROR GOTO 900
130 A$=INKEY$
140 IF A$="0" THEN GOSUB 600
150 IF A$="1" THEN GOSUB 700
160 IF A$=CHR$(27) THEN STOP
170 GOTO 130
500 '
501 LOCATE12,21:PRINT SPC(13)
505 CALL LIGHT(A)
510 LOCATE 16,3:PRINT A
515 CALL HEAR(A)
520 LOCATE 16,8:PRINT B
525 CALL TEMP(A)
530 LOCATE 16,13:PRINT C
540 RETURN
600 '
610 P=P+1 AND 1
620 LOCATE 16,18:PRINT P
625 CALL PORT0(P)
630 RETURN
700 '
710 Q=Q+1 AND 1
720 LOCATE 31,18:PRINT Q
725 CALL PORT1(Q)
730 RETURN
900 '
910 LOCATE 12,21:PRINT"NOT CONNECTED";
920 RESUME 130
1000 '
1010 BLOAD"main.bin"
1020 DEFUSR=&HC006:A=USR(0)
1030 RETURN
