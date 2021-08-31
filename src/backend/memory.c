#include"memory.h"
#include<stdbool.h>

static bool isSlowMemAccess(uint16_t adr){
    switch(adr){
    case 0x8000 ... 0x9FFF:
    case 0xFF00 ... 0xFF7F:
    case 0xFFFF:
        return true;
    }
    return false;
}

static uint8_t readVRAM(){}

static uint8_t writeVRAM(){}

static uint8_t readIO(){}

static uint8_t writeIO(){}


uint8_t read(Memory* mem, uint16_t adr){
    if(isSlowMemAccess(adr)){

    } else
        return mem->mmap[adr];
}

void write(Memory* mem, uint16_t adr, uint8_t val){
    if(isSlowMemAccess(adr)){

    } else
        mem->mmap[adr] = val;
}