//  Origin: C0.OBJ as it was compiled into the 1992 Stronghold game,
//          with the Borland C++ 3.0 compiler, using large memory model.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#define TINY    0
#define SMALL   1
#define MEDIUM  2
#define COMPACT 3
#define LARGE   4
#define HUGE    5

#define FCODE 0x8000  /*far code*/
#define FDATA 0x4000  /*far data*/

//offsets into PSP
#define PSPHigh  0x0002
#define PSPEnv   0x002c

#define MINSTACK 128  // minimal stack size in words


#define MAX_ENV_SZ 0x8000


typedef struct { /* offset,segment pair */
  uint16_t ofs;
  uint16_t seg;
} PACKED far_t;

typedef struct {
  uint16_t cpm_call0;            // CP/M-80-like termination (int 20h)
  uint16_t hiseg;                // Top of the free memory.
                                 // hiseg-SS gives the amount of memory
                                 // the program can use for stack and malloc
  uint8_t  unused1;
  uint8_t  cpm_call5[5];
  far_t    termhndlr;            // parent's terminate handler
  far_t    breakhndlr;           // parent's crtl/break handler
  far_t    crithndlr;            // parent's critical error handler
  uint16_t parent_psp;
  uint8_t  jft[20];              // job file table entries (FF = available)
  uint16_t envseg                // segment with environment vars
  struct   far_t int21_ss_sp;
  uint16_t jft_size;             // DOS 3+ number of entries in JFT (default 20) */
  struct   far_t jft_pointer;    // DOS 3+ pointer to JFT (default PSP:0018h)
  struct   far_t previous_psp;
  uint8_t  call_int21[3];
  uint8_t  unused2;
  uint8_t  dosver[2];            // DOS 5+ version to return on INT 21/AH=30h
  uint16_t win3x_pdb_next_psp;
  uint16_t win3x_pdb_partition;
  uint16_t win3x_pdb_next_pdb;
  uint8_t  win3x_winoldapp;
  uint8_t  unused3[3];
  uint16_t win3x_pdb_entry_stack;
  uint8_t  unused4[2];
  uint8_t  unix_call50[3];
  uint8_t  unused5[2];
  uint8_t  efcb1[7];             // used to extend fcb1
  uint8_t  fcb1[16];             // 1st CP/M-style FCB
  uint8_t  fcb2[16];             // 2nd CP/M-style FCB
  uint8_t  extra[4];             // overflow from FCBs
  uint8_t  cmdlen;               // length of command line parameters
  uint8_t  cmdline[127];         // command line parameters
} PACKED dos_psp_t;

typedef struct { // DOS Version
  uint16_t major;
  uint16_t minor;
} PACKED dosver_t;

#define SE_NEAR 0
#define SE_FAR  1
#define SE_OVER 0xFF
typedef struct { // Borland Startup Entry
  uint8_t calltype;  //SE_NEAR,SE_FAR,SE_OVER
  uint8_t priority;  //lower priority entires run first on startup
                     //and last on shutdown
  far_t proc;        //procedure
} PACKED brl_se_t;



//External variables the c0.obj references
extern uint16_t _nfile;  // requrested number of file handles
extern near _setupio() // required to be linked near

// The default _stklen value 0x1000 is defined in CLIB/STKLEN.C
// STRONG.EXE's _stklen is 0x1400
// meaning it the devlopers modified it
// Borland C++ 3.0 allowed statements like:
//   extern unsigned _stklen = 54321U;
// which redefined the value of an external variable.
// kind of an compile-time inter-obj configuration API.
uint16_t _stklen = 0x1400;


brl_se_t InitStart[] = { //start of the _INIT_ segment
  //these are the startup entries present in STRONG.EXE
  {SE_FAR , 1,&_OvrPrepare}, //init overlay manager
  {SE_NEAR, 2,&_setupio},     //init I/O
  {SE_NEAR,16,&_c0crtinit},   //init textmode video
  {SE_NEAR,16,&_setargv},     //init argv and envp vectors
};
extern brl_se_t InitEnd[];   //end of the _INIT_ segment

extern brl_se_t ExitStart[] = {
  {SE_FAR , 1,&__OVREXIT}     //EMS/EXT exit
};
extern brl_se_t ExitEnd[];

//Global variables the c0.obj provides

uint16_t DGROUP;


int16_t _C0argc;
char **_C0argv;
char **_C0environ;
far_t _env; //offset: EnvLng - size in bytes of the environment block
            //segment: EnvSeg: environment segment
uint16_t _envSize;   //number of environment variables
dos_psp_t *psp;
dosver_t _osversion; //symbol _version also points to it
int16_t errno;
uint32_t _StartTime; //used clock() to get time since startp

//uint16_t __brklvl = &__DGROUP_SEGMENT__::edata;
far_t _heapbase;
far_t _brklvl;
far_t _heaptop;


uint16_t __MMODEL = FCODE|FDATA|LARGE;


void SaveVectors() {
    //saving interrupt vectors
}

void _abort() {
  exit(EXIT_FAILURE);
}


union REGS regs;

dosver_t DOS_GetVersion() {
  regs.ah = 0x30;
  int86(0x21);
  dosver_t v;
  v.major = regs.al;
  v.minor = regs.ah;
  return v;
}

void DOS_ReallocateMemory(uint16_t segment_to_resize, uint16_t paragraphs) {
  regs.ah = 0x4A;
  regs.bx = paragraphs;
  regs.es = segment_to_resize;
  int86(0x21);
  if (regs.cflag) {
      _abort();
  }
}

uint16_t DOS_AllocateMemory(uint16_t paragraphs) {
  regs.ah = 0x48;
  regs.bx = paragraphs;
  int86(0x21);
  if (regs.cflag) {
      _abort();
  }
  return regs.ax;
}

void DOS_ReleaseMemory(uint16_t segment) {
  regs.ah = 0x49;
  regs.es = segment;
  int86(0x21);
  if (regs.cf) {
      _abort();
  }
}

#define DOS_ALLOC_FIRST_FIT  0
#define DOS_ALLOC_BEST_FIT   1
#define DOS_ALLOC_LAST_FIT   2


void DOS_SetAllocationStrategy(uint16_t strategy) {
  regs.ah = 0x58;
  regs.al = 0x01; //set
  regs.bx = strategy;
  int86(0x21);
  if (regs.cf) {
    _abort();
  }
}

//Weird call to increase amount of file handles needs to allocate memory if >2
void DOS_SetHandleCount(uint16_t handles) {
  regs.ah = 0x67;
  regs.bx = handles;
  int86(0x21);
  if (regs.cf) {
    _abort();
  }
}

//The RTC is capable of multiple frequencies.
//The base frequency is pre-programmed at 32.768 kHz. 
uint32_t BIOS_ReadRTC(uint8_t *is_midnight) {
  regs.ah = 0x00; // Read RTC 
  //https://wiki.osdev.org/RTC
  //https://en.wikipedia.org/wiki/BIOS_interrupt_call#Interrupt_table
  int86(0x1A);
  if (is_midnight) *is_midnight = regs.al;
  return ((uint32_t)regs.cx << 16) | regs.dx;
}

//Bios Data Area Clock Roll-over flag
uint32_t BDA_SetMidnightFlag(uint8_t flag) {
  *(uint8_t*)0x470 = flag; //40:70
}

void set_program_stack(uint16_t new_ss, uint16_t new_sp) {
  _asm {
    cli // req'd for pre-1983 88/86s
    mov ss, new_ss
    mov sp, new_sp
    sti
  }
}

void STARTX() __attribute__((naked)) {  
  // Save general information
  DGROUP = __DGROUP_SEGMENT__; //save the relocated address of DGROUP
                               //so that the MZ relocation table wont get
                               //overbloatedd
  _osversion = DOS_GetVersion();
  _psp = DS; //DS initially points to the psp_t, which is just above start()
  _heaptop.seg = _psp->hiseg; //get top of free memory

  _env.ofs = 0;
  _env.seg = _psp->envseg;

  // Save several interrupt handlers and install default divide by zero handler.
  SaveVectors();
  
  // Count the number of environment variables and compute the size.
  // Each variable is ended by a 0 and a zero-length variable stops
  // the environment.
  char *penv = (char*)_env;
  char *max_env = penv+MAX_ENV_SZ;
  int ax = 0;
  _envSize = 0;
  Env.ofs = 0;
  do {
    while (*penv++ && penv < max_env);
    if (penv == max_env)
      _abort(); // The environment can NOT be greater than 32k.
    ++_envSize;
  } while (*penv); //empty variable terminates environment list
  _envSize = (_envSize+4)*4 & 0xFFF0; //align to paragraphs
  _env = (far_t)penv; //store pointer to the end of the environment


  // Determine the amount of memory that we need to keep
  uint16_t available_memory = _heaptop.seg - _SS; //in segments
  far_t pstklen = &_stklen;
  es = pstklen.seg;
  if (_stklen < 2*MINSTACK) // requested stack big enough ?
    _stklen = 2*MINSTACK;

  uint16_t stklensegs = _stklen/16 + 1; //convert to segments
  if (available_memory < stklensegs) _abort();

  // Return to DOS the amount of memory in excess
  // Set far heap base and pointer
  uint16_t base = stklensegs + _heaptop.seg;
  _heapbase.seg = base;
  _brklvl.seg = base;
  int psp_seg = ((far_t)_psp).seg; //PSP is the start of memory DOS EXE loader
                                   //allocated for us
  DOS_ReallocateMemory(psp_seg, base-psp_seg);
  bx = base-psp_seg;

  // Reset the uninitialized data area (BSS) to 0
  for (uint16_t *p = bdata; p < edata; p++) *p = 0;

  // If default number of file handles have changed then tell DOS
  if (_nfile > 20) {
    if (_osversion.major > 3
        || (_osversion.major == 3 && _osversion.minor >= 30)) {
      DOS_SetAllocationStrategy(DOS_ALLOC_LAST_FIT);
      DOS_SetHandleCount(_nfile);
      // Allocate 16 bytes to find the new top of memory address
      _heaptop.seg = DOS_AllocateMemory(1) + 1;
      DOS_ReleaseMemory(_heaptop.seg - 1);
      DOS_SetAllocationStrategy(DOS_ALLOC_FIRST_FIT);
    }
  }
  
  uint8_t is_midnight;
  _StartTime = BIOS_ReadRTC(&is_midnight);
  BDA_SetMidnightFlag(is_midnight);

  StartExit(InitStart, InitEnd);

  int exit_code = main(_C0argc, _C0argv, _C0environ);

  exit(exit_code); // Flush I/O buffers, close streams and files
}


void near StartExit(borland_SE* se_start, borland_SE* se_end) {
  int doing_init = se_start == InitStart;
  while (1) {
    borland_SE *cur_se = se_end;
    uint8_t cur_priority = doing_init ? 0xff : 0;

    for (borland_SE *se = se_start; se != se_end; se++) {
      if (se->calltype == SE_OVER) continue;
      bool better = doing_init ? (se->priority <= cur_priority)
                               : (se->priority >= cur_priority);
      if (better) {
        cur_priority = se->priority;
        cur_se = se;
      }
    }

    // till we process all entries in order of priority
    if (cur_se == se_end) break;

    uint8_t calltype = cur_se->calltype;
    cur_se->calltype = SE_OVER; // mark as processed

    if (calltype == SE_NEAR) NEAR_FN(cur_se->proc)();
    else FAR_FN(cur_se->proc)();
  }
  return;
}
