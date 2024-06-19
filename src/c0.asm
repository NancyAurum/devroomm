dPtrSize equ 4
PSPHigh equ 00002h
PSPEnv equ 0002ch
MINSTACK equ 128 ; minimal stack size in words


;  Segment and Group declarations
;  PARA - aligned to 16bytes
_TEXT           SEGMENT BYTE PUBLIC 'CODE'
                ENDS
_FARDATA        SEGMENT PARA PUBLIC 'FAR_DATA'
                ENDS
_FARBSS         SEGMENT PARA PUBLIC 'FAR_BSS'
                ENDS


_OVERLAY_       SEGMENT PARA PUBLIC 'OVRINFO'
                ENDS
;;segment for overlay stubs
_1STUB_         SEGMENT PARA PUBLIC 'STUBSEG'
                ENDS


;;general data
_DATA           SEGMENT PARA PUBLIC 'DATA'
                ENDS

;;table of init procedures
_INIT_          SEGMENT WORD PUBLIC 'INITDATA'
InitStart       label byte
                ENDS
_INITEND_       SEGMENT BYTE PUBLIC 'INITDATA'
InitEnd         label byte
                ENDS

;;table of exit procedures
_EXIT_          SEGMENT WORD PUBLIC 'EXITDATA'
ExitStart       label byte
                ENDS
_EXITEND_       SEGMENT BYTE PUBLIC 'EXITDATA'
ExitEnd         label byte
                ENDS

//floating point segments
_CVTSEG         SEGMENT WORD PUBLIC 'DATA'
                ENDS
_SCNSEG         SEGMENT WORD PUBLIC 'DATA'
                ENDS




        ASSUME  CS:_TEXT, DS:DGROUP

;       External References

extrn       _main:DIST
extrn       _exit:DIST
extrn       __exit:DIST
extrn       __nfile:word
extrn       __setupio:near          ;required!
extrn       __stklen:word


;[]------------------------------------------------------------[]
;|      Start Up Data Area                                      |
;|                                                              |
;|      WARNING         Do not move any variables in the data   |
;|                      segment unless you're absolutely sure   |
;|                      that it does not matter.                |
;[]------------------------------------------------------------[]

_DATA           SEGMENT

;       Magic symbol used by the debug info to locate the data segment
                public DATASEG@
DATASEG@        label   byte

;       The CopyRight string must NOT be moved or changed without
;       changing the null pointer check logic

CopyRight       db      4 dup(0)
                db      'Borland C++ - Copyright 1991 Borland Intl.',0
lgth_CopyRight  equ     $ - CopyRight

ZeroDivMSG      db      'Divide error', 13, 10
lgth_ZeroDivMSG equ     $ - ZeroDivMSG

abortMSG        db      'Abnormal program termination', 13, 10
lgth_abortMSG   equ     $ - abortMSG

;                       Interrupt vector save areas
;       
;       Interrupt vectors 0,4,5 & 6 are saved at startup and then restored
;       when the program terminates.  The signal/raise functions might
;       steal these vectors during execution.
;
;       Note: These vectors save area must not be altered 
;             without changing the save/restore logic.
;
PubSym@         _Int0Vector     <dd     0>,             __CDECL__
PubSym@         _Int4Vector     <dd     0>,             __CDECL__
PubSym@         _Int5Vector     <dd     0>,             __CDECL__
PubSym@         _Int6Vector     <dd     0>,             __CDECL__
;
;                       Miscellaneous variables
;       
PubSym@         _C0argc,        <dw     0>,             __CDECL__
dPtrPub@        _C0argv,        0,                      __CDECL__
dPtrPub@        _C0environ,     0,                      __CDECL__
PubSym@         _envLng,        <dw     0>,             __CDECL__
PubSym@         _envseg,        <dw     0>,             __CDECL__
PubSym@         _envSize,       <dw     0>,             __CDECL__
PubSym@         _psp,           <dw     0>,             __CDECL__
PubSym@         _version,       <label word>,           __CDECL__
PubSym@         _osversion,     <label word>,           __CDECL__
PubSym@         _osmajor,       <db     0>,             __CDECL__
PubSym@         _osminor,       <db     0>,             __CDECL__
PubSym@         errno,          <dw     0>,             __CDECL__
PubSym@         _StartTime,     <dw   0,0>,             __CDECL__


PubSym@         __brklvl,       <dw   DGROUP:edata@>,   __CDECL__
PubSym@         _heapbase,      <dd   0>,       __CDECL__
PubSym@         _brklvl,        <dd   0>,       __CDECL__
PubSym@         _heaptop,       <dd   0>,       __CDECL__

_DATA           ENDS





;;floating point to int
_CVTSEG         SEGMENT
PubSym@         _RealCvtVector, <label  word>,  __CDECL__
                ENDS

;;floating point from string
_SCNSEG         SEGMENT
PubSym@         _ScanTodVector,  <label word>,  __CDECL__
                ENDS

;;zero-initialized data (aka block started by symbol)
_BSS            SEGMENT
bdata@          label   byte
                ENDS

;;end of zero-inited data
_BSSEND         SEGMENT
edata@          label   byte
                ENDS



_STACK          SEGMENT
                db      128 dup(?)               ;minimum stack size
                ENDS





; At the start, DS and ES both point to the segment prefix.
; SS points to the stack segment except in TINY model where
; SS is equal to CS
_TEXT  SEGMENT
STARTX PROC NEAR
  ; Save general information, such as:
  ; DGROUP segment address
  ; DOS version number
  ; Program Segment Prefix address
  ; Environment address
  ; Top of far heap
  mov dx, DGROUP ; DX = GROUP Segment address
  mov cs:DGROUP@@, dx ;  __BOSS__
  mov ah, 30h
  int 21h ; get DOS version number
  mov bp, ds:[PSPHigh]; BP = Highest Memory Segment Addr
  mov bx, ds:[PSPEnv] ; BX = Environment Segment address
  mov ds, dx
  mov _version@, ax   ; Keep major and minor version number
  mov _psp@, es  ; Keep Program Segment Prefix address
  mov _envseg@, bx ; Keep Environment Segment address
  mov word ptr _heaptop@ + 2, bp
  ; Save several vectors and install default divide by zero handler.
  call SaveVectors


  ; Count the number of environment variables and compute the size.
  ; Each variable is ended by a 0 and a zero-length variable stops
  ; the environment. The environment can NOT be greater than 32k.
  les di, dword ptr _envLng@ ;;load dword pointer from the location of _envLng@
  mov ax, di ;since _envLng@ starts as 0, ax becomes 0
  mov bx, ax ;same way set bx to 0
  mov cx, 07FFFh ; Environment cannot be > 32 Kbytes
  cld
@@EnvLoop:
  repnz   scasb
  jcxz InitFailed ; Bad environment !!!

  inc bx ; BX = Number of environment variables
  cmp es:[di], al ;;di points just past the last 0
  jne @@EnvLoop ; a zero length variable terminates the list
  or ch, 10000000b
  neg cx
  mov _envLng@, cx ; Save Environment size
  mov cx, dPtrSize / 2
  shl bx, cl
  add bx, dPtrSize * 4
  and bx, not ((dPtrSize * 4) - 1)
  mov _envSize@, bx   ; Save Environment Variables Nb.



  ; Determine the amount of memory that we need to keep
  mov dx, ss
  sub bp, dx ; BP = remaining size in paragraphs
  mov di, seg __stklen
  mov es, di
  mov di, es:__stklen ; DI = Requested stack size

  ; Make sure that the requested stack size is at least MINSTACK words.
  cmp di, 2*MINSTACK  ; requested stack big enough ?
  jae AskedStackOK
  mov di, 2*MINSTACK  ; no --> use minimal value
  mov es:__stklen, di ; override requested stack size

AskedStackOK label   near
  mov cl, 4
  shr di, cl ; $$$ Do not destroy CL $$$
  inc di  ; DI = DS size in paragraphs
  cmp bp, di ; if (bp > di) goto ExcessOfMemory
  jnb ExcessOfMemory ; Much more available than needed

  ; All initialization errors arrive here
InitFailed  label near
  jmp near ptr _abort

  ; Return to DOS the amount of memory in excess
  ; Set far heap base and pointer
ExcessOfMemory label near
  mov bx, di
  add bx, dx
  mov word ptr _heapbase@ + 2, bx
  mov word ptr _brklvl@ + 2, bx
  mov ax, _psp@
  sub bx, ax ; BX = Number of paragraphs to keep
  mov es, ax ; ES = Program Segment Prefix address
  mov ah, 04Ah
  push di ; preserve DI
  int 021h  ; this call clobbers SI,DI,BP !!!!!!
  pop di ; restore  DI

  shl di, cl  ; $$$ CX is still equal to 4 $$$

  cli ; req'd for pre-1983 88/86s
  mov ss, dx  ; Set the program stack
  mov sp, di
  sti

  mov ax, seg __stklen
  mov es, ax
  mov es:__stklen, di ; If separate stack segment, save size


  ; Reset uninitialized data area
  xor ax, ax
  mov es, cs:DGROUP@@
  mov di, offset DGROUP: bdata@
  mov cx, offset DGROUP: edata@
  sub cx, di
  cld
  rep stosb

  ; If default number of file handles have changed then tell DOS
  cmp __nfile, 20
  jbe @@NoChange

  cmp _osmajor@, 3 ; Check for >= DOS 3.3
  jb @@NoChange
  ja @@DoChange
  cmp _osminor@, 1Eh
  jb @@NoChange
@@DoChange:
  mov ax, 5801h ; Set last fit allocation
  mov bx, 2
  int 21h
  jc @@BadInit

  mov ah, 67h; Expand handle table
  mov bx, __nfile
  int 21h
  jc @@BadInit

  mov ah, 48h ; Allocate 16 bytes to find new
  mov bx, 1 ;   top of memory address
  int 21h
  jc @@BadInit
  inc ax ; Adjust address to point after block
  mov word ptr _heaptop@ + 2, ax

  dec ax ; Change back and release block
  mov es, ax
  mov ah, 49h
  int 21h
  jc @@BadInit

  mov ax, 5801h ; Set first fit allocation
  mov bx, 0
  int 21h
  jnc @@NoChange

@@BadInit:
  jmp near ptr _abort

@@NoChange:
  ; Prepare main arguments
  mov ah, 0
  int 1ah ; get current BIOS time in ticks
  mov word ptr _StartTime@,dx ; save it for clock() fn
  mov word ptr _StartTime@+2,cx
  or al,al ; was midnight flag set?
  jz @@NotMidnight
  mov ax,40h ; set BIOS midnight flag
  mov es,ax ; at 40:70
  mov bx,70h
  mov byte ptr es:[bx],1
 
@@NotMidnight:
  xor bp,bp ; set BP to 0 for overlay mgr

  mov es, cs:DGROUP@@
  mov si,offset DGROUP:InitStart ;si = start of table
  mov di,offset DGROUP:InitEnd   ;di = end of table
  call StartExit

  ; ExitCode = main(argc,argv,envp);
  push word ptr __C0environ+2
  push word ptr __C0environ
  push word ptr __C0argv+2
  push word ptr __C0argv
  push __C0argc
  call _main

  ; Flush and close streams and files
  push ax
  call _exit
STARTX   ENDP


;------------------------------------------------------------------
;  Loop through a startup/exit (SE) table, 
;  calling functions in order of priority.
;  ES:SI is assumed to point to the beginning of the SE table
;  ES:DI is assumed to point to the end of the SE table
;  First 64 priorities are reserved by Borland
;------------------------------------------------------------------
PNEAR           EQU     0
PFAR            EQU     1
NOTUSED         EQU     0ffh

SE              STRUC
calltype        db      ?                       ; 0=near,1=far,ff=not used
priority        db      ?                       ; 0=highest,ff=lowest
addrlow         dw      ?
addrhigh        dw      ?
SE              ENDS

StartExit       proc near
@@Start:        cmp     si,offset DGROUP:InitStart      ; startup or exit?
                je      @@StartLow              ; it's startup
                xor     ah,ah                   ; start with high priority
                jmp     short @@SaveEnd
@@StartLow:     mov     ah,0ffh                 ;start with lowest priority
@@SaveEnd:      mov     dx,di                   ;set sentinel to end of table
                mov     bx,si                   ;bx = start of table

@@TopOfTable:   cmp     bx,di                   ;and the end of the table?
                je      @@EndOfTable            ;yes, exit the loop
                cmp     es:[bx.calltype],NOTUSED;check the call type
                je      @@Next
                cmp     si,offset DGROUP:InitStart      ; startup or exit?
                je      @@CompareHigh           ; it's startup
                cmp     ah,es:[bx.priority]     ; it's exit
                jmp     short @@CheckPrior      ; if priority too low, skip
@@CompareHigh:  cmp     es:[bx.priority],ah     ;check the priority
@@CheckPrior:   ja      @@Next                  ;too high?  skip
                mov     ah,es:[bx.priority]     ;keep priority
                mov     dx,bx                   ;keep index in dx
@@Next:         add     bx,SIZE SE              ;bx = next item in table
                jmp     @@TopOfTable

@@EndOfTable:   cmp     dx,di                   ;did we exhaust the table?
                je      @@Done                  ;yes, quit
                mov     bx,dx                   ;bx = highest priority item
                cmp     es:[bx.calltype],PNEAR  ;is it near or far?
                mov     es:[bx.calltype],NOTUSED;wipe the call type
                push    es                      ;save es
                je      @@NearCall

@@FarCall:      call    DWORD PTR es:[bx.addrlow]
                pop     es                      ;restore es
                jmp     short @@Start

@@NearCall:     call    WORD PTR es:[bx.addrlow]
                pop     es                      ;restore es
                jmp     short @@Start

@@Done:         ret
                endp


                public __MMODEL
__MMODEL        dw      0C004h

_TEXT           ENDS