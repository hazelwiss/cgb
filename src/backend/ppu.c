#include"ppu.h"
#define SCANLINE_MAX_CYCLES             456
#define FRAME_MAX_CYCLES                (SCANLINE_MAX_CYCLES*153)
#define FRAME_CYCLES_BEFORE_VBLANK      (SCANLINE_MAX_CYCLES*143)

static void getTileBG(PPU* ppu, Memory* mem){
    size_t tile_index = 
        (((ppu->fetcher.x + ppu->scx)/8)&0x1F) + 
        ((((ppu->ly + ppu->scy)/8)&0x1F)*32);
    if(ppu->lcdc & BIT(3))
        ppu->fetcher.tile = memRead(mem, 0x9C00+tile_index);
    else
        ppu->fetcher.tile = memRead(mem, 0x9800+tile_index);
}

static void fetchDataBG(PPU* ppu, Memory* mem, bool high, uint8_t* data){
    size_t offset = ((ppu->ly + ppu->scy)%8)*2 + high;
    uint16_t index;
    if(ppu->lcdc & BIT(4))  //  8000 method
        index = 0x8000 + ppu->fetcher.tile*16 + offset;
    else                    //  8800 method
        index = 0x9000 + ((int8_t)ppu->fetcher.tile)*16 + offset;
    if((ppu->scx % 8) == 0){
        uint8_t tile_data = memRead(mem, index);
        *data = tile_data;
    } else{
        uint8_t tile_data = memRead(mem, index) << (ppu->scx % 8);
        tile_data |= memRead(mem, index + 1) >> (8-(ppu->scx % 8));
        *data = tile_data;
    }
}

static void fetchDataLowBG(PPU* ppu, Memory* mem){
    fetchDataBG(ppu, mem, false, &ppu->fetcher.data_low);
}

static void fetchDataHighBG(PPU* ppu, Memory* mem){
    fetchDataBG(ppu, mem, true, &ppu->fetcher.data_high);
}

static void pushToFIFOBG(PPU* ppu){
    uint32_t value;
    for(int x = ppu->fetcher.x; x < ppu->fetcher.x + 8; ++x){
        bool low    = (ppu->fetcher.data_low & (BIT(7) >> (x%8))); 
        bool high   = (ppu->fetcher.data_high & (BIT(7) >> (x%8)));
        size_t pixel_value = low | (high << 1);
        switch(pixel_value){
        case 0: value = 0xFFFFFF;   break;
        case 1: value = 0x999999;   break;
        case 2: value = 0x444444;   break;
        case 3: value = 0x000000;   break;
        default: PANIC;
        }
        if(ppu->ly >= RES_Y || x >= RES_X)
            PANIC;
        ppu->pixels[ppu->ly][x] = value;
    }
}

static void updateBG(PPU* ppu, Memory* mem){
    switch(ppu->fetcher.step){
    case 0:
        getTileBG(ppu, mem);
        break;
    case 1: break;
    case 2:
        fetchDataLowBG(ppu, mem);
        break;
    case 3: break;
    case 4:
        fetchDataHighBG(ppu, mem);
        break;
    case 5: break;
    case 6:
        pushToFIFOBG(ppu);
        break;
    case 7: 
        ppu->fetcher.x += 8; 
        break;
    default:
        PANIC;
    }
    ppu->fetcher.step = ppu->fetcher.step >= 7 ? 0 : ppu->fetcher.step + 1;
}

void tickPPU(PPU* ppu, Memory* mem){
    ppu->lcdc = memRead(mem, 0xFF40);
    //uint8_t stat = memRead(memory, 0xFF41);
    ppu->scy = memRead(mem, 0xFF42);
    ppu->scx = memRead(mem, 0xFF43);
    ppu->ly  = memRead(mem, 0xFF44); 
    if((ppu->lcdc & BIT(7)) == 0)
        return;
    if(ppu->ly < RES_Y){
        uint scanline_cycles = ppu->cycles % SCANLINE_MAX_CYCLES;
        if(scanline_cycles >= 80){
            if(ppu->fetcher.x < RES_X){
                if(ppu->lcdc & BIT(0)){
                    updateBG(ppu, mem);
                }
            }else{
                if(scanline_cycles + 1 >= SCANLINE_MAX_CYCLES){
                    ppu->fetcher.x = 0;
                    ++ppu->ly;
                }
            }
        } 
        ++ppu->cycles;
    } else{
        ++ppu->cycles;
        if(ppu->cycles >= FRAME_MAX_CYCLES){
            ppu->ly = 0;
            ppu->cycles = 0;
            updateMainWindow(ppu->pixels, RES_X);
            updateSubwindows();
        }
    }
    memWrite(mem, 0xFF40, ppu->lcdc);
    memWrite(mem, 0xFF42, ppu->scy);
    memWrite(mem, 0xFF43, ppu->scx);
    memWrite(mem, 0xFF44, ppu->ly); 
}