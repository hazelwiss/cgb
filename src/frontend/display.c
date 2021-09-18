#include"display.h"
#include<backend/cpu.h>

#define DEFAULT_R 0xFF
#define DEFAULT_G 0xFF
#define DEFAULT_B 0xFF

struct{
    CPU* reference;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    struct {
        SDL_Window* bg_map_w;
        SDL_Renderer* bg_map_r;
        SDL_Texture* bg_map_t;
        SDL_Window* tile_w;
        SDL_Renderer* tile_r;
        SDL_Texture* tile_t;
    } subwindows;
} display;

bool render_bg_map      = false;
bool render_tile_map    = false;
const size_t BG_MAP_WIDTH       = 256+32;
const size_t BG_MAP_HEIGHT      = 256+32;
const size_t TILES_PER_LINE     = 16;
const size_t TILE_LINE_COUNT    = 128/TILES_PER_LINE;
const size_t TILE_MAP_WIDTH     = (TILES_PER_LINE)*8 + TILES_PER_LINE;
const size_t TILE_MAP_HEIGHT    = TILE_LINE_COUNT*8 + TILE_LINE_COUNT;

void initDisplay(CPU* cpu, const char* name, size_t width, size_t height){
    SDL_CreateWindowAndRenderer(160, 144, 0, &display.window, &display.renderer);
    display.texture = SDL_CreateTexture(display.renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, width, height);
    SDL_SetRenderDrawColor(display.renderer, DEFAULT_R, DEFAULT_G, DEFAULT_B, 0xFF);
    SDL_RenderClear(display.renderer);
    SDL_RenderPresent(display.renderer);
    display.reference = cpu;
    //  init background map
    SDL_CreateWindowAndRenderer(BG_MAP_WIDTH*2, BG_MAP_HEIGHT*2, 0, &display.subwindows.bg_map_w, &display.subwindows.bg_map_r);
    SDL_SetWindowTitle(display.subwindows.bg_map_w, "background map viewer");
    display.subwindows.bg_map_t = SDL_CreateTexture(display.subwindows.bg_map_r, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, 
        BG_MAP_WIDTH, BG_MAP_HEIGHT);
    SDL_SetRenderDrawColor(display.subwindows.bg_map_r, DEFAULT_R, DEFAULT_G, DEFAULT_B, 0xFF);
    SDL_RenderClear(display.subwindows.bg_map_r);
    SDL_RenderPresent(display.subwindows.bg_map_r);
    //  init tile map
    SDL_CreateWindowAndRenderer(TILE_MAP_WIDTH*3, TILE_MAP_HEIGHT*3, 0, &display.subwindows.tile_w, &display.subwindows.tile_r);
    SDL_SetWindowTitle(display.subwindows.tile_w, "tile map viewer");
    display.subwindows.tile_t = SDL_CreateTexture(display.subwindows.tile_r, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET,
        TILE_MAP_WIDTH, TILE_MAP_HEIGHT);
    SDL_SetRenderDrawColor(display.subwindows.tile_r, DEFAULT_R, DEFAULT_G, DEFAULT_B, 0xFF);
    SDL_RenderClear(display.subwindows.tile_r);
    SDL_RenderPresent(display.subwindows.tile_r);
}

void updateMainWindow(const void* pixeldata, size_t row_length){
    SDL_RenderClear(display.renderer);
    SDL_UpdateTexture(display.texture, NULL, pixeldata, row_length*4);
    SDL_RenderCopy(display.renderer, display.texture, NULL, NULL);
    SDL_RenderPresent(display.renderer);
}

static void renderBGMapWindow(void){
    uint32_t data[BG_MAP_HEIGHT][BG_MAP_WIDTH];
    size_t begin;
    size_t end;
    if(display.reference->ppu.lcdc & BIT(3)){
        begin   = 0x9C00;
        end     = 0xA000;
    } else{
        begin   = 0x9800;
        end     = 0x9C00;
    }
    for(size_t i = begin; i < end; ++i){
        size_t x = ((i-begin)*9) % (BG_MAP_WIDTH);
        size_t y = ((i-begin)/32)*9;
        size_t tile = memRead(&display.reference->memory, i);
        for(size_t height = 0; height < 8; ++height){
            for(size_t row = 0; row < 8; ++row){
                uint8_t upper_byte; 
                uint8_t lower_byte;                
                if(display.reference->ppu.lcdc & BIT(4)){
                    upper_byte = memRead(&display.reference->memory, 0x8000+tile*16+height*2+1);
                    lower_byte = memRead(&display.reference->memory, 0x8000+tile*16+height*2);
                } else{
                    upper_byte = memRead(&display.reference->memory, 0x9000+((int8_t)tile)*16+height*2+1);
                    lower_byte = memRead(&display.reference->memory, 0x9000+((int8_t)tile)*16+height*2);
                }
                bool low    = (lower_byte & (BIT(7) >> (row%8))); 
                bool high   = (upper_byte & (BIT(7) >> (row%8)));
                size_t pixel_value = low | (high << 1);
                switch (pixel_value){
                case 0: data[y + height][x + row] = 0xFFFFFF;  break;
                case 1: data[y + height][x + row] = 0x999999;  break;
                case 2: data[y + height][x + row] = 0x444444;  break;
                case 3: data[y + height][x + row] = 0x000000;  break;
                default:
                    PANIC;
                }
            }
            data[y + height][x + 8] = 0x000000;
        }
        for(size_t j = 0; j < 9; ++j){
            data[y + 8][x + j] = 0x000000;
        }
    }
    SDL_RenderClear(display.subwindows.bg_map_r);
    SDL_UpdateTexture(display.subwindows.bg_map_t, NULL, data, (sizeof(*data)));
    SDL_RenderCopy(display.subwindows.bg_map_r, display.subwindows.bg_map_t, NULL, NULL);
    SDL_RenderPresent(display.subwindows.bg_map_r); 
}

static void renderTileMapWindow(){
    uint32_t data[TILE_MAP_HEIGHT][TILE_MAP_WIDTH];
    for(size_t tile = 0; tile < 128; ++tile){
        size_t x = (tile%TILES_PER_LINE)*9;
        size_t y = (tile/TILES_PER_LINE)*9;
        uint8_t datalow[8];
        uint8_t datahigh[8];
        size_t adr = tile*16 + 0x8000;
        for(size_t i = 0; i < 8; ++i){
            datalow[i]      = memRead(&display.reference->memory, adr+i*2);
            datahigh[i]     = memRead(&display.reference->memory, adr+i*2+1);
        }
        for(size_t height = 0; height < 8; ++height){
            for(size_t row = 0; row < 8; ++row){
                uint8_t upper_byte = datalow[height];
                uint8_t lower_byte = datahigh[height];
                bool low    = (lower_byte & (BIT(7) >> (row%8))); 
                bool high   = (upper_byte & (BIT(7) >> (row%8)));
                size_t pixel_value = low | (high << 1);
                switch (pixel_value){
                case 0: data[y + height][x + row] = 0xFFFFFF;  break;
                case 1: data[y + height][x + row] = 0x999999;  break;
                case 2: data[y + height][x + row] = 0x444444;  break;
                case 3: data[y + height][x + row] = 0x000000;  break;
                default:
                    PANIC;
                }
            }
            data[y + height][x + 8] = 0x000000;
        } 
        for(size_t j = 0; j < 9; ++j){
            data[y + 8][x + j] = 0x000000;
        }
    }
    SDL_RenderClear(display.subwindows.tile_r);
    SDL_UpdateTexture(display.subwindows.tile_t, NULL, data, (sizeof(*data)));
    SDL_RenderCopy(display.subwindows.tile_r, display.subwindows.tile_t, NULL, NULL);
    SDL_RenderPresent(display.subwindows.tile_r); 
}

void updateSubwindows(void){
    if(render_bg_map)
        renderBGMapWindow();
    if(render_tile_map)
        renderTileMapWindow();
}

void setRenderTileMap(bool b){
    render_tile_map = b;
}

void setRenderBGMap(bool b){
    render_bg_map = b;
}