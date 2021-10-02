#include"memory.h"
#include<stdio.h>
#include<string.h>
#include<backend/events.h>
#include<backend/cpu.h>

#define IO_DIV 0x04
#define IO_TIMA 0x05
#define IO_TMA 0x06
#define IO_TAC 0x07
#define IO_IF 0x0F
#define IO_LCDC 0x40
#define IO_STAT 0x41
#define IO_SCY 0x42 
#define IO_SCX 0x43 
#define IO_LY 0x44
#define IO_LYC 0x45 
#define IO_WY 0x4A
#define IO_WX 0x4B

static bool isSlowMemAccess(uint16_t adr){
    switch(adr){
    case VRAM_BEG   ...     VRAM_END:
    case ERAM_BEG   ...     ERAM_END:
    case OAM_BEG    ...     OAM_END:  
    case IO_BEG     ...     IO_END:   
    case IE:   
    case 0x100:
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

static uint8_t readIO(Memory* mem, uint16_t adr){
    uint16_t real_adr = adr-IO_BEG;
    if(real_adr > IO_END)
        PANIC;
    switch(real_adr){
    case IO_DIV:    return mem->mmap.slowmem.io.div;
    case IO_TIMA:   return mem->mmap.slowmem.io.tima; 
    case IO_TMA:    return mem->mmap.slowmem.io.tma;
    case IO_TAC:    return mem->mmap.slowmem.io.tac;
    case IO_LCDC:   return mem->sched->reference->ppu.lcdc;
    case IO_STAT:   return mem->sched->reference->ppu.stat;
    case IO_SCY:    return mem->sched->reference->ppu.scy;
    case IO_SCX:    return mem->sched->reference->ppu.scx;
    case IO_LY:{
        return ppuReadLY(&mem->sched->reference->ppu, mem->sched->reference->t_cycles);
    }     
    case IO_LYC:    return mem->sched->reference->ppu.lyc;
    case IO_WY:     return mem->sched->reference->ppu.wy;
    case IO_WX:     return mem->sched->reference->ppu.wx;
    case IO_IF:     return mem->mmap.slowmem.io.r_if;
    default:        return mem->mmap.slowmem.io.data[adr-IO_BEG];
    }
}

static void writeIO(Memory* mem, uint16_t adr, uint8_t val){
    uint16_t real_adr = adr-IO_BEG;
    if(real_adr > IO_END)
        PANIC;
    switch(real_adr){
    case IO_TIMA:{
        mem->mmap.slowmem.io.tima = val;
        if(mem->mmap.slowmem.io.tac&BIT(2)){
            switch(mem->mmap.slowmem.io.tac&0b11){
            case 0:
                scheduleEvent(mem->sched, (0xFF-val)*1024, eTIMER_INTERRUPT);
                break;
            case 1:
                scheduleEvent(mem->sched, (0xFF-val)*16, eTIMER_INTERRUPT);
                break;
            case 2:
                scheduleEvent(mem->sched, (0xFF-val)*64, eTIMER_INTERRUPT);
                break;
            case 3:
                scheduleEvent(mem->sched, (0xFF-val)*256, eTIMER_INTERRUPT);
                break;
            }
        }
        break;
    }
    case IO_TMA:{
        mem->mmap.slowmem.io.tma = val;
        break;
    }
    case IO_TAC:{
        mem->mmap.slowmem.io.tac = val;
        break;
    }
    case IO_LCDC:{
        if(val & BIT(7) && !(mem->sched->reference->ppu.lcdc & BIT(7))){
            mem->sched->reference->ppu.cycles_since_last_frame = mem->sched->reference->t_cycles;
            eventPPUFrame(mem->sched);
        }
        ppuCatchup(&mem->sched->reference->ppu, mem, mem->sched->reference->t_cycles);
        mem->sched->reference->ppu.lcdc = val;
        break;
    }
    case IO_STAT:{
        mem->sched->reference->ppu.stat = val;
        break;
    } 
    case IO_SCY:{
        ppuCatchup(&mem->sched->reference->ppu, mem, mem->sched->reference->t_cycles);
        mem->sched->reference->ppu.scy = val;
        break;
    }
    case IO_SCX:{
        ppuCatchup(&mem->sched->reference->ppu, mem, mem->sched->reference->t_cycles);
        mem->sched->reference->ppu.scx = val;
        break;
    } 
    case IO_LY:{
        //  Read only.
        break;
    }   
    case IO_LYC:{
        mem->sched->reference->ppu.lyc = val;
        break;
    }  
    case IO_WY:{
        ppuCatchup(&mem->sched->reference->ppu, mem, mem->sched->reference->t_cycles);
        mem->sched->reference->ppu.wy = val;
        break;
    }  
    case IO_WX:{
        ppuCatchup(&mem->sched->reference->ppu, mem, mem->sched->reference->t_cycles);
        mem->sched->reference->ppu.wx = val;
        break;
    }   
    case IO_IF:{
        mem->mmap.slowmem.io.r_if = val;
        eventEvaluateInterrupts(mem->sched);
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
        case 0x100:                 unmountBootROM(mem);
                                    return mem->mmap.fastmem[0x100];
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
        case IE:    mem->mmap.slowmem.io.r_ie = val;
                    eventEvaluateInterrupts(mem->sched);        break;
        case 0x100:                 writeRom(mem, 0x100, val);  break;
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
        PANIC;
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
