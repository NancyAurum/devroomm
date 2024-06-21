//This decompilation was done by ChatGPT
#include <stdint.h>
#include <string.h>
#include <dos.h>

extern uint16_t HEAPORG;
extern uint16_t HEAPPTR;
extern uint16_t OVRDEBUGPTR;
extern uint16_t OVRLOADLIST;
extern uint16_t OVRHEAPSIZE;
extern uint16_t OVRHEAPEND;
extern uint16_t OVRHEAPPTR;
extern uint16_t OVRDOSHANDLE;
extern uint16_t OVREMSHANDLE;
extern uint16_t EXITPROC;
extern uint16_t PREFIXSEG;
extern uint16_t OVRCODELIST;
extern uint16_t OVRHEAPORG;
extern uint16_t OVRRESULT;

#define OVRINITEMS$0 {69, 77, 77, 88, 88, 88, 88, 48}

void ovrinititems() {
    uint16_t ax = 0;
    if (ax == OVRDOSHANDLE) {
        ax--;
        OVRRESULT = ax;
        return;
    }
    if (ovr3() == 0) {
        ax = 65531;
        OVRRESULT = ax;
        return;
    }
    if (ovr5() >= 0) {
        ax = 65530;
        OVRRESULT = ax;
        return;
    }
    if (ovr7() >= 0) {
        uint16_t dx = OVREMSHANDLE;
        _AH = 69;  // 'E'
        int86(0x67, &regs, &regs);
        ax = 65532;
        OVRRESULT = ax;
        return;
    }
    uint16_t bx = OVRDOSHANDLE;
    _AH = 62;  // '>'
    int86(0x21, &regs, &regs);
    uint16_t *ptr = (uint16_t *)(0x00B7 + 368); // Adjust the offset appropriately
    *ptr = (uint16_t)(uintptr_t)0; // Replace 0 with actual pointer
    uint16_t *exitproc = (uint16_t *)(uintptr_t)EXITPROC;
    *exitproc = 0;
    *(exitproc + 1) = _CS;
    ax = 0;
    OVRRESULT = ax;
    return;
}

int16_t ovr3() {
    uint16_t ax = 13671;
    int86(0x21, &regs, &regs);
    uint8_t cx = 8;
    uint8_t *si = OVRINITEMS$0;
    uint8_t di = 10;
    regs.x.ds = _CS;
    _DS = _CS;
    for (; cx > 0; cx--, si++, di++) {
        if (*si != *(uint8_t *)di) return 0;
    }
    return 1;
}

int16_t ovr5() {
    _AH = 65;  // 'A'
    int86(0x67, &regs, &regs);
    uint16_t bx = regs.x.bx;
    uint16_t ax = 16383;
    uint16_t dx = 0;
    bx = OVRCODELIST;
    do {
        bx += PREFIXSEG + 16;
        _ES = bx;
        _ADC(dx, 0);
        _OR(bx, bx);
    } while (bx != 0);
    bx = 16384;
    _DIV(bx);
    bx = regs.x.ax;
    _AH = 67;  // 'C'
    int86(0x67, &regs, &regs);
    if (!(_AH & 1)) {
        OVREMSHANDLE = dx;
    }
    return 1;
}

int16_t ovr7() {
    uint16_t bp = _SP;
    uint16_t dx = OVREMSHANDLE;
    _AH = 71;  // 'G'
    int86(0x67, &regs, &regs);
    uint16_t ax = OVRCODELIST;
    uint16_t cx = 0;
    do {
        ax += PREFIXSEG + 16;
        _ES = ax;
        uint16_t es_ax = ax;
        _PUSH(ax);
        cx++;
    } while (ax != 0);
    uint16_t bx = 0;
    uint16_t di = 0;
    do {
        _POP(_ES);
        uint16_t es = es_ax;
        _PUSH(cx);
        ax = OVRHEAPORG;
        _PUSH(bx);
        _PUSH(di);
        // Call function here
        _POP(di);
        _POP(bx);
        _JB(12);
        // Call function here
        _JB(12);
        _POP(cx);
    } while (cx-- > 0);
    uint16_t dx = OVREMSHANDLE;
    _AH = 72;  // 'H'
    int86(0x67, &regs, &regs);
    _SP = bp;
    return 1;
}

void ovrinit() {
    uint16_t bp = _SP;
    _SP -= 128;
    _CLD;
    if (OVRCODELIST == 0) {
        if (ovr3() >= 0) {
            if (ovr5() >= 0) {
                if (ovr7() == 0) {
                    OVRRESULT = 65534;
                    _SP = bp;
                    return;
                }
            }
        }
    } else {
        OVRRESULT = 65535;
        _SP = bp;
        return;
    }
    OVRRESULT = 0;
    _SP = bp;
}

uint32_t ovrgetbuf() {
    uint16_t ax = OVRHEAPEND - OVRHEAPORG;
    uint8_t cl = 4;
    ax = _ROL(ax, cl);
    uint16_t dx = ax;
    ax &= 65520;
    dx &= 15;
    return (uint32_t)ax << 16 | dx;
}

void ovrclearbuf() {
    uint16_t bp = _SP;
    if (OVRDOSHANDLE == 0) {
        OVRRESULT = 65535;
        _SP = bp;
        return;
    }
    uint16_t ax = OVRHEAPORG;
    OVRHEAPPTR = ax;
    ax = OVRLOADLIST;
    do {
        _ES = ax;
        // Call function here
        ax = regs.x.ax;
    } while (ax != 0);
    OVRLOADLIST = ax;
    OVRRESULT = ax;
    _SP = bp;
}
