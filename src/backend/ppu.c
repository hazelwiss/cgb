#include "ppu.h"
#include <backend/events.h>
#include <stdlib.h>

#define TILES_PER_ROW 20
#define SPRITE_SIZE 4
#define SPRITE_HEIGHT 8

static int cmpSprites(const void *a, const void *b) {
    const struct SpriteStruct *_a = a;
    const struct SpriteStruct *_b = b;
    int32_t apos = _a->x_pos;
    int32_t bpos = _b->x_pos;
    return _a->x_pos - _b->x_pos;
}

__always_inline void checkLYC(PPU *ppu, Memory *mem) {
    if (ppu->ly == ppu->lyc) {
        if (ppu->stat & BIT(6)) {
            eventSTATInterrupt(mem->sched);
        }
        ppu->stat |= BIT(2);
    } else
        ppu->stat &= ~BIT(2);
}

__always_inline void changeMode(PPU *ppu, Memory *mem, enum PPUMode mode) {
    ppu->stat = (ppu->stat & (~0b11)) | mode;
    ppu->cur_mode = mode;
    switch (mode) {
    case PPUMODE0:
        if (ppu->stat & BIT(3))
            eventSTATInterrupt(mem->sched);
        break;
    case PPUMODE1:
        if (ppu->stat & BIT(4))
            eventSTATInterrupt(mem->sched);
        break;
    case PPUMODE2:
        if (ppu->stat & BIT(5))
            eventSTATInterrupt(mem->sched);
        break;
    case PPUMODE3:
        break;
    default:
        PANIC;
    }
}

__attribute_warn_unused_result__ __always_inline uint32_t
getPixelValue(PPU *ppu, uint8_t col_value, uint8_t bgp) {
    size_t pixel_value = (bgp >> (col_value * 2)) & 0b11;
    switch (col_value) {
    case 0b00:
        return 0xFFFFFF;
    case 0b01:
        return 0x999999;
    case 0b10:
        return 0x444444;
    case 0b11:
        return 0x000000;
    default:
        PANIC;
    }
    return 0;
}

__always_inline void pushToLCD(PPU *ppu, uint8_t bgp) {
    for (uint32_t x = 0; x < ppu->fifo_pixels_to_draw && ppu->cur_x_pos < RES_X;
         ++x) {
        uint32_t pixel_colour;
        if (!ppu->sprite_fifo[x].valid || !ppu->sprite_fifo[x].col_val) {
            pixel_colour = getPixelValue(ppu, ppu->bgwn_fifo[x], bgp);
        } else {
            if (ppu->sprite_fifo[x].is_transparent) {
                if (ppu->bgwn_fifo[x]) {
                    pixel_colour = getPixelValue(ppu, ppu->bgwn_fifo[x], bgp);
                } else {
                    pixel_colour =
                        getPixelValue(ppu, ppu->sprite_fifo[x].col_val, bgp);
                }
            } else {
                pixel_colour =
                    getPixelValue(ppu, ppu->sprite_fifo[x].col_val, bgp);
            }
        }
        ppu->pixels[ppu->ly][ppu->cur_x_pos++] = pixel_colour;
    }
    memset(ppu->sprite_fifo, 0, sizeof(ppu->sprite_fifo));
    memset(ppu->bgwn_fifo, 0, sizeof(ppu->bgwn_fifo));
}

__always_inline void updateFIFOBGWN(PPU *ppu) {
    for (size_t x = 0; x < ppu->fifo_pixels_to_draw; ++x) {
        bool low = ppu->fetcher.datalow & (BIT(7) >> x);
        bool high = ppu->fetcher.datahigh & (BIT(7) >> x);
        size_t pixel_value = low | (high << 1);
        ppu->bgwn_fifo[x] = pixel_value;
    }
}

__always_inline void updateFIFOSprite(PPU *ppu, uint8_t start_pos,
                                      uint8_t count) {
    for (int32_t x = start_pos; x < start_pos + count && x < PIXEL_PER_FIFO;
         ++x) {
        bool low = ppu->fetcher.datalow & (BIT(7) >> x);
        bool high = ppu->fetcher.datahigh & (BIT(7) >> x);
        size_t pixel_value = low | (high << 1);
        ppu->sprite_fifo[x].col_val = pixel_value;
        ppu->sprite_fifo[x].valid = true;
    }
}

__always_inline void fetchTileDataBGWN(PPU *ppu, Memory *mem) {
    bool mode_8000 = ppu->lcdc & BIT(4);
    uint16_t offset =
        ppu->is_window_drawing ? ppu->window_ly + ppu->wy : ppu->scy + ppu->ly;
    uint16_t address;
    if (mode_8000) {
        address = 0x8000 + ppu->fetcher.tile_n * 16 + (offset % 8) * 2;
    } else {
        address =
            0x9000 + ((int8_t)ppu->fetcher.tile_n) * 16 + (offset % 8) * 2;
    }
    ppu->fetcher.datalow = memRead(mem, address);
    ppu->fetcher.datahigh = memRead(mem, address + 1);
}

__always_inline void fetchTileDataSprite(PPU *ppu, Memory *mem,
                                         const struct SpriteStruct *sprite) {
    uint16_t offset = ppu->ly - sprite->y_pos;
    ppu->fetcher.datalow =
        memRead(mem, 0x8000 + ppu->fetcher.tile_n * 16 + (offset % 8) * 2);
    ppu->fetcher.datahigh =
        memRead(mem, 0x8000 + ppu->fetcher.tile_n * 16 + (offset % 8) * 2 + 1);
}

static void fetchTileNumberBGWN(PPU *ppu, Memory *mem) {
    uint8_t offset_y =
        ppu->is_window_drawing ? ppu->window_ly : ppu->scy + ppu->ly;
    uint8_t offset_x = (ppu->is_window_drawing ? ppu->fetcher.x - (ppu->wx - 7)
                                               : ppu->fetcher.x + ppu->scx);
    size_t adr = ((offset_y / 8) % 32) * 32 + ((offset_x / 8) % 32);
    bool map_9C00 = ppu->lcdc & (ppu->is_window_drawing ? BIT(6) : BIT(3));
    if (map_9C00)
        adr += 0x9C00;
    else
        adr += 0x9800;
    ppu->fetcher.tile_n = memRead(mem, adr);
}

static uint32_t loadFetcherBGWN(PPU *ppu, Memory *mem) {
    ppu->is_window_drawing = ppu->is_window_drawing
                                 ? ppu->is_window_drawing
                                 : ppu->lcdc & BIT(5) && ppu->ly >= ppu->wy &&
                                       ppu->fetcher.x >= ppu->wx - 7;
    ppu->increment_wly =
        ppu->increment_wly ? ppu->increment_wly : ppu->is_window_drawing;
    fetchTileNumberBGWN(ppu, mem);
    fetchTileDataBGWN(ppu, mem);
    if ((ppu->lcdc & BIT(0)) == false) {
        ppu->fetcher.datahigh = 0;
        ppu->fetcher.datalow = 0;
    }
    ppu->is_window_drawing = false;
}

static void updateBGWN(PPU *ppu, Memory *mem) {
    loadFetcherBGWN(ppu, mem);
    if (!ppu->fetcher.x) {
        uint8_t shift = ppu->scx % 8;
        ppu->fifo_pixels_to_draw = 8 - shift;
        ppu->fetcher.datahigh <<= shift;
        ppu->fetcher.datalow <<= shift;
    } else {
        ppu->fifo_pixels_to_draw = 8;
    }
    updateFIFOBGWN(ppu);
}

static void fetchSprites(PPU *ppu, Memory *mem) {
    memset(ppu->sprites, 0, sizeof(ppu->sprites));
    uint32_t added_sprites = 0;
    for (uint16_t oami = 0; oami < OAM_END - OAM_BEG; oami += SPRITE_SIZE) {
        struct SpriteStruct sprite;
        memcpy(&sprite, &mem->mmap.slowmem.oam[oami], SPRITE_SIZE);
        uint8_t y = sprite.y_pos - 16;
        uint8_t height = ppu->lcdc & BIT(2) ? 16 : 8;
        if (y <= ppu->ly && y + 8 > ppu->ly && sprite.x_pos > 0)
            memcpy(&ppu->sprites[added_sprites++], &sprite, SPRITE_SIZE);
        if (added_sprites >= MAX_SPRITES_PER_SCANLINE)
            break;
    }
    // qsort(ppu->sprites, added_sprites, SPRITE_SIZE, cmpSprites);
}

static bool renderSprite(PPU *ppu, Memory *mem, struct SpriteStruct *sprite) {
    if (ppu->lcdc & BIT(1) == 0 && ppu->ly)
        return false;
    if (ppu->sprites[0].x_pos) {
        struct SpriteStruct *p_sprite = ppu->sprites;
        uint32_t xpos1 = p_sprite->x_pos - 8;
        uint32_t xpos2 = p_sprite->x_pos;
        if (xpos1 >= ppu->fetcher.x && xpos1 < ppu->fetcher.x + 8) {
            memcpy(sprite, p_sprite, SPRITE_SIZE);
            if (xpos2 <= ppu->fetcher.x + 8) {
                memmove(ppu->sprites, &ppu->sprites[1],
                        SPRITE_SIZE * (MAX_SPRITES_PER_SCANLINE - 1));
            }
            return true;
        } else if (xpos2 > ppu->fetcher.x && xpos2 < ppu->fetcher.x + 8) {
            memcpy(sprite, p_sprite, SPRITE_SIZE);
            memmove(ppu->sprites, &ppu->sprites[1],
                    SPRITE_SIZE * (MAX_SPRITES_PER_SCANLINE - 1));
            return true;
        }
    }
    return false;
}

static void updateSprite(PPU *ppu, Memory *mem,
                         const struct SpriteStruct *sprite) {
    ppu->fetcher.tile_n = sprite->tile_n;
    fetchTileDataSprite(ppu, mem, sprite);
    if (ppu->fetcher.x <= sprite->x_pos - 8) {
        updateFIFOSprite(ppu, sprite->x_pos - 8 - ppu->fetcher.x, 8);
    } else {
        uint32_t pixels = sprite->x_pos - ppu->fetcher.x;
        ppu->fetcher.datahigh <<= 8 - pixels;
        ppu->fetcher.datalow <<= 8 - pixels;
        updateFIFOSprite(ppu, 0, pixels);
    }
}

static void ppuMode1(PPU *ppu, Memory *mem) { changeMode(ppu, mem, PPUMODE1); }

static void ppuMode2(PPU *ppu, Memory *mem) {
    changeMode(ppu, mem, PPUMODE2);
    checkLYC(ppu, mem);
}

static void ppuMode3(PPU *ppu, Memory *mem) { changeMode(ppu, mem, PPUMODE3); }

static void endOfScanline(PPU *ppu, Memory *mem) {
    ppu->window_ly += ppu->increment_wly;
    ++ppu->ly;
    ppu->fetcher.x = 0;
    ppu->increment_wly = false;
    ppu->cur_x_pos = 0;
    memset(ppu->sprites, 0, sizeof(ppu->sprites));
}

static void endOfFrame(PPU *ppu, Memory *mem) {
    ppu->ly = 0;
    ppu->window_ly = 0;
    ppu->cycles = 0;
    changeMode(ppu, mem, PPUMODE2);
    updateWindows(ppu->pixels, RES_X);
    SDL_Delay(30);
}

void ppuTick(PPU *ppu, Memory *mem) {
    if ((ppu->lcdc & BIT(7)) == 0)
        return;
    uint32_t scanline_cycles = ppu->cycles % SCANLINE_MAX_CYCLES;
    if (scanline_cycles < 80) {
        if (scanline_cycles == 0)
            ppuMode2(ppu, mem);
        else if (scanline_cycles + 1 == 80)
            fetchSprites(ppu, mem);
    } else if (ppu->ly < RES_Y && scanline_cycles >= 80) {
        if (scanline_cycles == 80) {
            ppuMode3(ppu, mem);
            ppu->fifo_timestamp = scanline_cycles + 8;
        }
        if (ppu->fetcher.x < RES_X) {
            uint8_t bgp = ppu->bgp;
            if (scanline_cycles >= ppu->fifo_timestamp) {
                struct SpriteStruct sprite;
                // if (renderSprite(ppu, mem, &sprite)) {
                //    bgp = sprite.flags & BIT(4) ? ppu->obp1 : ppu->obp0;
                //    updateSprite(ppu, mem, &sprite);
                //}
                updateBGWN(ppu, mem);
                pushToLCD(ppu, bgp);
                ppu->fifo_timestamp += 8;
                ppu->fetcher.x += 8;
            }
        } else if (scanline_cycles + 1 >= SCANLINE_MAX_CYCLES) {
            endOfScanline(ppu, mem);
            struct SpriteStruct sprite;
            for (uint32_t i = 0; i < 10; ++i) {
                memcpy(&sprite, &ppu->sprite_fifo[i], sizeof(sprite));
                if (sprite.x_pos) {
                    uint32_t pixel_colour =
                        getPixelValue(ppu, , bgp);
                }
            }
        } else if (ppu->cur_mode != PPUMODE0) {
            changeMode(ppu, mem, PPUMODE0);
        }
    } else if (ppu->cycles + 1 >= FRAME_MAX_CYCLES) {
        endOfFrame(ppu, mem);
    } else if (ppu->cur_mode != PPUMODE1) {
        ppuMode1(ppu, mem);
    }
    ++ppu->cycles;
}