#pragma once
#include <utility.h>
#include "memory.h"
#include <frontend/display.h>

#define RES_X 160
#define RES_Y 144
#define SCANLINE_MAX_CYCLES 456
#define FRAME_MAX_CYCLES (SCANLINE_MAX_CYCLES * 154)
#define FRAME_CYCLES_BEFORE_VBLANK (SCANLINE_MAX_CYCLES * 144)
#define MAX_SPRITES_PER_SCANLINE 10
#define PIXEL_PER_FIFO 8

enum PPUMode {
    PPUMODE0 = 0,
    PPUMODE1,
    PPUMODE2,
    PPUMODE3,
};

struct PixelFetcher {
    uint8_t x;
    uint8_t tile_n;
    uint8_t datalow;
    uint8_t datahigh;
};

__attribute__((packed)) struct SpriteStruct {
    uint8_t y_pos;
    uint8_t x_pos;
    uint8_t tile_n;
    uint8_t flags;
};

struct SpritePixel {
    uint8_t col_val;
    bool is_transparent;
    bool valid;
};

_Static_assert(sizeof(struct SpriteStruct) == 4,
               "invalid size for sprite struct.");

typedef struct {
    uint8_t lcdc, stat;
    uint8_t scx, scy;
    uint8_t wx, wy;
    uint8_t window_ly, ly, lyc;
    uint8_t bgwn_fifo[PIXEL_PER_FIFO];
    uint8_t fifo_timestamp;
    uint8_t fifo_pixels_to_draw;
    uint8_t cur_x_pos;
    uint32_t cycles;
    uint8_t bgp;
    uint8_t obp0, obp1;
    bool increment_wly;
    bool is_window_drawing;
    struct SpritePixel sprite_fifo[PIXEL_PER_FIFO];
    uint32_t pixels[RES_Y][RES_X];
    struct PixelFetcher fetcher;
    enum PPUMode cur_mode;
    struct SpriteStruct sprites[MAX_SPRITES_PER_SCANLINE];
} PPU;

void ppuTick(PPU *ppu, Memory *mem);
