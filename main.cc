#include <stdint.h>
#include <string.h>
#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// --- Hardware Memory Definitions ---
#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

// --- Hardcoded Temporary Characters ---
#define TEMP_A 254 // $FE
#define TEMP_B 255 // $FF

typedef struct {
    uint8_t x, y;          
    uint8_t off_x, off_y;  
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

// --- Software Sprite Engine ---

void draw_merged_tile(uint8_t x, uint8_t y, uint8_t temp_idx, uint8_t color) {
    if (x >= 40 || y >= 25) return;
    uint16_t offset = (y * 40) + x;
    uint8_t bg_char = settlers_map[offset]; 
    uint8_t* bg_ptr = CHARSET_DEST + (bg_char << 3);
    uint8_t* ov_ptr = CHARSET_DEST + (temp_idx << 3);

    for (uint8_t i = 0; i < 8; i++) {
        ov_ptr[i] |= bg_ptr[i]; 
    }
    SCREEN_RAM[offset] = temp_idx;
    COLOR_RAM[offset] = color; 
}

// Now accepts the specific temp characters to use
void draw_settler(Settler* s, uint8_t t1, uint8_t t2) {
    memset(CHARSET_DEST + (t1 << 3), 0, 8);
    memset(CHARSET_DEST + (t2 << 3), 0, 8);

    uint8_t* src_gfx = CHARSET_DEST + (s->char_index << 3);

    for (uint8_t i = 0; i < 8; i++) {
        if (s->off_x > 0) {
            uint16_t row = (uint16_t)src_gfx[i] << (8 - s->off_x);
            (CHARSET_DEST + (t1 << 3))[i] = (uint8_t)(row >> 8);
            (CHARSET_DEST + (t2 << 3))[i] = (uint8_t)(row & 0xFF);
        } else {
            uint8_t target_y = i + s->off_y;
            if (target_y < 8) {
                (CHARSET_DEST + (t1 << 3))[target_y] = src_gfx[i];
            } else {
                (CHARSET_DEST + (t2 << 3))[target_y - 8] = src_gfx[i];
            }
        }
    }

    draw_merged_tile(s->x, s->y, t1, s->color);
    if (s->off_x > 0) draw_merged_tile(s->x + 1, s->y, t2, s->color);
    if (s->off_y > 0) draw_merged_tile(s->x, s->y + 1, t2, s->color);
}

// --- Full Movement Logic ---

// Wrapper that sets up the movement state
void start_move(Settler* s, int8_t dx, int8_t dy) {
    if (s->steps_remaining == 0) { 
        s->dir_x = dx;
        s->dir_y = dy;
        s->steps_remaining = 8; 
    }
}

// Move functions now define which temp characters to pass to the logic
void move_up(Settler* s)    { start_move(s,  0, -1); }
void move_down(Settler* s)  { start_move(s,  0,  1); }
void move_left(Settler* s)  { start_move(s, -1,  0); }
void move_right(Settler* s) { start_move(s,  1,  0); }

void update_settler(Settler* s) {
    if (s->steps_remaining > 0) {
        int16_t px = (s->x * 8) + s->off_x + s->dir_x;
        int16_t py = (s->y * 8) + s->off_y + s->dir_y;

        s->x = px / 8;
        s->off_x = px % 8;
        s->y = py / 8;
        s->off_y = py % 8;

        s->steps_remaining--;
    }
}

void handle_input(Settler* s) {
    if (s->steps_remaining > 0) return; 

    (*(volatile uint8_t*)0xDC00) = 0xFD; 
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);

    if (!(row1 & 0x02))      move_up(s);
    else if (!(row1 & 0x20)) move_down(s);
    else if (!(row1 & 0x04)) move_left(s);
    else {
        (*(volatile uint8_t*)0xDC00) = 0xFB; 
        uint8_t row2 = (*(volatile uint8_t*)0xDC01);
        if (!(row2 & 0x04))  move_right(s);
    }
}

// --- Main Loop ---

int main(void) {
    init_system();
    Settler npc = {10, 10, 0, 0, 48, 1, 0, 0, 0}; 

    while (1) {
        wait_vsync();

        // 1. SURGICAL CLEANUP
        uint16_t off = (npc.y * 40) + npc.x;
        SCREEN_RAM[off] = settlers_map[off];
        SCREEN_RAM[off + 1] = settlers_map[off + 1];
        SCREEN_RAM[off + 40] = settlers_map[off + 40];
        SCREEN_RAM[off + 41] = settlers_map[off + 41];

        // 2. LOGIC
        handle_input(&npc);             
        update_settler(&npc);  

        // 3. DRAW - Passing the desired temp characters here
        draw_settler(&npc, TEMP_A, TEMP_B);
    }
    return 0;
}