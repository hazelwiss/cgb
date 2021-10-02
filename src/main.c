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
    loadROM(&cpu->memory, "roms/dmg-acid2.gb");
    memWrite(&cpu->memory, 0xFF44, 0x90);
    //setRenderTileMap(true);
    //setRenderBGMap(true);
    initDisplay(cpu, "window", 160, 144);
    while(true){
        SDL_PumpEvents();
        updateCPU(cpu);
    }
    destroyCPU(cpu);
}