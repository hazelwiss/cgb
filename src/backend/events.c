#include"events.h"
#include<backend/cpu.h>

enum InterruptBit{ 
    STAT = 1, 
    TIMER = 2,
};

void eventDI(Scheduler* sched){
    sched->reference->ime = false;
}

void eventEI(Scheduler* sched){
    sched->reference->ime = true;
}

__always_inline void dispatchInterrupt(CPU* cpu, enum InterruptBit bit){
    if(cpu->memory.mmap.slowmem.io.r_ie & BIT(bit)){
        if(cpu->ime){
            cpu->halted = false;            
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
        } else{
            if(cpu->halted){
                cpu->halted = false;
                cpu->pc += 1;
                tickM(cpu, 1);
            }
        }
    }
}

void eventTimerInterrupt(Scheduler* sched){
    sched->reference->memory.mmap.slowmem.io.r_if |= BIT(TIMER);
    eventEvaluateInterrupts(sched);
}

void eventSTATInterrupt(Scheduler* sched){
    sched->reference->memory.mmap.slowmem.io.r_if |= BIT(STAT);
    eventEvaluateInterrupts(sched);
}

void eventEvaluateInterrupts(Scheduler* sched){
    uint8_t val = sched->reference->memory.mmap.slowmem.io.r_if;
    if(val & BIT(0)){

    } else if(val & BIT(STAT)){
        dispatchInterrupt(sched->reference, STAT);
    } else if(val & BIT(TIMER)){
        dispatchInterrupt(sched->reference, TIMER);
    } else if(val & BIT(3)){

    } else if(val & BIT(4)){

    }
}
