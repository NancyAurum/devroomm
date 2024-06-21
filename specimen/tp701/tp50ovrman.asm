DSEG   SEGMENT WORD
       ASSUME DS:DSEG
       EXTRN HEAPORG                   : WORD
       EXTRN HEAPPTR                   : WORD
       EXTRN OVRDEBUGPTR               : WORD
       EXTRN OVRLOADLIST               : WORD
       EXTRN OVRHEAPSIZE               : WORD
       EXTRN OVRHEAPEND                : WORD
       EXTRN OVRHEAPPTR                : WORD
       EXTRN OVRDOSHANDLE              : WORD
       EXTRN OVREMSHANDLE              : WORD
       EXTRN EXITPROC                  : WORD
       EXTRN PREFIXSEG                 : WORD
       EXTRN OVRCODELIST               : WORD
       EXTRN OVRHEAPORG                : WORD
       EXTRN OVRRESULT                 : WORD
DSEG   ENDS

CSEG   SEGMENT WORD
       ASSUME CS:CSEG

       LOCALS @@

OVRINITEMS$0:
       DB        69,  77,  77,  88,  88,  88,  88,  48  ; EMMXXXX0
OVRINITEMS PROC    FAR
       PUBLIC  OVRINITEMS
       XOR     AX,AX
       CMP     AX,[OVRDOSHANDLE]
       JNZ     @@1
       DEC     AX
       JMP     SHORT @@2
@@1:   CALL    NEAR PTR @@3
       JZ      @@4
       MOV     AX,+65531
       JMP     SHORT @@2
@@4:   CALL    NEAR PTR @@5
       JNB     @@6
       MOV     AX,+65530
       JMP     SHORT @@2
@@6:   CALL    NEAR PTR @@7
       JNB     @@8
       MOV     DX,[OVREMSHANDLE]
       MOV     AH,+69      ;  'E'
       INT     067h
       MOV     AX,+65532
       JMP     SHORT @@2
@@8:   MOV     BX,[OVRDOSHANDLE]
       MOV     AH,+62      ;  '>'
       INT     021h
       MOV     WORD PTR [OFFSET OVRINITEMS$0+368],OFFSET ????
       LES     AX,DWORD PTR [EXITPROC]
       MOV     [????],AX
       MOV     [????],ES
       MOV     WORD PTR [EXITPROC],EXITPROC
       MOV     [EXITPROC+2],CS
       XOR     AX,AX
@@2:   MOV     [OVRRESULT],AX
       RETF
       MOV     DX,[OVREMSHANDLE]
       MOV     AH,+69      ;  'E'
       INT     067h
       LES     AX,DWORD PTR [????]
       MOV     [EXITPROC],AX
       MOV     [EXITPROC+2],ES
       RETF
@@3:   MOV     AX,+13671
       INT     021h
       MOV     CX,+8
       MOV     SI,OFFSET OVRINITEMS$0
       MOV     DI,+10
       PUSH    DS
       PUSH    CS
       POP     DS
       CLD
       REPE
       CMPSB
       POP     DS
       RET
@@5:   MOV     AH,+65      ;  'A'
       INT     067h
       MOV     [????],BX
       MOV     AX,+16383
       XOR     DX,DX
       MOV     BX,[OVRCODELIST]
@@9:   ADD     BX,[PREFIXSEG]
       ADD     BX,+16
       MOV     ES,BX
       ES:
       ADC     DX,+0
       ES:
       OR      BX,BX
       JNZ     @@9
       MOV     BX,+16384
       DIV     BX
       MOV     BX,AX
       MOV     AH,+67      ;  'C'
       INT     067h
       SHL     AH,1
       JB      @@10
       MOV     [OVREMSHANDLE],DX
@@10:  RET
@@7:   PUSH    BP
       MOV     BP,SP
       MOV     DX,[OVREMSHANDLE]
       MOV     AH,+71      ;  'G'
       INT     067h
       MOV     AX,[OVRCODELIST]
       XOR     CX,CX
@@11:  ADD     AX,[PREFIXSEG]
       ADD     AX,+16
       MOV     ES,AX
       PUSH    AX
       INC     CX
       ES:
       OR      AX,AX
       JNZ     @@11
       XOR     BX,BX
       XOR     DI,DI
@@14:  POP     ES
       PUSH    CX
       MOV     AX,[OVRHEAPORG]
       ES:
       ES:
       ES:
       PUSH    BX
       PUSH    DI
       CALL    [????]
       POP     DI
       POP     BX
       ES:
       JB      @@12
       CALL    NEAR PTR @@13
       JB      @@12
       POP     CX
       LOOP    @@14
@@12:  PUSHF
       MOV     DX,[OVREMSHANDLE]
       MOV     AH,+72      ;  'H'
       INT     067h
       POPF
       MOV     SP,BP
       POP     BP
       RET
@@13:  ES:
       XOR     SI,SI
@@19:  OR      DI,DI
       JNZ     @@15
       PUSH    DX
       MOV     DX,[OVREMSHANDLE]
       MOV     AX,+17408
       INT     067h
       POP     DX
       SHL     AH,1
       JB      @@16
@@15:  MOV     CX,+16384
       SUB     CX,DI
       CMP     CX,DX
       JB      @@17
       MOV     CX,DX
@@17:  SUB     DX,CX
       PUSH    DS
       PUSH    ES
       MOV     ES,[????]
       MOV     DS,[OVRHEAPORG]
       CLD
       REPE
       MOVSB
       POP     ES
       POP     DS
       CMP     DI,+16384
       JNZ     @@18
       INC     BX
       XOR     DI,DI
@@18:  OR      DX,DX
       JNZ     @@19
@@16:  RET
       MOV     DX,[OVREMSHANDLE]
       MOV     AH,+71      ;  'G'
       INT     067h
       ES:
       ES:
       ES:
       XOR     DI,DI
@@22:  MOV     CX,+16384
       SUB     CX,SI
       CMP     CX,DX
       JB      @@20
       MOV     CX,DX
@@20:  SUB     DX,CX
       PUSH    DX
       MOV     DX,[OVREMSHANDLE]
       MOV     AX,+17408
       INT     067h
       POP     DX
       SHL     AH,1
       JB      @@21
       PUSH    DS
       PUSH    ES
       MOV     DS,[????]
       ES:
       CLD
       REPE
       MOVSB
       POP     ES
       POP     DS
       INC     BX
       XOR     SI,SI
       OR      DX,DX
       JNZ     @@22
@@21:  PUSHF
       MOV     DX,[OVREMSHANDLE]
       MOV     AH,+72      ;  'H'
       INT     067h
       POPF
       RET
OVRINITEMS ENDP

OVRINIT$0:
       DB         0,   0,   0,   0,  80,  65,  84,  72  ; ....PATH
       DB        61                                     ; =
OVRINIT PROC    FAR
       PUBLIC  OVRINIT
       PUSH    BP
       MOV     BP,SP
       SUB     SP,+128
       CLD
       CMP     WORD PTR [OVRCODELIST],+0
       JZ      @@1
       CALL    NEAR PTR @@2
       JNB     @@3
       CALL    NEAR PTR @@4
       JNB     @@3
       CALL    NEAR PTR @@5
       JB      @@6
@@3:   MOV     BX,AX
       PUSH    DS
       LEA     DX,DWORD PTR [BP-4]
       PUSH    SS
       POP     DS
       MOV     CX,+4
       MOV     AH,+63      ;  '?'
       INT     021h
       POP     DS
       JB      @@7
       CMP     WORD PTR [BP-4],+20564
       JNZ     @@7
       CMP     WORD PTR [BP-2],+22095
       JNZ     @@7
       MOV     [OVRDOSHANDLE],BX
       MOV     WORD PTR [OFFSET @@5+262],OFFSET ????
       PUSH    DS
       MOV     DX,OFFSET @@5+367
       PUSH    CS
       POP     DS
       MOV     AX,+9535
       INT     021h
       POP     DS
       XOR     AX,AX
@@8:   MOV     [OVRRESULT],AX
       MOV     SP,BP
       POP     BP
       RETF    +4
@@7:   MOV     AH,+62      ;  '>'
       INT     021h
@@1:   MOV     AX,+65535
       JMP     SHORT @@8
@@6:   MOV     AX,+65534
       JMP     SHORT @@8
@@2:   PUSH    DS
       LEA     DI,DWORD PTR [BP-128]
       PUSH    SS
       POP     ES
       CALL    NEAR PTR @@9
       POP     DS
       RET
@@4:   MOV     AH,+48      ;  '0'
       INT     021h
       CMP     AL,+3
       JB      @@10
       PUSH    DS
       MOV     DS,[PREFIXSEG]
       MOV     DS,+44]
       XOR     SI,SI
@@11:  LODSB
       OR      AL,AL
       JNZ     @@11
       LODSB
       OR      AL,AL
       JNZ     @@11
       LODSW
       LEA     DI,DWORD PTR [BP-128]
       PUSH    SS
       POP     ES
       MOV     BX,DI
@@13:  LODSB
       STOSB
       OR      AL,AL
       JZ      @@12
       CMP     AL,+92; '\'
       JNZ     @@13
       MOV     BX,DI
       JMP     SHORT @@13
@@12:  MOV     DI,BX
       CALL    NEAR PTR @@9
       POP     DS
@@10:  RET
@@5:   PUSH    DS
       MOV     DS,[PREFIXSEG]
       MOV     DS,+44]
       XOR     SI,SI
@@16:  MOV     DI,OFFSET OVRINIT$0+4
       PUSH    CS
       POP     ES
       MOV     CX,+5
       REPE
       CMPSB
       JZ      @@14
       DEC     SI
@@15:  LODSB
       OR      AL,AL
       JNZ     @@15
       CMP     AL,[SI]
       JNZ     @@16
@@17:  POP     DS
       STC
       RET
@@14:  CMP     BYTE PTR [SI],+0
       JZ      @@17
       LEA     DI,DWORD PTR [BP-128]
       PUSH    SS
       POP     ES
       XOR     AL,AL
@@20:  MOV     AH,AL
       LODSB
       OR      AL,AL
       JZ      @@18
       CMP     AL,+59; ';'
       JZ      @@19
       STOSB
       JMP     SHORT @@20
@@18:  DEC     SI
@@19:  CMP     AH,+58
       JZ      @@21
       CMP     AH,+92
       JZ      @@21
       MOV     AL,+92      ;  '\'
       STOSB
@@21:  PUSH    DS
       PUSH    SI
       CALL    NEAR PTR @@9
       POP     SI
       POP     DS
       JB      @@14
       POP     DS
       RET
@@9:   LDS     SI,DWORD PTR [BP+6]
       LODSB
       MOV     CL,AL
       XOR     CH,CH
       REPE
       MOVSB
       XOR     AL,AL
       STOSB
       LEA     DX,DWORD PTR [BP-128]
       PUSH    SS
       POP     DS
       MOV     AX,+15616
       INT     021h
       RET
OVRINIT ENDP

OVRGETBUF PROC    FAR
       PUBLIC  OVRGETBUF
@@$RESULT_LO   EQU     [BP-4]
@@$RESULT_HI   EQU     [BP-2]
       MOV     AX,[OVRHEAPEND]
       SUB     AX,[OVRHEAPORG]
       MOV     CL,+4
       ROL     AX,CL
       MOV     DX,AX
       AND     AX,+65520
       AND     DX,+15
       RETF
OVRGETBUF ENDP

OVRCLEARBUF$0:
       DB         0,   0,   0,   0,  80,  65,  84,  72  ; ....PATH
       DB        61,  85, 139, 236, 129, 236, 128,   0  ; =U‹μμ€.
       DB       252, 131,  62,   0,   0,   0, 116,  84  ; όƒ>...tT
       DB       232,  91,   0, 115,  10, 232,  97,   0  ; θ[.s.θa.
       DB       115,   5, 232, 150,   0, 114,  74, 139  ; s.θ–.rJ‹
       DB       216,  30, 141,  86, 252,  22,  31, 185  ; ΨVό.Ή
       DB         4,   0, 180,  63, 205,  33,  31, 114  ; ..΄?Ν!r
       DB        47, 129, 126, 252,  84,  80, 117,  40  ; /~όTPu(
       DB       129, 126, 254,  79,  86, 117,  33, 137  ; ~ώOVu!‰
       DB        30,   0,   0, 199,   6,   0,   0, 208  ; ..Η...Π
       DB         1,  30, 186,  57,   2,  14,  31, 184  ; .Ί9..Έ
       DB        63,  37, 205,  33,  31,  51, 192, 163  ; ?%Ν!3ΐ£
       DB         0,   0, 139, 229,  93, 202,   4,   0  ; ..‹ε]Κ..
       DB       180,  62, 205,  33, 184, 255, 255, 235  ; ΄>Ν!Έÿÿλ
       DB       238, 184, 254, 255, 235, 233,  30, 141  ; ξΈώÿλι
       DB       126, 128,  22,   7, 232, 147,   0,  31  ; ~€..θ“.
       DB       195, 180,  48, 205,  33,  60,   3, 114  ; Γ΄0Ν!<.r
       DB        49,  30, 142,  30,   0,   0, 142,  30  ; 1..
       DB        44,   0,  51, 246, 172,  10, 192, 117  ; ,.3φ¬.ΐu
       DB       251, 172,  10, 192, 117, 246, 173, 141  ; ϋ¬.ΐuφ­
       DB       126, 128,  22,   7, 139, 223, 172, 170  ; ~€..‹ί¬ª
       DB        10, 192, 116,   8,  60,  92, 117, 246  ; .ΐt.<\uφ
       DB       139, 223, 235, 242, 139, 251, 232,  89  ; ‹ίλς‹ϋθY
       DB         0,  31, 195,  30, 142,  30,   0,   0  ; .Γ..
       DB       142,  30,  44,   0,  51, 246, 191,   4  ; ,.3φΏ.
       DB         0,  14,   7, 185,   5,   0, 243, 166  ; ...Ή..σ¦
       DB       116,  13,  78, 172,  10, 192, 117, 251  ; t.N¬.ΐuϋ
       DB        58,   4, 117, 234,  31, 249, 195, 128  ; :.uκωΓ€
       DB        60,   0, 116, 248, 141, 126, 128,  22  ; <.tψ~€.
       DB         7,  50, 192, 138, 224, 172,  10, 192  ; .2ΐΰ¬.ΐ
       DB       116,   7,  60,  59, 116,   4, 170, 235  ; t.<;t.ªλ
       DB       242,  78, 128, 252,  58, 116,   8, 128  ; ςN€ό:t.€
       DB       252,  92, 116,   3, 176,  92, 170,  30  ; ό\t.°\ª
       DB        86, 232,   6,   0,  94,  31, 114, 207  ; Vθ..^rΟ
       DB        31, 195, 197, 118,   6, 172, 138, 200  ; ΓΕv.¬Θ
       DB        50, 237, 243, 164,  50, 192, 170, 141  ; 2νσ¤2ΐª
       DB        86, 128,  22,  31, 184,   0,  61, 205  ; V€.Έ.=Ν
       DB        33, 195,  85, 139, 236,  51, 192,  59  ; !ΓU‹μ3ΐ;
       DB         6,   0,   0, 116,  78,  59,   6,   0  ; ...tN;..
       DB         0, 117,  72, 161,   2,   0,  43,   6  ; .uH΅..+.
       DB         2,   0,  11,   6,   0,   0, 117,  59  ; ......u;
       DB       139,  70,   6, 139,  86,   8, 177,   4  ; ‹F.‹V.±.
       DB       211, 232, 211, 202, 129, 226,   0, 240  ; ΣθΣΚβ.π
       DB        11, 194,  59,   6,   0,   0, 114,  35  ; .Β;...r#
       DB         3,   6,   0,   0, 114,  34, 142,   6  ; ....r".
       DB         0,   0,  38,  59,   6,   2,   0, 119  ; ..&;...w
       DB        23, 163,   0,   0, 163,   2,   0, 163  ; .£..£..£
       DB         2,   0,  51, 192, 163,   0,   0,  93  ; ..3ΐ£..]
       DB       202,   4,   0, 184, 255, 255, 235, 244  ; Κ..Έÿÿλτ
       DB       184, 253, 255, 235, 239, 161,   0,   0  ; Έύÿλο΅..
       DB        43,   6,   0,   0, 177,   4, 211, 192  ; +...±.Σΐ
       DB       139, 208,  37, 240, 255, 131, 226,  15  ; ‹Π%πÿƒβ.
       DB       203                                     ; Λ
OVRCLEARBUF PROC    FAR
       PUBLIC  OVRCLEARBUF
       PUSH    BP
       MOV     BP,SP
       CMP     WORD PTR [OVRDOSHANDLE],+0
       JZ      @@1
       MOV     AX,[OVRHEAPORG]
       MOV     [OVRHEAPPTR],AX
       MOV     AX,[OVRLOADLIST]
       JMP     SHORT @@2
@@4:   MOV     ES,AX
       CALL    NEAR PTR @@3
       ES:
@@2:   OR      AX,AX
       JNZ     @@4
       MOV     [OVRLOADLIST],AX
@@5:   MOV     [OVRRESULT],AX
       POP     BP
       RETF
@@1:   MOV     AX,+65535
       JMP     SHORT @@5
       PUSH    DS
       MOV     BX,[OVRDOSHANDLE]
       MOV     DI,[PREFIXSEG]
       ADD     DI,+16
       ES:
       ES:
       MOV     AX,+16896
       INT     021h
       ES:
       XOR     DX,DX
       ES:
       MOV     AH,+63      ;  '?'
       INT     021h
       JB      @@6
       CMP     AX,CX
       JB      @@6
       ADD     AX,+15
       MOV     CL,+4
       SHR     AX,CL
       ES:
       MOV     DS,AX
       XOR     DX,DX
       ES:
       JCXZ    @@7
       MOV     AH,+63      ;  '?'
       INT     021h
       JB      @@6
       CMP     AX,CX
       JB      @@6
       SHR     CX,1
       XOR     SI,SI
       PUSH    ES
       ES:
       CLD
@@8:   LODSW
       MOV     BX,AX
       ADD     ES:[BX],DI
       LOOP    @@8
       POP     ES
@@7:   CLC
@@6:   POP     DS
       RET
       POP     CS:[OFFSET OVRCLEARBUF$0]
       POP     CS:[OFFSET OVRCLEARBUF$0+2]
       POPF
       PUSH    BP
       MOV     BP,SP
       PUSH    AX
       PUSH    BX
       PUSH    CX
       PUSH    DX
       PUSH    SI
       PUSH    DI
       PUSH    DS
       PUSH    ES
       PUSHF
       MOV     AX,SEG DSEG
       MOV     DS,AX
       LES     BX,DWORD PTR CS:[OFFSET OVRCLEARBUF$0]
       MOV     AX,ES:[BX]
       MOV     CS:[OFFSET OVRCLEARBUF$0],AX
       CMP     BX,+2
       JNZ     @@9
       MOV     BP,[BP+0]
@@9:   CALL    NEAR PTR @@10
       LES     BX,DWORD PTR CS:[OFFSET OVRCLEARBUF$0]
       ES:
       MOV     CS:[OFFSET OVRCLEARBUF$0+2],AX
       MOV     AX,[OVRDEBUGPTR]
       OR      AX,[OVRDEBUGPTR+2]
       JZ      @@11
       CALL    [OVRDEBUGPTR]
@@11:  POPF
       POP     ES
       POP     DS
       POP     DI
       POP     SI
       POP     DX
       POP     CX
       POP     BX
       POP     AX
       POP     BP
       JMP     CS:[OFFSET OVRCLEARBUF$0]
@@10:  PUSH    ES
       PUSH    BX
       CALL    NEAR PTR @@12
       ES:
       ADD     DX,+15
       MOV     CL,+4
       SHR     DX,CL
       ADD     DX,AX
       MOV     AX,[OVRLOADLIST]
       OR      AX,AX
       JZ      @@13
       MOV     ES,AX
       ES:
       SUB     AX,[OVRHEAPPTR]
       JNB     @@14
       MOV     AX,[OVRHEAPEND]
       SUB     AX,[OVRHEAPPTR]
@@14:  SUB     DX,AX
       JBE     @@13
@@15:  MOV     ES,[OVRLOADLIST]
       ES:
       MOV     [OVRLOADLIST],AX
       PUSH    DX
       CALL    NEAR PTR @@3
       CALL    NEAR PTR @@12
       POP     DX
       SUB     DX,AX
       JA      @@15
       MOV     AX,[OVRLOADLIST]
       OR      AX,AX
       JZ      @@16
       MOV     ES,AX
       ES:
       CMP     DX,[OVRHEAPPTR]
       JA      @@13
       XOR     CX,CX
@@17:  INC     CX
       PUSH    AX
       MOV     ES,AX
       ES:
       OR      AX,AX
       JNZ     @@17
       MOV     [OVRLOADLIST],AX
       MOV     AX,[OVRHEAPEND]
       MOV     [OVRHEAPPTR],AX
@@19:  POP     ES
       PUSH    CX
       MOV     AX,[OVRLOADLIST]
       ES:
       MOV     [OVRLOADLIST],ES
       CALL    NEAR PTR @@12
       SUB     [OVRHEAPPTR],AX
       CALL    NEAR PTR @@18
       POP     CX
       LOOP    @@19
@@16:  MOV     AX,[OVRHEAPORG]
       MOV     [OVRHEAPPTR],AX
@@13:  POP     BX
       POP     ES
       CALL    NEAR PTR @@20
       CALL    NEAR PTR @@12
       ADD     [OVRHEAPPTR],AX
       PUSH    DS
       MOV     BX,OVRLOADLIST
@@22:  MOV     AX,[BX]
       OR      AX,AX
       JZ      @@21
       MOV     DS,AX
       MOV     BX,+20
       JMP     SHORT @@22
@@21:  MOV     [BX],ES
       ES:
       POP     DS
       RET
@@12:  ES:
       ADD     AX,+15
       MOV     CL,+4
       SHR     AX,CL
       RET
@@20:  MOV     AX,[OVRHEAPPTR]
       ES:
       ES:
       JZ      @@23
       PUSH    BP
       MOV     DX,ES
       CMP     BX,+2
       JZ      @@24
       ES:
       ES:
       MOV     [BP+2],BX
@@27:  MOV     [BP+4],AX
@@26:  MOV     BP,[BP+0]
@@24:  OR      BP,BP
       JZ      @@25
       CMP     DX,[BP+4]
       JNZ     @@26
       JMP     SHORT @@27
@@25:  POP     BP
@@23:  CALL    [????]
       JB      @@28
       ES:
       ES:
       MOV     DI,+32
       CLD
@@29:  MOV     DX,ES:[DI+2]
       MOV     AL,+234      ;  'κ'
       STOSB
       MOV     AX,DX
       STOSW
       MOV     AX,BX
       STOSW
       LOOP    @@29
       RET
@@28:  JMP     NEAR PTR ????
@@18:  PUSH    BP
       MOV     AX,[OVRHEAPPTR]
       ES:
       ES:
       JMP     SHORT @@30
@@32:  MOV     BP,[BP+0]
@@30:  OR      BP,BP
       JZ      @@31
       CMP     DX,[BP+4]
       JNZ     @@32
       MOV     [BP+4],AX
       JMP     SHORT @@32
@@31:  POP     BP
       ES:
       PUSH    DS
       PUSH    ES
       MOV     DS,DX
       MOV     ES,AX
       MOV     SI,CX
       MOV     DI,CX
       STD
       DEC     SI
       DEC     DI
       SHR     CX,1
       JNB     @@33
       MOVSB
@@33:  DEC     SI
       DEC     DI
       REPE
       MOVSW
       POP     ES
       POP     DS
       ES:
       MOV     DI,+35
       CLD
@@34:  STOSW
       ADD     DI,+3
       LOOP    @@34
       RET
@@3:   PUSH    BP
       XOR     AX,AX
       ES:
       JMP     SHORT @@35
@@37:  MOV     BP,[BP+0]
@@35:  OR      BP,BP
       JZ      @@36
       CMP     AX,[BP+4]
       JNZ     @@37
@@36:  ES:
       OR      BP,BP
       JZ      @@38
       MOV     BX,[BP+2]
       ES:
       MOV     WORD PTR [BP+2],+0
@@40:  MOV     [BP+4],ES
@@39:  MOV     BP,[BP+0]
       OR      BP,BP
       JZ      @@38
       CMP     AX,[BP+4]
       JNZ     @@39
       JMP     SHORT @@40
@@38:  POP     BP
       ES:
       MOV     DI,+32
       CLD
@@41:  MOV     DX,ES:[DI+1]
       MOV     AX,+16333
       STOSW
       MOV     AX,DX
       STOSW
       XOR     AL,AL
       STOSB
       LOOP    @@41
       RET
OVRCLEARBUF ENDP

OVRSETBUF PROC    FAR
       PUBLIC  OVRSETBUF
       PUSH    BP
       MOV     BP,SP
       XOR     AX,AX
       CMP     AX,[OVRDOSHANDLE]
       JZ      @@1
       CMP     AX,[OVRLOADLIST]
       JNZ     @@1
       MOV     AX,[HEAPORG+2]
       SUB     AX,[HEAPPTR+2]
       OR      AX,[HEAPPTR]
       JNZ     @@1
       MOV     AX,[BP+6]
       MOV     DX,[BP+8]
       MOV     CL,+4
       SHR     AX,CL
       ROR     DX,CL
       AND     DX,+61440
       OR      AX,DX
       CMP     AX,[OVRHEAPSIZE]
       JB      @@1
       ADD     AX,[OVRHEAPORG]
       JB      @@2
       MOV     ES,[PREFIXSEG]
       ES:
       JA      @@2
       MOV     [OVRHEAPEND],AX
       MOV     [HEAPORG+2],AX
       MOV     [HEAPPTR+2],AX
       XOR     AX,AX
@@3:   MOV     [OVRRESULT],AX
       POP     BP
       RETF    +4
@@1:   MOV     AX,+65535
       JMP     SHORT @@3
@@2:   MOV     AX,+65533
       JMP     SHORT @@3
OVRSETBUF ENDP

CSEG   ENDS

       END
