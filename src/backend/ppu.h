#pragma once
#include<utility.h>
#include"memory.h"
#include<frontend/display.h>

#define RES_X 160
#define RES_Y 144
#define SCANLINE_MAX_CYCLES             456
#define FRAME_MAX_CYCLES                (SCANLINE_MAX_CYCLES*154)
#define FRAME_CYCLES_BEFORE_VBLANK      (SCANLINE_MAX_CYCLES*144)

enum PPUMode{
    PPUMODE0 = 0, PPUMODE1, PPUMODE2, PPUMODE3, 
};

struct PixelFetcher{
    uint8_t x;
    uint8_t tile_n;
    uint8_t datalow;
    uint8_t datahigh;
    uint8_t mode:3;
};

typedef struct{
    uint8_t lcdc, stat;
    uint8_t scx, scy;
    uint8_t wx, wy;
    uint8_t window_ly, ly, lyc;
    uint32_t cycles;
    struct PixelFetcher fetcher; 
    uint32_t pixels[RES_Y][RES_X];
    uint8_t bgp;
    enum PPUMode cur_mode;
    bool increment_wly;
    bool is_window_drawing;
} PPU;

void ppuTick(PPU* ppu, Memory* mem);
