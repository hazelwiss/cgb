#pragma once
#include<utility.h>
#include"memory.h"
#include<shift_register.h>
#include<frontend/display.h>

#define RES_X 160
#define RES_Y 144

typedef struct{
    uint8_t x;
    size_t step;
    uint8_t tile;
    uint8_t data_low;
    uint8_t data_high;
} PixelFetcher;

typedef struct{
    ShiftRegister bgfifo;
    ShiftRegister sfifo;
    PixelFetcher fetcher;
    uint8_t lcdc;
    uint8_t scx, scy;
    uint8_t ly;
    size_t cycles;
    uint32_t pixels[RES_Y][RES_X];
} PPU;

void tickPPU(PPU* ppu, Memory* memory);