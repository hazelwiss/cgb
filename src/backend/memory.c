#include"memory.h"
#include<stdio.h>
#include<string.h>
#include<backend/events.h>

static bool isSlowMemAccess(uint16_t adr){
    switch(adr){
    case VRAM_BEG   ...     VRAM_END:
    case ERAM_BEG   ...     ERAM_END:
    case OAM_BEG    ...     OAM_END:  
    case IO_BEG     ...     IO_END:   
    case IE:   
        return true;
    }
    return false;
}

static void writeRom(Memory* mem, uint16_t adr, uint8_t val){

}

static uint8_t readVRAM(Memory* mem, uint16_t adr){
    return mem->mmap.slowmem.vram[adr-VRAM_BEG];
}

static void writeVRAM(Memory* mem, uint16_t adr, uint8_t val){
    mem->mmap.slowmem.vram[adr-VRAM_BEG] = val;
}

static void evaluateInterrupts(Memory* mem, uint8_t val){
    mem->mmap.slowmem.io.r_if = val;
    if(val & BIT(0)){

    } else if(val & BIT(1)){

    } else if(val & BIT(2)){
        eventInterruptTimer(mem->sched);
    } else if(val & BIT(3)){

    } else if(val & BIT(4)){

    }
}

static uint8_t readIO(Memory* mem, uint16_t adr){
    uint16_t real_adr = adr-IO_BEG;
    if(real_adr > IO_END)
        PANIC;
    switch(real_adr){
    case 0x0F: return mem->mmap.slowmem.io.r_if;
    default:
        return mem->mmap.slowmem.io.data[adr-IO_BEG];
    }
}

static void writeIO(Memory* mem, uint16_t adr, uint8_t val){
    uint16_t real_adr = adr-IO_BEG;
    if(real_adr > IO_END)
        PANIC;
    switch(real_adr){
    case 0x05: break;
    case 0x06: break;
    case 0x0F:{
        evaluateInterrupts(mem, val);
        break;
    } 
    default:
        mem->mmap.slowmem.io.data[adr-IO_BEG] = val;
    }
}

static uint8_t readOAM(Memory* mem, uint16_t adr){
    return mem->mmap.slowmem.oam[adr-OAM_BEG];
}

static void writeOAM(Memory* mem, uint16_t adr, uint8_t val){
    mem->mmap.slowmem.oam[adr] = val;
}

static uint8_t readERAM(Memory* mem, uint16_t adr){
    return mem->mmap.slowmem.eram[adr-ERAM_BEG];
}

static void writeERAM(Memory* mem, uint16_t adr, uint8_t val){
    mem->mmap.slowmem.eram[adr-ERAM_BEG] = val;
}

uint8_t memRead(Memory* mem, uint16_t adr){
    if(isSlowMemAccess(adr)){
        switch (adr){
        case VRAM_BEG ... VRAM_END: return readVRAM(mem, adr);
        case ERAM_BEG ... ERAM_END: return readERAM(mem, adr);
        case OAM_BEG ... OAM_END:   return readOAM(mem, adr);
        case IO_BEG ... IO_END:     return readIO(mem, adr);
        case IE:                    return mem->mmap.slowmem.io.r_ie;
        default: PANIC;
        }
    } else
        return mem->mmap.fastmem[adr];
    return 0;
}

void memWrite(Memory* mem, uint16_t adr, uint8_t val){
    if(isSlowMemAccess(adr)){
        switch (adr){
        case VRAM_BEG ... VRAM_END: writeVRAM(mem, adr, val);   break;
        case ERAM_BEG ... ERAM_END: writeERAM(mem, adr, val);   break;
        case OAM_BEG ... OAM_END:   writeOAM(mem, adr, val);    break;
        case IO_BEG ... IO_END:     writeIO(mem, adr, val);     break;
        case IE:    evaluateInterrupts(mem, val);
                    mem->mmap.slowmem.io.r_ie = val;               break;
        default: PANIC;
        }
    } else if(adr <= 0x7FFF)
        writeRom(mem, adr, val); 
    else
        mem->mmap.fastmem[adr] = val;
        
}

void loadROM(Memory* mem, const char* path){
    if(mem->boot_rom_path[0] == '\0'){
        fprintf(stderr, "no boot rom specified! Panicking!\n");
        PANIC;
    }
    FILE* file = fopen(path, "r");
    if(!file){
        fprintf(stderr, "%s rom could not be found! Panicking!\n", path);
        PANIC;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    if(size > KB(32)){
        fprintf(stderr, "rom file too large! Panicking!\n");
        PANIC;
    }
    rewind(file);
    fread(mem->mmap.fastmem, 1, size, file);
    memcpy(mem->unmapped_rom, mem->mmap.fastmem, 256);
    fclose(file);
    file = fopen(mem->boot_rom_path, "r");
    if(!file){
        fprintf(stderr, "%s bootrom could not be found! Panicking!\n", mem->boot_rom_path);
        PANIC
    }
    fread(mem->boot_rom, 1, 256, file);
    memcpy(mem->mmap.fastmem, mem->boot_rom, 256);
    fclose(file);
    strcpy(mem->rom_path, path);
}

void setBootROM(Memory* mem, const char* path){
    strcpy(mem->boot_rom_path, path);
}

void unmountBootROM(Memory* mem){
    memcpy(mem->mmap.fastmem, mem->unmapped_rom, 256);
}