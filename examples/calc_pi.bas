10 N=16 : M=10*(N+1)/3 : DIM D(M-1)
20 FOR T=0 TO M-1
30   D(T)=2
40 NEXT
50 P=0
60 FOR T=0 TO N
70   C=0
80   FOR A=M-1 TO 1 STEP-1
90     B=2*A+1 : F=D(A)*10+C : E=F/B : D(A)=F-E*B : C=A*E
100   NEXT
110   F=D(0)*10+C : E=F/10 : D(0)=F-E*10 : GOSUB 150
120 NEXT
130 E=0 : GOSUB 150 : PRINT : END
140 :
150 IF T=2 THEN PRINT".";
160 ON 2+SGN(E-9) GOTO 170,180,190
170 PRINT MID$(STR$(P),2,3); : P=10+E : RETURN
180 P=10*P+9 : RETURN
190 P=P+1 : GOSUB 170 : P=10 : RETURN
