#include"ppu.h"
#include<backend/events.h>
#define TILES_PER_ROW                   20

__always_inline void checkLYC(PPU* ppu, Memory* mem){
    if(ppu->ly == ppu->lyc){
        if(ppu->stat & BIT(6)){
            eventSTATInterrupt(mem->sched);
        }
        ppu->stat |= BIT(2);
    } else
        ppu->stat &= ~BIT(2);
}

__always_inline void changeMode(PPU* ppu, Memory* mem, enum PPUMode mode){
    ppu->stat = (ppu->stat&(~0b11)) | mode;
    ppu->cur_mode = mode;
    switch(mode){
    case PPUMODE0:
        if(ppu->stat&BIT(3))
            eventSTATInterrupt(mem->sched);
        break;
    case PPUMODE1:
        if(ppu->stat&BIT(4))
            eventSTATInterrupt(mem->sched);
        break;
    case PPUMODE2:
        if(ppu->stat&BIT(5))
            eventSTATInterrupt(mem->sched);
        break;
    case PPUMODE3:
        break;
    default:
        PANIC;
    }
}

__attribute_warn_unused_result__
__always_inline uint32_t getBGWNPixelValue(PPU* ppu, size_t x, size_t y, uint8_t data_hi, uint8_t data_lo){
    if(y >= RES_Y || x >= RES_X)
        PANIC;
    bool low    = (data_lo & (BIT(7) >> (x%8))); 
    bool high   = (data_hi & (BIT(7) >> (x%8)));
    size_t pixel_value =  (ppu->bgp >> ((low | (high << 1))*2))&0b11;
    uint32_t value;
    switch(pixel_value){
    case 0: value = 0xFFFFFF;   break;
    case 1: value = 0x999999;   break;
    case 2: value = 0x444444;   break;
    case 3: value = 0x000000;   break;
    default: PANIC;
    }
    return value;
}

__always_inline void updateFIFO(PPU* ppu, Memory* mem){
    for(size_t x = 0; x < 8; ++x){
        ppu->pixels[ppu->ly][ppu->fetcher.x + x] 
            = getBGWNPixelValue(ppu, ppu->fetcher.x+x, ppu->ly, ppu->fetcher.datahigh, ppu->fetcher.datalow);
    }
}

__always_inline void fetchTileDataBGWN(
    PPU* ppu, 
    Memory* mem, 
    bool is_hi)
{
    bool mode_8000 = ppu->lcdc & BIT(4);
    uint8_t* data = is_hi ? &ppu->fetcher.datahigh : &ppu->fetcher.datalow;
    uint16_t offset = ppu->is_window_drawing ? ppu->window_ly+ppu->wy : ppu->scy+ppu->ly;
    if(mode_8000){
        *data = memRead(mem, 0x8000 + ppu->fetcher.tile_n*16 + (offset%8)*2 + is_hi);
    } else{
        *data = memRead(mem, 0x9000 + ((int8_t)ppu->fetcher.tile_n)*16 + (offset%8)*2 + is_hi);
    }
}

static void fetchTileNumberBGWN(PPU* ppu, Memory* mem){
    uint16_t offset_y   = ppu->is_window_drawing ? ppu->window_ly : ppu->scy+ppu->ly;
    uint16_t offset_x   = ppu->is_window_drawing ? ppu->fetcher.x - (ppu->wx-7) : ppu->fetcher.x; 
    size_t adr = ((offset_y/8)%32)*32 + ((offset_x/8)%32);
    bool map_9C00 = ppu->lcdc & (ppu->is_window_drawing ? BIT(6) : BIT(3));
    if(map_9C00)
        adr += 0x9C00;
    else
        adr += 0x9800;
    ppu->fetcher.tile_n = memRead(mem, adr);
}

static void updateBGWN(PPU* ppu, Memory* mem){
    switch(ppu->fetcher.mode){
    case 0: break;
    case 1:
        ppu->is_window_drawing = ppu->is_window_drawing ? ppu->is_window_drawing : 
            ppu->lcdc & BIT(5) && ppu->ly >= ppu->wy && ppu->fetcher.x >= ppu->wx-7;
        ppu->increment_wly = ppu->increment_wly ? ppu->increment_wly : ppu->is_window_drawing;
        fetchTileNumberBGWN(ppu, mem);
        break;
    case 2: break;
    case 3: 
        fetchTileDataBGWN(ppu, mem, false);
        break;
    case 4: break;
    case 5:  
        fetchTileDataBGWN(ppu, mem, true);
        break;
    case 6: break;
    case 7: 
        updateFIFO(ppu, mem);
        ppu->fetcher.x += 8;
        ppu->is_window_drawing = false;
        break;
    default:
        PANIC;
    }
    ++ppu->fetcher.mode;
}

static void endOfScanline(PPU* ppu, Memory* mem){
    ppu->window_ly += ppu->increment_wly;
    ++ppu->ly;
    ppu->fetcher.x = 0;
    ppu->increment_wly = false;
}

static void ppuMode1(PPU* ppu, Memory* mem){
    changeMode(ppu, mem, PPUMODE1);
}

static void ppuMode2(PPU* ppu, Memory* mem){
    changeMode(ppu, mem, PPUMODE2);
    checkLYC(ppu, mem);
}

static void ppuMode3(PPU* ppu, Memory* mem){
    changeMode(ppu, mem, PPUMODE3);
}

static void endOfFrame(PPU* ppu, Memory* mem){
    ppu->ly = 0;
    ppu->window_ly = 0;
    ppu->cycles = 0;
    changeMode(ppu, mem, PPUMODE2);
    updateWindows(ppu->pixels, RES_X);
    SDL_Delay(30);
}

void ppuTick(PPU* ppu, Memory* mem){
    if((ppu->lcdc & BIT(7)) == 0)
        return;
    uint32_t scanline_cycles = ppu->cycles % SCANLINE_MAX_CYCLES;
    if(scanline_cycles < 80){
        if(scanline_cycles == 0)
            ppuMode2(ppu, mem);
    } else if(ppu->ly < RES_Y && scanline_cycles >= 80){
        if(scanline_cycles == 80){
            ppuMode3(ppu, mem);
        }
        if(ppu->fetcher.x < RES_X){
            updateBGWN(ppu, mem);
        } else if(scanline_cycles + 1 >= SCANLINE_MAX_CYCLES){
            endOfScanline(ppu, mem);
        } else if(ppu->cur_mode != PPUMODE0){
            changeMode(ppu, mem, PPUMODE0);
        }
    } else if(ppu->cycles + 1 >= FRAME_MAX_CYCLES){
        endOfFrame(ppu, mem);
    } else{
        ppuMode1(ppu, mem);
    }
    ++ppu->cycles;
} 