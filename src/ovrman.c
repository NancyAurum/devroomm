/*
  Decompiled procedures from the Borland VROOMM overlay manager
  Intended for reference purposes. Will not compile.
  Origin: OVERLAY.LIB as it was compiled into the 1992 Stronghold game,
          with the Borland C++ 3.0 compiler, using large memory model.

  Note: I tried to recover the original symbol names from the OVERLAY.LIB,
        but many are madeup by me.
 
 
                                                  --Nancy Sadkov



  Quick overfiew:
  * 16bit x86 machines were limited by 640K RAM
  * Large porition of program's code runs just once, and is not required
    majority of the time.
  * Overlays are a method to load code into a buffer inside these 640K, run it,
    and then discard, freeing memory for new code.
  * Overlay manager is responsible for doing that seamlessly to the programmer,
    with the cooperation of the compiler. I.e. instead of direct calls,
    compiler calls to the overlay manager, which uses compiler provided data
    to load and relocate the overlays.
  * Borland's VROOMM iwas part of the Borland C++ compiler's runtime,
    residing inside the OVERLAY.LIB.
    VROOMM allowed fine grained overlay loading to support object oriented code.
    Among its features was the ability to unload the calling overlay, by walking
    and patching the program's stack frames.
  * VROOMM was used in several video games, among them the 1993 game Stronghold,
    a fantasy city management game. And the 1993 Nomad game, a space exploration
    and trading game.
    In general the presence of VROOMM can be detected by the letters "FBOV",
    and the string "Runtime overlay error".
  * The execution of the program starts at C0.OBJ (see c0.c),
    which runs __OvrPrepare in this file, before initializing the standard
    C library and passing control to the main().
    Given that this code uses only the __farmalloc from the C library,
    while the rest of library can be compiled as overlays.
*/




#define SEG2PTR(seg) (void*)((uint32_t)seg*16)  
#define GETREG(reg_id) /* assembly code to retrieve the register value */ 


#define DOS_START_OF_FILE     0
#define DOS_CURRENT_POSITION  1
#define DOS_END_OF_FILE       2

#define MZ_MAGIC 0x5A4D
#define MZ_HDR_SZ 0x1C
#define MZ_PGSZ 512
typedef struct {
  uint16_t magic;     //00 Magic number MZ_MAGIC */
  uint16_t cblp;      /*02 Bytes of last page */
                      /*   If it is 4, it should be treated as 0,
                           since pre-1.10 versions of the MS linker
                           set it that way. */
  uint16_t cp;        /*04 Pages in file */
  uint16_t crlc;      /*06 Number of relocation entries */
  uint16_t cparhdr;   /*08 Size of header in paragraphs */
  uint16_t minalloc;  /*0A Minimum extra paragraphs needed */
  uint16_t maxalloc;  /*0C Maximum extra paragraphs needed */
                      /* if 0 DOS loads as high as possible, else above PSP*/
                      /* The maximum allocation is set to FFFFh by default.*/
  uint16_t ss;        /*0E Initial (relative) SS value */
  uint16_t sp;        /*10 Initial SP value */
  uint16_t csum;      /*12 Checksum (0 = no checksum) */
  uint16_t ip;        /*14 Initial IP value */
  uint16_t cs;        /*16 Initial (relative) CS value */
  uint16_t lfarlc;    /*18 File address of relocation table */
  uint16_t ovno;      /*1A Microsoft Overlay number
                           MS LINK can create single EXE containing multiple
                           overlays (up to 63) which are simply numbered in
                           sequence.
                           The 0 one is loaded by DOS
                           Each overlay within the file is essentially an
                           EXE file (structure) with it's own MZ header.*/
} PACKED mz_hdr_t;


#define FB_FB  0x4246
#define FB_OV  0x564F


typedef struct { //Borland file header
  uint32_t id[2];  // magic id FBOV_MAGIC ('FB','OV') 
  uint32_t size;   // size in bytes of this file excluding this header
  uint32_t stofs;  // offset to the boseg_t array from the start of MZ header
                   // OVERLAY.LIB names it _SEGTABLE_ or TSegMap
  int32_t  nsegs;  // number of the boseg_t entries at stofs
} PACKED bofh_t;

#define FBOV_CODE  1
#define FBOV_OVR   2
#define FBOV_DATA  4
typedef struct { //borland overlay segment entry
  uint16_t seg;    // segment (coincides with the MZ reloc table segments)
  uint16_t maxoff; // -1 - Ignored by the linker's OvrCodeReloc
  uint16_t flags;  // FBOV_CODE,FBOV_OVR,FBOV_DATA
  uint16_t minoff;
} PACKED boseg_t;


typedef struct { //Overlay header record, defined in RTL/INC/SE.ASM
  uint8_t  code[2];       //00 int 3Fh (0xCD 0x3F) overlay manager interrupt
                          //   OvrMan replaces stack returns from overlayed
                          //   functions by calls to this address
                          //   when the calling overlay gets unladed
                          //   ot it can be restored on return.
                          //   OvrMan actually walks the stack frames.
  uint16_t saveret;       //02 offest of the actual function return address
                          //   which gets returned to after OvrMan
                          //   restores it's overlay
  int32_t  fileofs;       //04 offset inside the EXE file
                          //   retative the end of bofh_t
  uint16_t codesz;        //08 size of the overlay
  uint16_t fixupsz;       //0A size in bytes of the table of pointers to data
                          //   which we must relocate after loading the overlay
                          //   In EXE the table is located just after the code.
                          //   The pointers are relative to the bufseg,
                          //   and each is uint16_t.
  uint16_t jmpcnt;        //0C number of fbov_trmp_t to update on load
                          //   the jumps are located just after this header
  uint16_t link;          //0E backlink?
  //Following are the OvrMan houskeeping fields.
  uint16_t bufseg;        //10 Buffer segment (0 = overlay is not loaded)
                          //   location of the overlay inside memory
  uint16_t retrycnt;      //12 used to track number of calls to the overlay
                          //   also holds next segment for OVRINIT
  uint16_t next;          //14 next loaded overlay
  uint16_t ems_page;      //16 location of the overlay inside expanded memory
  uint16_t ems_ofs;       //18 ofset of the function loading the overlay
  uint8_t  user[6];       //1A Runtime data about the users of this segment
                          //1A user[0] flags:
                           //  2=???, 4=out of probation, 8=loaded
                          //1B user[1] nrefs number of references to this seg
                          //   decremented in the __OvrAllocateSpace
                          //1C user[2:3] __OvrLoadList, points to next heap segh
                          //1E user[4:5] also a segment
  //fbov_trap_t dispatch[]; //switch table
} PACKED bosh_t;

#define UserNext(segh) *(uint16_t*)((segh)->user + 2)


typedef void (*pfn_t)();



//Data is broken into section groups, as it resides in memory

//_DATA (first segment of DGROUP):
uint16_t __ovrbuffer = 0x1100; //declared in OVRBUFF.OBJ
                               //user can init it by `extern _ovrbuffer = <val>`
int32_t __OvrSize; //size of the FBOV file (i.e. the data after bofh_t)




//_OVRDATA_ (first segment of OVRGROUP):
far pfn_t __OVRTRAPHANDLER = &__OvrTrapHandler;
uint8_t __OVRFILENAME[]; //first byte holds the mode
char __OVRVDISK[5] = "VDISK"; //Used by __INITEXT in OVRDETEC.OBJ
near pfn_t __CALLEXTEMSSWAP;
near pfn_t __CALLEXTEXIT;
near pfn_t __CALLEMSEXIT;
far pfn_t __OVRHOOK__; //called after overlay segment gets loaded from disk
                       //__OVRHOOK__ takes arguments in AX, BX, segh_t in ES
uint16_t __OVRBACKLINK__ = 0x000E;
char __OvrExepathBuf[];


//_STUB_ (second segment of OVRGROUP, overlay stub related data):

//the value of the `/o#xx` linker option
//default is `CD,3F` (`/o#3F` or or just `/o`)
uint8_t __OVRTRAP__[2] = {0xCD, 0x3F}; // STRONG.EXE was compiled with just /o

int32_t __OvrFileBase; //offset just after bofh_t
uint16_t __OvrRetrySize;
uint16_t __OvrMinHeapSize; //minimum overlay size in paragraphs
uint16_t __OvrTrapCount;
uint16_t __OvrProbation;
uint16_t __OvrHeapPtr; //probation area at the bottom of the heap
uint16_t __OvrCodeList; //Linked-list of all overlays
uint16_t __OvrHeapOrg;
uint16_t __OvrHeapEnd;
uint16_t __OvrDosHandle; //file handle of this .EXE, returned by DOS_OpenFile
uint16_t __OvrLoadCount;
uint16_t __OvrLoadList; //Linked-list of loaded overlays, youngest last


//_VDISKSEG_ (third segment of OVRGROUP):
uint8_t __FakeVdisk[];
uint8_t __BootHandler[];

//_OVRDATA_ (fourth segment, compiler/linker generated stuff):

//Linker generated segments table, same as the one inside FBOV
extern boseg_t __SEGTABLE__[];
extern boseg_t __SEGTABEND__[];
char *__EXENAME__ = "st.exe"; //original exename made by linkier


//_OVRTEXT_  (5th segment of _TEXT_, also has a few variables)
uint16_t __OVRDATA = 0x225d; //linker generated offset
uint16_t __DATA = 0x2410; //linker generated offset
char __OvrEnvPath[5] = "PATH=";
uint16_t __OvrParentBreakhndlrSeg = 0;


void __OvrRead(uint8_t *dst, int32_t position, int32_t sz){
  DOS_SetFilePosition(__OvrDosHandle, position, DOS_START_OF_FILE);
  for(; sz; dst += 0xFFF0) {
    uint16_t rdsz = sz>0xFFF0 ? 0xFFF0 : sz;
    dos_read_t r = DOS_ReadFile(__OvrDosHandle, dst, rdsz);
    if (r.error || r.readed < rdsz) break;
    sz -=  r.readed;
  }
  return 1;
}

dos_read_t __OvrReadHeader(uint16_t handle, void *buf, uint16_t size) {
  dos_read_t r = DOS_ReadFile(handle,buf,size);
  if (r.error) return r;
  if (r.readed < size) r.error = 1;
  return r;
}





#define X86_MOD          0xF8
#define X86_RM           0x07
#define X86_MOV_IMM16    0xB8
#define X86_PUSH         0x50

//Relocates a function address (resulting from the `&` operation).
//The assembly for this function was rather convoluted,
//and had a few useless instructions, i.e. `CX,word ptr [0xc]`.
//Was it made in haste?
void __OvrFixupFunref(uint16_t rseg, uint16_t ptr, uint16_t seg) {
  //ptr at the fixup location
  uint8_t *p = SEG2PTR(seg) + ptr;


  //segment inside which the fixup location does a function call
  uint8_t *q = SEG2PTR(rseg);

  //Ensure our fixup site loads a far pointer, like the following
  //mov reg1,seg
  //push reg1
  //mov reg2,ofs
  //push reg2
  //Check the MOD part of the MODRM byte
  //it does some opcode magic, since 0x50 is both `push AX`
  //and when masked by 0xF8 is a general `push <register>` group
  //Same with 0xB8, which usually `mov AX,<imm16>` and an opcode group
  if ((p[-1]&X86_MOD) != X86_MOV_IMM16) return; //moves segment?
  if ((p[ 2]&X86_MOD) != X86_PUSH) return;
  if ((p[-1]&X86_RM) != (p[2]&X86_RM)) return; //same RM operand?

  //now check the offset part...
  if (p[-1] != p[3]) return; //both are `move <reg>,IMM`?
  if (p[ 2] != p[6]) return; //both are `push <reg>`?

  uint16_t fofs = *(uint16_t*)&p[4]; //get raw offset of the function

  uint16_t rofs = sizeof(bosh_t);

  //go through the fbov_trap_t entries
  for ( ; fofs != *(uint16_t*)&q[rofs+2]; rofs += 5);
  
  *(uint16_t*)&p[4] = rofs; //relocate the offset part of the far function ptr
}



#define FIXUP_FUNREF 0x1

void __OvrDoFixups(bosh_t *segh, uint16_t fixupsz) {

  //fixups are stored after the segment's code
  uint16_t **pDS_SI =  SEG2PTR(segh->bufseg) + segh->codesz;

  //ES points at the start of the overlay code loaded into memory
  uint8_t **pES = SEG2PTR(segh->bufseg);


  uint16_t fixups_remain = fixupsz/2;
  do {
    uint16_t bx = *pDS_SI++; //get offset of the fixup location
    uint16_t di = *(uint16_t*)&pES[bx]; //index inside __SEGTABLE__
    uint16_t flags = di&0x3; //lower 3 bits are the relocation flags
    uint16_t rseg = __SEGTABLE__[di>>3].seg; //higher bits are the seg index
    *(uint16_t*)&pES[bx] = rseg; //relocate the seg
    if (flags&FIXUP_FUNREF) __OvrFixupFunref(rseg,bx,pES);
  } while (--fixups_remain != 0);
}


#define X86_JMP 0xEA
void __OvrUntrapJmps(bosh_t *segh) {
  uint16_t bufseg = segh->bufseg;
  fbov_trap_t *trap = (fbov_trap_t*)(segh+1);
  uint16_t remain = segh->jmpcnt;
  do {
    uint16_t ofs = trap->dst;
    fbov_trmp_t *trmp = trap++;
    trmp->code = X86_JMP;
    trmp->dst.ofs = ofs;
    trmp->dst.seg = bufseg;
  } while (--remain);
}

dos_read_t __OvrInitReadSegs() {
  dos_read_t result;
  uint16_t hptr = __OvrHeapOrg;
  __OvrLoadList = __OvrCodeList;
  bosh_t *cur = SEG2PTR(__OvrCodeList);
  bosh_t *prev = cur;
  
  // See what segment we can immediately pre-load
  while (1) {
    if (!cur.retrycnt) break; //end of the preloaded segs
    bosh_t *next = SEG2PTR(cur.retrycnt);
    uint16_t segsz = (next->fileofs - cur->fileofs)/16
    if (hptr + segsz > __OvrHeapEnd) break; //Heap overflow!!
    cur->bufseg = hptr;
    hptr += segsz;
    UserNext(cur) = next;
    prev = cur;
    cur = next;
  }
  UserNext(prev) = 0; //terminate the list

  uint16_t load_size = __OvrHeapPtr - __OvrHeapOrg;
  if (load_size) {
    bosh_t *seg = SEG2PTR(__OvrLoadList);
    result = __OvrRead(__OvrHeapOrg, seg->fileofs, load_size);
    if (result.error) return result;
    seg = SEG2PTR(__OvrLoadList);

    do { //relocate segments
      if (seg->fixupsz) __OvrDoFixups(seg);
      if (seg->jmpcnt) __OvrUntrapJmps(seg);
      bosh_t *bseg = SEG2PTR(seg->bufseg-1);
      bseg->link = seg;
      __OVRHOOK__(seg,0xFFFF,0xFFFF); //tell debugger we inited a segment
      seg = UserNext(seg);
    } while (seg);
  }
  result.error = 0;
  return result;
}


//overlay size in paragraphs
uint16_t __OvrCalcSize(bosh_t *segh) {
  //+17 to accomodate for alignment
  return (segh->fixupsz+15)/16 + (segh.codesz+17)/16;
}


uint16_t __OvrCalcCodeSz(bosh_t *segh) { //just the code size
  //+17 to accomodate for alignment
  return (segh->codesz + 0x17) / 16;
}

void __OvrAddToLoadList(bosh_t *segh) {
  // Allocate in probation area (lower heap addresses)
  __OvrHeapPtr += __OvrCalcCodeSz(segh);

  // Add it to the linked list as the last tail element
  bosh_t *segh2 = __OvrLoadList;
  for (; UserNext(segh2); segh2 = UserNext(segh2));
  UserNext(segh2) = segh;
  UserNext(segh) = 0;
  return;
}


#define BOSH_UNLISTED 0xFF

int __InitModules() {
  //save pointer to the parent's break handler's segment
  ovr_parent_breakhndlr_seg = &_psp->breakhndlr.seg;

  uint16_t min_size = 0;
  boseg_t *pse = __SEGTABLE__;
  do {
    if ((pse->flags & FBOV_OVR) && pse->maxoff) {
      bosh_t *segh = SEG2PTR(pse->seg);
      __OvrCodeList = pse->seg;
      if (segh->user[0] == BOSH_UNLISTED) __OvrCodeList = 0; //not loaded
      else {
        // Set overlay loader routine and relocate the overlay offset
        segh->ems_ofs = &__ReadOvrDisk;
        segh->fileofs += __OvrFileBase;
        uint16_t sz = __OvrCalcSize(segh); //calculate the ovelray size
        if (min_size < sz) min_size = sz;
      }
    }
  } while (++pse < __SEGTABEND__);
  __OvrMinHeapSize = min_size + 2; //adding 2 accomodates for alignment
  return 0;
}


//Installs handler for the VROOMM interrupts (usually 0x3f)
//if caleld again, uninstalls it.
void __OvrResetInterruptVector() {
  undefined2 uVar1;
  dos_open_t dVar2;
  far_t fVar3;
  uint16_t in_stack_00000000;
  undefined2 in_stack_00000002;
  
  far_t prev_handler = DOS_GetInterruptHandler(__OVRTRAP__[1]);
  DOS_SetInterruptHandler(__OVRTRAP__[1],__OVRTRAPHANDLER);
  __OVRTRAPHANDLER = prev_handler; //save previous handler to be restored

  if (__OvrDosHandle) {
    DOS_CloseFile(__OvrDosHandle);
    __OvrDosHandle = 0;
  } else {
    __OvrDosHandle = DOS_OpenFile(__OVRFILENAME[0],__OvrExepathBuf).handle;
  }
}

dos_open_t OvrDoOpenFile(char *path, char *exename) {
  //Note how path always points inside __OvrExepathBuf
  if (exename) { // If name not NULL, append it to path (else path has the name)
    int count = 12; //DOS filename: 8+1+3 = 12
    do {
      char ch = *exename++;
      *path++ = ch;
      if (!ch) goto break;
    } while (--count);
    *path = '\0';
  }
  return DOS_OpenFile(__OVRFILENAME, __OvrExepathBuf);
}

dos_open_t __OvrExeOpenEnvPath(char *exename) {
  dos_open_t result;
  char *penv = SEG2PTR(_env.seg);
  // Find PATH= variable
  do { // Process all environment entries
    char *path = __OvrEnvPath;
    int remain = sizeof(__OvrEnvPath);
    do {
      if (*path++ != *penv++) break;
    } while (penv[-1] && --remain);
    if (!remain) goto try_path_items;
    if (penv[-1]) while (*penv++); // Skip the rest of the variable
  } while (*penv++); // Empty variable means end

  if (!remain) goto return_error;

  while (*penv) {
    char *path = __OvrExepathBuf;
    char c = '\0';
    while( true ) {
      pd = penv + 1;
      char nc = *penv;
      if (!nc) break;
      penv = pd;
      if (nc == ';') break;
      *path++ = nc;
      c = nc;
    }
    if (c != ':' && c != '\\') *path++ = '\\'; //normalize the path
    result = OvrDoOpenFile(path, exename);
    if (!result.error) return result;
  }

return_error:
  result.error = 1;
  return result;
}


dos_open_t __OvrExeOpenFilename(char *exename) {
  return OvrDoOpenFile(__OvrExepathBuf, exename);
}

//Use the command line after the environment to open the exe
dos_open_t __OvrExeOpenEnv(char *exename) {
  // Only DOS v3 and up add the command line after the environment vars
  if (DOS_GetDOSVersion().major < 3) {
    dos_open_t result;
    result.error = 1;
    return result;
  }

  //DOS file open modes
  __OVRFILENAME[0] = 0x20;

  // The command line string is located after the environment variables.
  // Environment variables end with 00 00.  
  char *penv = SEG2PTR(_env.seg);
  // The origianl OVERLAY.LIB code assumes that ENV has at least one variable
  do { // Skip all environment entries
    while (*penv++); // Skip environment variable
  } while (*penv++); // Empty variable means end

  char *pcmd = penv + 2; //skip last "\0" and the command line size

  // Find the filename part
  char *path = __OvrExepathBuf;
  char *pfb = __OvrExepathBuf;
  do {
    char ch = *pcmd++;
    *pfb++ = ch;
    if (ch == '\\') path = pfb; //get the last path part
  } while (ch);

  return OvrDoOpenFile(path,exename);
}

#define INIT_OKAY    0
#define INIT_BADEXE -1
#define INIT_NOEXE  -2

int16_t __OVRINIT(char *exename, uint16_t bufend, uint16_t bufbeg) {
  if (!__OVRTRAP__[0])
    return INIT_OKAY; //no overlays compiled-in -- nothing to init!

  dos_open_t opnrslt = OvrExeOpenEnv(exename);
  if (opnrslt.error) {
    if (!exename) exename = __EXENAME__;
    opnrslt = OvrExeOpenFilename(exename);
    if (opnrslt.error) { 
      opnrslt = OvrExeOpenEnvPath(exename);
      if (opnrslt.error) return INIT_NOEXE;
    }
  }

  __OvrDosHandle = opnrslt.handle;

  //the compiled code used a common buffer for the read operations.
  //We assume the compiler is smart enough to reuse the buffers.
  mz_hdr_t mzh;
  dos_read_t rdrslt = __OvrReadHeader(__OvrDosHandle,&mzh,0x14);
   if (rdrslt.error || mzh.magic != MZ_MAGIC) {   // Really MZ?
bad_file:
     DOS_CloseFile(__OvrDosHandle);
     return INIT_BADEXE;
   }

  //Borland overlays start at the end of the normal MZ data
  uint16_t full_pages = mzh.cblp != 0 ? mzh.cp - 1 : mzh.cp;
  int32_t exeofs = (int32_t)full_pages*MZ_PGSZ + mzh.cblp;
  exeofs = (exeofs+15)/16; //align to paragraph

  while (1) { // go through FB__ sections till FBOV
    DOS_SetFilePosition(__OvrDosHandle,exeofs,DOS_START_OF_FILE);
    bofh_t bofh;
    rdrslt = __OvrReadHeader(__OvrDosHandle,&bofh,16);
    if (rdrslt.error || bofh.id[0] != FB_FB) goto bad_file;

    exeofs += 16; //header is not part of the section size.

    if (bofh.id[1] == FB_OV) break; //overlay file?
    exeofs += bofh.size; //skip non-OV files
  }

  __OvrFileBase = exeofs; //start of the OV file, minus its header
  __OvrSize = bofh.size;

  //required for __OvrResetInterruptVector to init us properly
  DOS_CloseFile(__OvrDosHandle);
  __OvrDosHandle = 0
  
  __OvrResetInterruptVector();

  __OvrHeapPtr = __OvrHeapOrg = bufbeg + 1;
  __OvrHeapEnd = bufend;
  __InitModules();
  uint16_t total_memory = __OvrHeapEnd - __OvrHeapOrg;
  if (__OvrMinHeapSize > total_memory) goto bad_file;
  __OvrRetrySize = total_memory >> 2;

  rdrslt = __OvrInitReadSegs();
  if (rdrslt.error) return rdrslt.readed; //readed holds the error code

  return INIT_OKAY;
}

void __OVREXIT() {
  if (__OvrDosHandle != 0) __OvrResetInterruptVector();
  __CALLEMSEXIT(GETREG(CS));
  __CALLEXTEXIT(GETREG(CS));
  return;
}


void __OvrPrepare() {
  __InitModules();
  int32_t bufsz = __ovrbuffer <= __OvrMinHeapSize
                  ? __OvrMinHeapSize*2
                  : __ovrbuffer;
  bufsz += 1; //enough size for a paragraph alignment

  far_t buf = _farmalloc(bufsz*16); // _farmalloc can allocate more than 64K
  if (!buf) _abort();

  buf.seg += 1; //ensure it is aligned.

  if(__OVRINIT(0, buf.seg+bufsz, buf.seg) != INIT_OKAY) abort();
}
