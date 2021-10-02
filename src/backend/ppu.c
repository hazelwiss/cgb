#include"ppu.h"
#define TILES_PER_ROW                   20

__attribute_warn_unused_result__
__always_inline uint32_t getPixelValue(PPU* ppu, size_t x, size_t y, uint8_t data_hi, uint8_t data_lo){
    if(y >= RES_Y || x >= RES_X)
        PANIC;
    bool low    = (data_lo & (BIT(7) >> (x%8))); 
    bool high   = (data_hi & (BIT(7) >> (x%8)));
    size_t pixel_value = low | (high << 1);
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

__always_inline void internalScanlineBGWNUpdateTile(PPU* ppu, Memory* mem, 
    uint8_t x_pos, uint8_t y_pos, uint16_t map_base_adr, 
    uint16_t tile_base_adr, int16_t tile_n)
{
    uint8_t tile_index = memRead(mem, map_base_adr + ((y_pos + ppu->scy)/8)*32 + tile_n);
    for(size_t x = 0; x < 8; ++x){
        size_t index = tile_base_adr + tile_index*16 + ((y_pos + ppu->scy )%8)*2;
        uint8_t tile_lo = memRead(mem, index);
        uint8_t tile_hi = memRead(mem, index + 1);
        ppu->bg_pixel_data[y_pos][x_pos + x] 
            = getPixelValue(ppu, x_pos+x, y_pos, tile_hi, tile_lo);
    }
}

__always_inline void internalScanlineBGWNUpdate(PPU* ppu, Memory* mem, bool map_9C00, bool mode_8000){
    size_t adr = (((ppu->scy+ppu->local.ly)/8)%32)*32 + ((ppu->scx/8)%32);
    if(map_9C00)
        adr += 0x9C00;
    else
        adr += 0x9800;
    if(mode_8000){
        for(size_t tile_n = 0; tile_n < 20; ++tile_n){
            uint8_t x = (tile_n%TILES_PER_ROW)*8;
            internalScanlineBGWNUpdateTile(ppu, mem, x, ppu->local.ly, 
                adr, 0x8000, tile_n);
        }
    } else{
        for(size_t tile_n = 0; tile_n < 20; ++tile_n){
            uint8_t x = (tile_n%TILES_PER_ROW)*8;
            internalScanlineBGWNUpdateTile(ppu, mem, x, ppu->local.ly, 
                adr, 0x9000, (int8_t)tile_n);
        }
    }
}

static void ppuScanlineUpdateBGAndWNData(PPU* ppu, Memory* mem){
    bool map_9C00 = ppu->lcdc & BIT(3);
    bool mode_8000 = ppu->lcdc & BIT(4);
    internalScanlineBGWNUpdate(ppu, mem, map_9C00, mode_8000);
    ppu->bg_wn_updated = true;
}

void ppuCatchup(PPU* ppu, Memory* mem, uint64_t tot_cycles){
    if((ppu->lcdc & BIT(7)) == 0)
        return;
    ppu->ly += (tot_cycles - ppu->cycles_since_last_frame)/SCANLINE_MAX_CYCLES;
    if(ppu->ly > RES_Y)
        PANIC;
    int32_t diff = ppu->ly - ppu->local.ly;
    if(diff < 0 || ppu->local.ly + diff > RES_Y)
        PANIC;
    do{
        ppuScanlineUpdateBGAndWNData(ppu, mem);
        ppu->local.ly += diff != 0;
    } while(--diff > 0);
    ppu->cycles_since_last_frame = tot_cycles;
}

void ppuRenderFrame(PPU* ppu, Memory* mem){
    if((ppu->lcdc & BIT(7)) == 0)
        return;
    for(size_t y = 0; y < RES_Y; ++y){
        for(size_t x = 0; x < RES_X; ++x){
            if(y >= ppu->wy && x >= ppu->wx - 7 && ppu->lcdc & BIT(5)){
                ppu->pixels[y][x] = ppu->wn_pixel_data[y][x];
            } else{
                ppu->pixels[y][x] = ppu->bg_pixel_data[y][x];
            }
        }
    }
    updateWindows(ppu->pixels, RES_X);
    ppu->bg_wn_updated = false;
} 

static uint8_t e = 2;
static uint8_t c = 2;
uint8_t ppuReadLY(PPU* ppu, uint64_t tot_cycles){
    if((tot_cycles - ppu->cycles_since_last_frame)/SCANLINE_MAX_CYCLES == 0x90){
        --c;
        if(!c){
            --e;
        }
        printf("e: %d c: %d\n", e, c);
    }

    return (tot_cycles - ppu->cycles_since_last_frame)/SCANLINE_MAX_CYCLES;
}