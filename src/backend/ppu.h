#pragma once
#include<utility.h>
#include"memory.h"
#include<frontend/display.h>

#define RES_X 160
#define RES_Y 144
#define MAX_SCANLINES_PER_FRAME
#define SCANLINE_MAX_CYCLES             456
#define FRAME_MAX_CYCLES                (SCANLINE_MAX_CYCLES*153)
#define FRAME_CYCLES_BEFORE_VBLANK      (SCANLINE_MAX_CYCLES*144)

typedef struct{
    uint8_t lcdc, stat;
    uint8_t scx, scy;
    uint8_t wx, wy;
    uint8_t window_ly, ly, lyc;
    struct{
        uint8_t ly;
    } local;
    uint64_t cycles, cycles_since_last_frame;
    bool bg_wn_updated;
    uint32_t pixels[RES_Y][RES_X];
    uint32_t bg_pixel_data[RES_Y][RES_X];
    uint32_t wn_pixel_data[RES_Y][RES_X];
} PPU;

void ppuRenderFrame(PPU* ppu, Memory* mem);
void ppuCatchup(PPU* ppu, Memory* mem, uint64_t tot_cycles);
uint8_t ppuReadLY(PPU* ppu, uint64_t tot_cycles);
