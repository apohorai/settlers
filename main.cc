#include <stdint.h>
#include <string.h>
#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

#define TEMP_A    254 // $FE
#define TEMP_B    255 // $FF

typedef struct {
    uint8_t x, y;          
    uint8_t off_x, off_y;  
    uint8_t old_x, old_y;  
    uint8_t char_index;    
    uint8_t color;
    int8_t  dir_x, dir_y;    
    uint8_t steps_remaining; 
} Settler;

// --- System Functions ---

void wait_vsync() {
    while ((*(volatile uint8_t*)0xd011) & 0x80); 
    while ((*(volatile uint8_t*)0xd012) != 0xFF);
}

void init_system() {
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    (*(volatile uint8_t*)0xd018) = 0x1C; 
    (*(volatile uint8_t*)0xd020) = 0;    
    (*(volatile uint8_t*)0xd021) = 6;    
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
    memset((void*)COLOR_RAM, 1, 1000); 
}

// --- Helper: Pre-fills a temp char with a clean background tile ---
void prepare_temp(uint8_t x, uint8_t y, uint8_t t_idx) {
    if (x >= 40 || y >= 25) return;
    uint8_t bg_char = settlers_map[(y * 40) + x];
    memcpy(CHARSET_DEST + (t_idx << 3), CHARSET_DEST + (bg_char << 3), 8);
    SCREEN_RAM[(y * 40) + x] = t_idx;
}

// --- Main Drawing Engine ---
void draw_settler(Settler* s, uint8_t t1, uint8_t t2) {
    uint8_t* src = CHARSET_DEST + (s->char_index << 3);
    uint8_t* dst1 = CHARSET_DEST + (t1 << 3);
    uint8_t* dst2 = CHARSET_DEST + (t2 << 3);

    // CASE 1: HORIZONTAL MOVEMENT
    if (s->off_x > 0) {
        prepare_temp(s->x, s->y, t1);
        prepare_temp(s->x + 1, s->y, t2);
        for (uint8_t i = 0; i < 8; i++) {
            uint16_t row = (uint16_t)src[i] << (8 - s->off_x);
            dst1[i] |= (uint8_t)(row >> 8);
            dst2[i] |= (uint8_t)(row & 0xFF);
        }
    } 
    // CASE 2: VERTICAL MOVEMENT
    else if (s->off_y > 0) {
        prepare_temp(s->x, s->y, t1);
        prepare_temp(s->x, s->y + 1, t2);
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t target_y = i + s->off_y;
            if (target_y < 8) dst1[target_y] |= src[i];
            else             dst2[target_y - 8] |= src[i];
        }
    }
    // CASE 3: IDLE / SNAPPED TO GRID
    else {
        prepare_temp(s->x, s->y, t1);
        for (uint8_t i = 0; i < 8; i++) {
            dst1[i] |= src[i];
        }
    }
}

void update_settler(Settler* s) {
    if (s->steps_remaining > 0) {
        int16_t px = (s->x * 8) + s->off_x + s->dir_x;
        int16_t py = (s->y * 8) + s->off_y + s->dir_y;

        s->x = px / 8;     s->off_x = px % 8;
        s->y = py / 8;     s->off_y = py % 8;

        s->steps_remaining--;

        if (s->steps_remaining == 0) {
            // Restore background to vacated tiles
            uint16_t old_off = (s->old_y * 40) + s->old_x;
            SCREEN_RAM[old_off] = settlers_map[old_off];
            if (s->dir_x != 0) SCREEN_RAM[old_off + 1] = settlers_map[old_off + 1];
            if (s->dir_y != 0) SCREEN_RAM[old_off + 40] = settlers_map[old_off + 40];

            s->dir_x = 0; s->dir_y = 0;
        }
    }
}

void start_move(Settler* s, int8_t dx, int8_t dy) {
    if (s->steps_remaining == 0) { 
        s->old_x = s->x; s->old_y = s->y;
        s->dir_x = dx;   s->dir_y = dy;
        s->steps_remaining = 8; 
    }
}

void handle_input(Settler* s) {
    if (s->steps_remaining > 0) return; 
    (*(volatile uint8_t*)0xDC00) = 0xFD; 
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);
    if (!(row1 & 0x02))      start_move(s, 0, -1);
    else if (!(row1 & 0x20)) start_move(s, 0, 1);
    else if (!(row1 & 0x04)) start_move(s, -1, 0);
    else {
        (*(volatile uint8_t*)0xDC00) = 0xFB; 
        if (!((*(volatile uint8_t*)0xDC01) & 0x04)) start_move(s, 1, 0);
    }
}

int main(void) {
    init_system();
    Settler npc = {10, 10, 0, 0, 10, 10, 48, 1, 0, 0, 0}; 

    while (1) {
        wait_vsync();
        handle_input(&npc);             
        update_settler(&npc);  
        draw_settler(&npc, TEMP_A, TEMP_B);
    }
    return 0;
}