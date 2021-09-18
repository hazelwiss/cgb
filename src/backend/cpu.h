/*
    Header include the implementation of the Custom 8-bit Sharp LR35902(https://en.wikipedia.org/wiki/Game_Boy) that's
    based on a modified Z80 and 8080 chip Possibly a SM83 core. 
    The processor of the GB runs at a clock speed of around 4.19Mhz
*/
#pragma once
#include<utility.h>
#include"memory.h"
#include"ppu.h"
#include"scheduler.h"

#define REGISTER_UNION(UPPER, LOWER)    \
union{                                  \
struct{ uint8_t LOWER; uint8_t UPPER; }; uint16_t UPPER ##LOWER;  }

typedef struct CPU{
    union{
        struct{
            struct{
                uint8_t unused:4, c:1, h:1, n:1, z:1; 
            } f;
            uint8_t a;
        };
        uint16_t af;
    };
    REGISTER_UNION(b, c);
    REGISTER_UNION(d, e);
    REGISTER_UNION(h, l);
    uint16_t sp;
    uint16_t pc;
    Memory memory;
    PPU ppu;
    Scheduler sched;
    uint64_t t_cycles;
    bool ime;
} CPU;

CPU* createCPU(void);
void destroyCPU(CPU*);

void updateCPU(CPU*);

void tickM(CPU* cpu, size_t cycles);