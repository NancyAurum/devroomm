/*
  MZap
  A command line utility to process the DOS MZ executbables.
  Doesn't work with PE EXEs.
  
  If you just want to prepare your Borland FBOV .exe for Ghidra, then do,

    ./mzap.exe fbov.exe -mb ./out/unfbov.exe

  That produces unfbov.exe, which has the FBOV section merged into the
  normal executable, which should work with both Ghidra and IDA Pro.
  With some tricks it should be possible to run and debug it too.

  If that method fails, then try

    ./mznfo.exe fbov.exe -r 0x10000 ./out/unfbov.exe

  that will relocate everything in advance, clearing the relocation table,
  so you will have to create all these segments manually by doing

    ./mzap.exe fbov.exe -lb
    
  and then editing the ghidra memory map, splitting everything according
  this -lb output.

  You will also need the map produced by

     ./mzap.exe fbov.exe -lbh

  that one is for the overlays.
  
  Note: IDA Pro can't handle such MZap relocated exes, so use Ghidra.
  
                                         -Nancy Sadkov

*/

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "nap.h"

#define NG_UTIL_IMPLEMENTATION
#include "util.h"


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

//ensures compiler wont add gaps between the fields.
#define PACKED __attribute__((packed)) 

typedef struct { /* offset,segment pair */
  uint16_t ofs;
  uint16_t seg;
} PACKED far_t;

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

typedef struct {
  uint16_t seg;
  uint16_t size;
  int refs;       //number of references;
  int trefs;      //number of references by the targets of relocations
  int ofs;        //flat offset
} mz_uniq_seg_t;

typedef struct { //borland MZ heaader extension
  uint16_t unk0;      /* always 0x0001 ? */
  uint8_t  id;        /* 0xFB */
  uint8_t  version;   /* major nybl, minor is low nybl */
  uint16_t unk1;      /* euther 0x726A (v3.0+) or 0x6A72 (prior to v3.0)*/
  uint8_t  unk2[34];  /* always 0 ? */
} PACKED borland_mz_ext_t;



typedef struct { // Entry in the Borlands's startup's table of init functions
  uint8_t calltype; // 0=near,1=far,ff=not used
  uint8_t priority; // 0=highest,ff=lowest
  far_t addr;
} PACKED borland_SE;

#define FB_FB    0x4246
#define FB_OV    0x564F
typedef struct { //Borland file header
  uint16_t id[2];  // magic id FBOV_MAGIC ('FB','OV') 
  uint32_t size;   // size in bytes of the FBOV section excluding its header
  uint32_t stofs;  // offset to the boseg_t array from the start of MZ header
                   // OVERLAY.LIB names it _SEGTABLE_ or TSegMap
  int32_t  nsegs;  // number of the boseg_t entries at stofs
} PACKED bofh_t;

#define FBOV_CODE  1
#define FBOV_OVR   2
#define FBOV_DATA  4
typedef struct { //borland overlay segment entry
  uint16_t seg;    // Segment (coincides with the MZ reloc table segments)
  uint16_t maxoff; // Maximum offset inside that segment
                   // 0xFFFF - Ignored by the linker's OvrCodeReloc
  uint16_t flags;  // FBOV_CODE,FBOV_OVR,FBOV_DATA
  uint16_t minoff; // Minimum offset inside that segment,
                   // I.e. when the data/code doesn't begin on the paragrapsh
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


typedef struct { //trapped jmp, which overlay manager turns into a normal one
                 //on the first entry
  uint8_t code[2]; //int 3Fh (0xCD 0x3F)
  uint16_t ofs;    //offset inside the overlay
  uint8_t pad;     //unused
} PACKED botrp_t;

typedef struct { //trampouline to an overlayed function
                 //which rplaces botrp_t on overlay activation
  uint8_t code; //  0xEA (jmp far)
  far_t dst;    
} PACKED bojmp_t;



void fail(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int l = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char *s = NULL;
  arrsetlen(s, l+1);
  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);
  fprintf(stderr, "%s\n", s);
  exit(-1);
}


uint16_t seg_refs[0x10000];
uint16_t seg_trefs[0x10000];
mz_uniq_seg_t uniq_segs[0x10000];
int nuniq_segs;

#define MM_UNKNOWN  0
#define MM_TINY     1
#define MM_SMALL    2
#define MM_COMPACT  3
#define MM_LARGE    4
#define MM_HUGE     5

char *mmodels[] = {"unknown", "tiny", "small", "compact", "large", "huge"};

void usage() {
#define P printf
  P("MZap v0.1 by Nancy Sadkov\n");
  P("Usage: mzap [OPTIONS] <program.exe> [folder/|file.bin]\n");
  P("\n");
  P("If a folder is specified, dumps reloc tables segments there\n");
  P("Creates a folder if required\n");
  P("\n");
  P("OPTIONS:\n");
  P("  -lu                 List unique segments from relocation table\n");
  P("  -lr                 List relocation table entries\n");
  P("  -lb                 List Borland's FBOV __SEGTABLE__\n");
  P("  -lbh                List borland FBOV headers\n");
  P("  -mb                 Merge Borland FBOV section into the core MZ\n");
  P("                      The resulting exe can be loaded into Ghidra\n");
  P("  -r <base>           Relocate executable to <base> before dump.\n");
  P("                      Allows to calculate additional info.\n");
  P("                      <base> should be aligned to 16bytes.\n");
  P("                      Requires a folder or a file.\n");
  P("                      Ghidra loads it at 0x10000.\n");
  P("                      Also relocates the Borland FBOV section.\n");
  P("                      Note that IDA cant handle such exes.\n");
  P("  -ot                 Omit relocation target when determining\n");
  P("                      the unique segments\n");
  P("  -umr                Minimum references required for a segment\n");
  P("                      to be dumped under unique segments\n");
  exit(0);
#undef P
}

#if 0
void __OvrFixupFunref(uint16_t rseg, uint8_t *p) {
  //p - pointer at the fixup location


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

  //go through the botrp_t entries
  for ( ; fofs != *(uint16_t*)&q[rofs+2]; rofs += 5);
  
  *(uint16_t*)&p[4] = rofs; //relocate the offset part of the far function ptr
}
#endif


#define FIXUP_FUNREF 0x1
#define X86_JMP 0xEA

int main(int argc, char **argv) {
  int i;
  int do_reloc = 0;
  int base = 0;
  far_t *pr, *spr, *epr;
  
  int uniq_min_refs = 1;
  int uniq_targets = 1;
  int list_reloc_table = 0;
  int list_unique_segs = 0;
  int list_fbov = 0;
  int merge_fbov = 0;
  int list_bosh = 0;

  NAP_INTRO
  NAP_IF("r")
    do_reloc = 1;
    NAP_INT(base);
    if (base&0xF) fail("-r: base should be aligned to 16 bytes");
  NAP_FI
  NAP_IF("umr") NAP_INT(uniq_min_refs); NAP_FI
  NAP_IF("ot") uniq_targets = 0; NAP_FI
  NAP_IF("lu") list_unique_segs = 1; NAP_FI
  NAP_IF("lr") list_reloc_table = 1; NAP_FI
  NAP_IF("lb") list_fbov = 1; NAP_FI
  NAP_IF("lbh") list_bosh = 1; NAP_FI
  NAP_IF("mb") merge_fbov = 1; NAP_FI
  NAP_OUTRO(1,2)

  char *outpath;
  int outfolder = 0;  
  if (fargc >= 2) {
    outpath = fargv[1];
    if (outpath[strlen(outpath)-1] == '/') outfolder = 1;
    if (!outfolder && path_is_folder(outpath)) {
      outfolder = 1;
      outpath = cat(outpath,"/");
    }
  }

  if (!file_exists(fargv[0])) fail("File doesn't exist: %s", fargv[0]);
  
  int64_t file_size;
  uint8_t *file = read_whole_file(fargv[0], &file_size);
  mz_hdr_t *h = (mz_hdr_t *)file;
  if (h->magic != MZ_MAGIC) fail("Not an MZ file");
  printf("MZ Header:\n");
  if (h->csum)
    printf("* Checksum: 0x%x (normally it is 0)\n", h->csum);

  int reloc = base>>4;

  int is_borland = 0;

  int mzextsz = h->lfarlc-MZ_HDR_SZ;
  if (mzextsz) {
    borland_mz_ext_t *be = (borland_mz_ext_t*)(h+1);
    if (be->unk0 == 0x0001 && be->id == 0xFB &&
          (be->unk1 == 0x726A || be->unk1 == 0x6A72)) {
      is_borland = 1;
      printf("* Has Borland header (TLINK v%d.%d)\n"
        , be->version>>4, be->version&0xf);
    } else {
      printf("* Has extended header of size %dB\n", h->lfarlc-MZ_HDR_SZ);
      printf("  I.e. bytes at 0x%X between the MZ header and relocation table\n",
                MZ_HDR_SZ);
    }
  }


  int full_pages = h->cblp != 0 ? h->cp - 1 : h->cp;
  int loaded_sz = full_pages*MZ_PGSZ + h->cblp;

  int start_ofs = h->cparhdr*0x10;
  int extra_sz = file_size - loaded_sz;

  //SS is usually at the end of everything, so
  //if (h->ss) loaded_sz = h->ss*16 + start_ofs; 

  printf("* Size of the MZ section loaded by DOS: %dB\n", loaded_sz);
  if (loaded_sz > file_size) {
    printf("  That is more than the file size!\n");
    loaded_sz = file_size;
  }


  printf("* Can allocate from %dB up to %dB\n",h->minalloc*16,h->maxalloc*16);
  if (h->minalloc > h->maxalloc)
    printf("  minalloc > maxalloc!!!\n");

  int topload = 0;
  if (!h->minalloc && !h->maxalloc) {
    topload = 1;
    printf("* Loaded at the top of conventional memory\n");
  } else {
    printf("* Loaded just above the 256 byte PSP structure\n");
  }

  //Note: IDA loader also tries to load it from paragraph and page aligns
  int is_fbov = 0;
  int fbovofs = (loaded_sz+15)/16*16;
  bofh_t *fbov = (bofh_t*)(file + fbovofs);
  if (sizeof(bofh_t) < extra_sz && fbov->id[0]==FB_FB && fbov->id[1]==FB_OV) {
    is_fbov = 1;
  }

  if (extra_sz) {
    printf("* Extra data size: %dB\n", extra_sz, file_size);
    printf("  Resides at 0x%X\n", loaded_sz);
  
    int loaded_asz = (loaded_sz+0xF)&~0xF; //align to paragraph
    if (loaded_sz != loaded_asz) printf("  It is not aligned to 16 bytes!!!\n");

    if (is_fbov) {
      printf("  The data is Borland FBOV overlays.\n");
      if (!is_borland) printf("  But it wasn't made by TLINK!!!\n");
    } else if (loaded_sz < file_size/2) {
      printf("  Likely overlayed, compressed or PE executable\n");
    }
  }



  printf("* Header and relocation table size: %d = 0x%x\n",
    start_ofs,start_ofs);

  int rlctend = h->lfarlc + h->crlc*4; //relocation table plus MZ header
  int check_ofs = (rlctend+MZ_PGSZ-1)/MZ_PGSZ*MZ_PGSZ; //header is page aligned
                                                       //compilers zero pad it
  int rlctpad = start_ofs - rlctend;
  if (rlctpad) {
    printf("  Relocation table has %d extra bytes at the end\n", rlctpad);
    int zeropad = 1;
    for (uint8_t *pp = file+rlctend; pp < file+start_ofs; pp++) {
      if (*pp) {
        zeropad = 0;
        break;
      }
    }
    if (check_ofs == start_ofs && zeropad) {
      printf("  These are 0s aligning it to a page, so likely padding bytes\n");
    }
  }

  printf("* Start SS: 0x%x\n", h->ss+reloc);
  printf("* Start SP: 0x%x\n", h->sp);
  if (do_reloc) {
    printf("* Start CS: 0x%x\n", h->cs+reloc);
    printf("* Start IP: 0x%x\n", h->ip);
    if (topload ) {
      printf("* Start DS and ES: DOS decides\n");
    } else {
      printf("* Start DS and ES: 0x%x\n", reloc-0x10);
    }
  }

  if (h->ovno)
    printf("* Overlay Number: 0x%x (intended for dynamic load)\n", h->ovno);

  spr = (far_t*)(file+h->lfarlc);
  epr = spr + h->crlc;
  //fprintf(stderr, "Processign the reloactiong table\n");
  for (pr = spr; pr < epr; pr++) {
    seg_refs[pr->seg]++;
    int linear = start_ofs + pr->seg*16 + pr->ofs;
    if (linear > file_size) {
      printf("!!!Relocation is out of bounds, %04X:%04X!!!\n"
            , pr->seg, pr->ofs);
      printf("Halting.\n");
      return 0;
    }
    //fprintf(stderr, "%04X:%04X\n", pr->seg, pr->ofs);
    uint16_t word = *(uint16_t*)(file+linear);
    seg_trefs[word]++;
  }

  int nuniq_segs = 0;
  for (int i = 0; i < 0x10000; i++) {
    int refs = seg_refs[i];
    int trefs = seg_trefs[i];
    if (uniq_targets) refs += trefs;
    if (refs < uniq_min_refs) continue;
    uniq_segs[nuniq_segs].seg = i;
    uniq_segs[nuniq_segs].refs = seg_refs[i];
    uniq_segs[nuniq_segs].trefs = seg_trefs[i];
    uniq_segs[nuniq_segs].ofs = start_ofs + (i<<4);
    if (nuniq_segs)
      uniq_segs[nuniq_segs-1].size =
        uniq_segs[nuniq_segs].ofs - uniq_segs[nuniq_segs-1].ofs;
    ++nuniq_segs;
  }

  int last_is_ss = 0;
  if (nuniq_segs) {
    //usually last segment is the stack segment
    last_is_ss = uniq_segs[nuniq_segs-1].seg == h->ss;

    if (last_is_ss) uniq_segs[nuniq_segs-1].size = h->sp;
    else {
      uniq_segs[nuniq_segs-1].size = loaded_sz - uniq_segs[nuniq_segs-1].ofs;
    }
  }

  int mmodel = MM_UNKNOWN;

  if (!h->ss || nuniq_segs == 0) mmodel = MM_TINY;
  else if (nuniq_segs == 1) mmodel =  h->crlc > 1 ? MM_COMPACT : MM_SMALL;
  else if (nuniq_segs > 1) mmodel = MM_LARGE;

  if (mmodel) printf("* Suspected memory model: %s\n", mmodels[mmodel]);

  printf("* Relocation table offset: 0x%x\n", h->lfarlc);
  printf("* Number of relocation table entries: %d\n", h->crlc);
  printf("* Number of unique segments in relocation table: %d\n", nuniq_segs);
  if (last_is_ss) printf("  Last segment is SS\n");


  boseg_t *sbst, *ebst, *pbs;
  int boc = 0;
  if (is_fbov) {
    fbov->size += 16;
    printf("* Size of Borland FBOV extra: %dB\n", fbov->size);
    printf("  Number of segments: %d\n", fbov->nsegs);
    printf("  The __SEGTABLE__ is at 0x%X\n", fbov->stofs);
    int extras_sz2 = extra_sz - fbov->size;
    if (extras_sz2) printf("  Has additional extra data after the FBOV\n");
    sbst = (boseg_t*)(file+fbov->stofs);
    ebst = sbst + fbov->nsegs;
    for (pbs = sbst; pbs < ebst; pbs++) {
      if (pbs->flags&FBOV_OVR) boc++;
    }
  }


#if 1
  if (is_fbov && list_fbov) {

    printf("\n");
    printf("Borland FBOV segments table:\n");
    printf("The OVR segments are the overlay segment header chunks\n");
    printf("Seg  | Min Off | Max Off | Flags\n");
    for (pbs = sbst; pbs < ebst; pbs++) {
      printf("%04X |   %04X  |   %04X  | %04X"
      , pbs->seg + reloc, pbs->minoff, pbs->maxoff, pbs->flags);
      if (pbs->flags) printf(":");
      if (pbs->flags&FBOV_CODE) printf(" CODE"); 
      if (pbs->flags&FBOV_OVR) {
        printf(" OVR"); 
      }
      if (pbs->flags&FBOV_DATA) printf(" DATA"); 
      printf("\n");  
    }
  }
#endif
#if 1
  if (is_fbov && list_bosh) {
    printf("\n");
    printf("Borland FBOV header chunks (%d total):\n", boc);
    printf("Seg  | Code | SaveRet | FileOfs | Size | FixupSz\n");
    for (pbs = sbst; pbs < ebst; pbs++) {
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);
      printf("%04X | %02X%02X |   %04X  |  %06X | %04X | %04X"
        , pbs->seg + reloc,bo->code[0], bo->code[1], bo->saveret
        , fbovofs+16+bo->fileofs, bo->codesz, bo->fixupsz);
      printf("\n");  
    }
  }
#endif


  if (list_unique_segs) {
    printf("\n");
    printf("Unique segments in the MZ relocation table:\n");
    printf("Index |  Seg | EXE Offset |     Size      |  Refs | Target Refs\n");
    for (int i = 0; i < nuniq_segs; i++) {
      printf("  %3d | %04X |   %05X    | %5dB (%04X) | %5d | %5d\n"
        , i, uniq_segs[i].seg, uniq_segs[i].ofs
        , uniq_segs[i].size
        , uniq_segs[i].size
        , uniq_segs[i].refs, uniq_segs[i].trefs);
    }
  }

  if (list_reloc_table) {
    printf("\n");
    printf("MZ Relocation Table (seg:ofs relative to end of reloc table):\n");
    printf("Index |  Seg:Ofs  | EXE Offset | Value | Relocated\n");  
    i = 0;
    for (pr = spr; pr < epr; pr++) {
      int linear = start_ofs + pr->seg*16 + pr->ofs;
      uint16_t word = *(uint16_t*)(file+linear);
      printf("%5d | %04X:%04X |   %05X    |  %04X |  %04X\n"
            , i, pr->seg,pr->ofs,linear
            ,word, word + reloc);
      ++i;
    }
  }


  if (merge_fbov) {
    if (!is_fbov) {
      printf("Couldn't find the FBOV section\n");
      return 0;
    }
    if (fargc < 2) {
      printf("No output file specified\n");
      return 0;
    }

    printf("Preparing the FBOV-MZ merge...\n");
    far_t *relocs = 0;
    for (pr = spr; pr < epr; pr++) arrput(relocs, *pr);
    //int linear = start_ofs + pr->seg*16 + pr->ofs;
    //*(uint16_t*)(file+linear) += reloc;

#if 1
    printf("Untrapping the FBOV stub headers...\n");
    for (pbs = sbst; pbs < ebst; pbs++) { //untrap trampoulines
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);
      botrp_t *trp = (botrp_t*)(bo+1);

      int cofs32 = fbovofs+16+bo->fileofs;
      uint8_t *p = file + cofs32; //segments code
      cofs32 -= start_ofs;
      int cseg = (cofs32&0xFFFF0) >> 4; //code segment
      int cdisp = cofs32&0xF; //displacement

      for (i = 0; i < bo->jmpcnt; i++) {
        uint16_t ofs = trp->ofs;
        bojmp_t *trmp = (bojmp_t *)trp++;
        trmp->code = X86_JMP;
        trmp->dst.ofs = ofs + cdisp;
        trmp->dst.seg = cseg;
        far_t ptr;
        ptr.seg = pbs->seg;
        ptr.ofs = (uint8_t*)&trmp->dst.seg - file - linear;
        arrput(relocs, ptr);
      }
    }
#endif

#if 1
    printf("Rebasing FBOV fixups...\n");
    for (pbs = sbst; pbs < ebst; pbs++) { //relocate
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);

      int cofs32 = fbovofs+16+bo->fileofs;
      uint8_t *p = file + cofs32; //segments code
      cofs32 -= start_ofs;
      int cseg = (cofs32&0xFFFF0) >> 4; //code segment
      int cdisp = cofs32&0xF; //displacement

      //FBOV fixups are stored after the segment's code
      uint16_t *q = (uint16_t*)(p + bo->codesz);
      for (i = 0; i < bo->fixupsz/2; i++) {
        uint8_t *pp = &p[q[i]];
        int sf = *(uint16_t*)pp;
        int flags = sf&7; //fixup flags
        int rseg = sbst[sf>>3].seg;
        *(uint16_t*)pp = rseg;
        far_t ptr;
        ptr.ofs = pp - p + cdisp;
        ptr.seg = cseg;
        //printf("%04X:%04X\n", ptr.seg, ptr.ofs);
        arrput(relocs, ptr);
        if (flags&FIXUP_FUNREF) {
          printf("FIXME: merge function reference reloc!!!\n");
          //__OvrFixupFunref(rseg,*(uint8_t*)&p[q[i]]);
        }
      }
    }
#endif

    //printf("%d\n", file_size-start_ofs);
    h->crlc = arrlen(relocs);
    int new_cparhdr = h->lfarlc + (int32_t)h->crlc*4;
    h->cparhdr = (new_cparhdr+15)/16;   
    int hdrpadsz = (int32_t)(new_cparhdr+15)/16*16 - new_cparhdr;
    int new_file_size = h->cparhdr*16 + file_size-start_ofs;
    h->cp = (new_file_size+MZ_PGSZ-1)/MZ_PGSZ;
    h->cblp = new_file_size%MZ_PGSZ;

    printf("Dumping to %s..\n", outpath);
    FILE *fo = fopen(outpath, "wb");
    fwrite(file, 1, h->lfarlc, fo);

    for (i = 0; i < arrlen(relocs); i++) {
      //fprintf(stderr, "%04X:%04X\n", relocs[i].seg, relocs[i].ofs);
      fwrite(&relocs[i], 1, 4, fo);
    }
    if (hdrpadsz) {   //pad relocation table to paragraph size
      uint8_t pad[16];
      memset(pad, 0, hdrpadsz);
      fwrite(pad, 1, hdrpadsz, fo);
    }
    fbov->id[0] = 0; //otherwise IDA will treat it as a valid FBOV
    fbov->id[1] = 0;
    fwrite(file+start_ofs, 1, file_size-start_ofs, fo);

    fclose(fo);


#if 0
    printf("Relocating the FBOV section...\n");
    for (pbs = sbst; pbs < ebst; pbs++) { //relocate
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);

      uint8_t *p = file + fbovofs+16+bo->fileofs; //segments code

      //FBOV fixups are stored after the segment's code
      uint16_t *q = (uint16_t*)(p + bo->codesz);
      for (i = 0; i < bo->fixupsz/2; i++) {
        int sf = *(uint16_t*)&p[q[i]];
        int flags = sf&7; //fixup flags
        int rseg = sbst[sf>>3].seg + reloc;
        *(uint16_t*)&p[q[i]] = rseg;
        if (flags&FIXUP_FUNREF) {
          printf("FIXME: relocate function reference\n");
          //__OvrFixupFunref(rseg,*(uint8_t*)&p[q[i]]);
        }
      }
    }

    printf("Untrapping the FBOV stub headers...\n");
    for (pbs = sbst; pbs < ebst; pbs++) { //untrap trampoulines
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);
      botrp_t *trp = (botrp_t*)(bo+1);
      for (i = 0; i < bo->jmpcnt; i++) {
        uint16_t ofs = trp->ofs;
        bojmp_t *trmp = (bojmp_t *)trp++;
        trmp->code = X86_JMP;
        trmp->dst.ofs = ofs;
        int memofs = fbovofs+16+bo->fileofs - start_ofs + base;
        trmp->dst.seg = (memofs+15)/16;
      }
    }
#endif
    printf("Done!\n");
    return 0;
  }


  if (do_reloc && is_fbov) {
    printf("Relocating the FBOV section...\n");
    for (pbs = sbst; pbs < ebst; pbs++) { //relocate
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);

      uint8_t *p = file + fbovofs+16+bo->fileofs; //segments code

      //FBOV fixups are stored after the segment's code
      uint16_t *q = (uint16_t*)(p + bo->codesz);
      for (i = 0; i < bo->fixupsz/2; i++) {
        int sf = *(uint16_t*)&p[q[i]];
        int flags = sf&7; //fixup flags
        int rseg = sbst[sf>>3].seg + reloc;
        *(uint16_t*)&p[q[i]] = rseg;
        if (flags&FIXUP_FUNREF) {
          printf("FIXME: relocate function reference\n");
          //__OvrFixupFunref(rseg,*(uint8_t*)&p[q[i]]);
        }
      }
    }

    printf("Untrapping the FBOV stub headers...\n");
    for (pbs = sbst; pbs < ebst; pbs++) { //untrap trampoulines
      if (!(pbs->flags&FBOV_OVR)) continue;
      int linear = start_ofs + pbs->seg*16;
      bosh_t *bo = (bosh_t *)(file+linear);
      botrp_t *trp = (botrp_t*)(bo+1);
      for (i = 0; i < bo->jmpcnt; i++) {
        uint16_t ofs = trp->ofs;
        bojmp_t *trmp = (bojmp_t *)trp++;
        trmp->code = X86_JMP;
        trmp->dst.ofs = ofs;
        int memofs = fbovofs+16+bo->fileofs - start_ofs + base;
        trmp->dst.seg = (memofs+15)/16;
      }
    }
    
    //int full_pages = h->cblp != 0 ? h->cp - 1 : h->cp;
    //int loaded_sz = full_pages*MZ_PGSZ + h->cblp;
    //int start_ofs = h->cparhdr*0x10;
    //int extra_sz = file_size - loaded_sz;

    //make the resulting dump loadable by IDA Pro and Ghidra
    h->cp = (file_size+MZ_PGSZ-1)/MZ_PGSZ;
    h->cblp = file_size%MZ_PGSZ;
    fbov->id[0] = 0; //otherwise IDA will treat it as a valid FBOV
    fbov->id[1] = 0;
  }

  if (do_reloc) {
    printf("Relocating normal MZ section...\n");
    for (pr = spr; pr < epr; pr++) {
      int linear = start_ofs + pr->seg*16 + pr->ofs;
      //uint16_t ofs = *(uint16_t*)(file+linear);
      //printf("%04X -> %04X\n", ofs, ofs+reloc);
      *(uint16_t*)(file+linear) += reloc;
      pr->seg = 0; //just in case somebody tries to relocate again
      pr->ofs = 0;
    }
    h->crlc = 0; //since we relocated everything ourselves
  }

  if (fargc < 2) return 0;

  if (!outfolder) {
    printf("Dumping to %s..\n", outpath);
    write_whole_file_path(outpath, file, file_size);
    printf("Done.\n", outpath);
    return 0;
  }

  printf("\n");

#if 0
  printf("Dumping everything to %s...\n", outpath);
  for (int i = 0; i < nuniq_segs; i++) {
    char *path = fmt("%s%04d_%05X_%04X.seg", outpath, i
                   ,uniq_segs[i].ofs, uniq_segs[i].seg);
    //printf("%s\n", path);
    write_whole_file_path(path, file+uniq_segs[i].ofs, uniq_segs[i].size);
  }

  write_whole_file_path(fmt("%sd1smz.bin",outpath), file, MZ_HDR_SZ);
  write_whole_file_path(fmt("%sd2smzext.bin",outpath), file+MZ_HDR_SZ, mzextsz);
  write_whole_file_path(fmt("%sd3relocs.bin",outpath),
    file+h->lfarlc, rlctend - h->lfarlc);
  write_whole_file_path(fmt("%sd4relextra.bin",outpath), file+rlctend, rlctpad);
  write_whole_file_path(fmt("%sd5extra.bin",outpath),file+loaded_sz,extra_sz);
#endif

#if 0
  int lseg = 0;
  i = 0;
  if (is_fbov) {
    for (pbs = sbst; pbs < ebst; pbs++) {
      //if (pbs->maxoff == 0xFFFF) continue;
      //if (pbs->maxoff <= pbs->minoff) continue;
      if (pbs->seg < lseg) {
        printf("FBOV segments are not sequential!!! Terminating dump.\n");
        break;
      }
      if (!(pbs->flags&FBOV_OVR)) continue;
      lseg = pbs->seg;
    }
  }
#endif

  return 0;
}
