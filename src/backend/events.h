#pragma once

typedef struct Scheduler Scheduler;

void eventDI(Scheduler*);
void eventEI(Scheduler*);

void eventTimerInterrupt(Scheduler*);
void eventEvaluateInterrupts(Scheduler*);

void eventPPUMode0(Scheduler*);
void eventPPUMode1(Scheduler*);
void eventPPUMode2(Scheduler*);
void eventPPUMode3(Scheduler*);
void eventPPUFrame(Scheduler*);

typedef enum{
    eDI = 0,
    eEI,
    eTIMER_INTERRUPT,
    eEVALUATE_INTERRUPTS,
    ePPUMODE0,
    ePPUMODE1,
    ePPUMODE2,
    ePPUMODE3,
    ePPUFRAME,
    eCOUNT
} EventEnum;