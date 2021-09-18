/*
    A GB(Gameboy/Game Boy) emulator written in C. 
*/
#include<stdio.h>
#include"backend/cpu.h"
#include<SDL2/SDL.h>

int main(){
    SDL_Init(SDL_INIT_EVERYTHING);
    CPU* cpu = createCPU();
    setBootROM(&cpu->memory, "roms/dmg_boot.bin");
    loadROM(&cpu->memory, "roms/blargg-test-roms/cpu_instrs/individual/02-interrupts.gb");
    memWrite(&cpu->memory, 0xFF44, 0x90);
    initDisplay(cpu, "window", 160, 144);
    setRenderTileMap(true);
    setRenderBGMap(true);
    while(true){
        SDL_PumpEvents();
        updateCPU(cpu);
    }
    destroyCPU(cpu);
}