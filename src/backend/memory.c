#include"memory.h"

static bool isSlowMemAccess(uint16_t adr){
    switch(adr){
    case 0x8000 ... 0x9FFF:
    case 0xFE00 ... 0xFE9F:
    case 0xFF00 ... 0xFF7F:
    case 0xFFFF:
        return true;
    }
    return false;
}

static uint8_t readVRAM(){}

static void writeVRAM(){}

static uint8_t readIO(){}

static void writeIO(){}

static uint8_t readOAM(){}

static void writeOAM(){}


uint8_t read(Memory* mem, uint16_t adr){
    if(isSlowMemAccess(adr)){
        switch (adr){
        case 0x8000 ... 0x9FFF: return readVRAM();
        case 0xFE00 ... 0xFE9F: return readOAM();
        case 0xFF00 ... 0xFF7F:
        case 0xFFFF:            return readIO();
        default: PANIC;
        }
    } else
        return mem->mmap[adr];
    return 0;
}

void write(Memory* mem, uint16_t adr, uint8_t val){
    if(isSlowMemAccess(adr)){
        switch (adr){
        case 0x8000 ... 0x9FFF: writeVRAM();    break;
        case 0xFE00 ... 0xFE9F: writeOAM();     break;
        case 0xFF00 ... 0xFF7F:
        case 0xFFFF:            writeIO();      break;
        default: PANIC;
        }
    } else
        mem->mmap[adr] = val;
}

void loadROM(Memory* mem, const char* path){
    
}