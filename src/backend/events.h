#pragma once

typedef struct Scheduler Scheduler;

void eventUnmountBootROM(Scheduler*);

void eventDI(Scheduler*);
void eventEI(Scheduler*);

void eventInterruptTimer(Scheduler*);