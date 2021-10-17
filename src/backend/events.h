#pragma once

typedef struct Scheduler Scheduler;
typedef void(*EventFunc)(Scheduler*);

void eventDI(Scheduler*);
void eventEI(Scheduler*);

void eventTimerInterrupt(Scheduler*);
void eventSTATInterrupt(Scheduler*);
void eventEvaluateInterrupts(Scheduler*);

typedef enum{
    eDI = 0,
    eEI,
    eTIMER_INTERRUPT,
    eEVALUATE_INTERRUPTS,
    eCOUNT
} EventEnum;