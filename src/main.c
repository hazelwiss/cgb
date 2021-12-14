/*
    A GB(Gameboy/Game Boy) emulator written in C.
*/
#include <stdio.h>
#include "backend/cpu.h"
#include <SDL2/SDL.h>

int main(int argc, char *argv[]) {
    CPU *cpu = createCPU();
    setBootROM(&cpu->memory, "roms/dmg_boot.bin");
    loadROM(&cpu->memory, argv[1]);
    memWrite(&cpu->memory, 0xFF44, 0x90);
    initDisplay(cpu, "gbemu", 160, 144);
    while (true) {
        updateCPU(cpu);
    }
    destroyCPU(cpu);
}