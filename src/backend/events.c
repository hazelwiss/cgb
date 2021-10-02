#include"events.h"
#include<backend/cpu.h>

void eventDI(Scheduler* sched){
    sched->reference->ime = false;
}

void eventEI(Scheduler* sched){
    sched->reference->ime = true;
}

enum InterruptBit{ TIMER = 2 };
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

void eventEvaluateInterrupts(Scheduler* sched){
    uint8_t val = sched->reference->memory.mmap.slowmem.io.r_if;
    if(val & BIT(0)){

    } else if(val & BIT(1)){

    } else if(val & BIT(2)){
        dispatchInterrupt(sched->reference, TIMER);
    } else if(val & BIT(3)){

    } else if(val & BIT(4)){

    }
}

void eventPPUMode0(Scheduler* sched){
    scheduleEvent(sched, 80, ePPUMODE3);
    scheduleEvent(sched, 172, ePPUMODE0);
    scheduleEvent(sched, 456, ePPUMODE2);
}

void eventPPUMode1(Scheduler* sched){
    ppuCatchup(&sched->reference->ppu, &sched->reference->memory, sched->reference->t_cycles);
    sched->reference->ppu.local.ly = 0;
    sched->reference->ppu.ly = 0;
}

void eventPPUMode2(Scheduler* sched){

}

void eventPPUMode3(Scheduler* sched){
}

void eventPPUFrame(Scheduler* sched){
    ppuRenderFrame(&sched->reference->ppu, &sched->reference->memory);
    //SDL_Delay(300);
    sched->reference->ppu.cycles_since_last_frame = sched->reference->t_cycles;
    eventPPUMode2(sched);
    scheduleEvent(sched, 80, ePPUMODE3);
    scheduleEvent(sched, 172, ePPUMODE0);
    scheduleEvent(sched, SCANLINE_MAX_CYCLES, ePPUMODE2);
    scheduleEvent(sched, FRAME_CYCLES_BEFORE_VBLANK, ePPUMODE1);
    scheduleEvent(sched, FRAME_MAX_CYCLES, ePPUFRAME);
}
