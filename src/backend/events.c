#include"events.h"
#include<backend/cpu.h>

void eventUnmountBootROM(Scheduler* sched){
    if(sched->reference->pc != 0x100){
        scheduleEvent(sched, 1, eventUnmountBootROM);
    } else{
        unmountBootROM(&sched->reference->memory);
    }
}

void eventDI(Scheduler* sched){
    sched->reference->ime = false;
}

void eventEI(Scheduler* sched){
    sched->reference->ime = true;
}

enum InterruptBit{ TIMER = 2 };
__always_inline void dispatchInterrupt(CPU* cpu, enum InterruptBit bit){
    if(cpu->memory.mmap.slowmem.io.r_ie & BIT(bit) && cpu->ime){
        memWrite(&cpu->memory, --cpu->sp, cpu->pc >> 8);
        memWrite(&cpu->memory, --cpu->sp, cpu->pc);
        switch((size_t)bit){
        case 0: cpu->pc = 0x40; break;
        case 1: cpu->pc = 0x48; break;
        case TIMER: cpu->pc = 0x50; break;
        case 3: cpu->pc = 0x58; break;
        case 4: cpu->pc = 0x60; break;
        default:
            PANIC;
        }
        cpu->memory.mmap.slowmem.io.r_if ^= BIT(bit);
        cpu->ime = false;
        tickM(cpu, 3);
    }
}

void eventInterruptTimer(Scheduler* sched){
    dispatchInterrupt(sched->reference, TIMER);
}

