#include <stdint.h>
#include <string.h>
#include <stdlib.h> // For rand()

#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

#define POOL_START 150
#define POOL_END   255
#define POOL_SIZE  (POOL_END - POOL_START + 1)

typedef struct {
    uint8_t stack[POOL_SIZE];
    int16_t top;
} TempPool;

typedef struct {
    uint8_t x, y;          
    uint8_t off_x, off_y;  
    uint8_t old_x, old_y;  
    uint8_t char_index;    
    uint8_t color;
    int8_t  dir_x, dir_y;    
    uint8_t steps_remaining; 
    uint8_t t1, t2; 
} Settler;

static TempPool charPool;

// --- Pool Management ---

void init_pool() {
    charPool.top = -1;
    for (int16_t i = POOL_START; i <= POOL_END; i++) {
        charPool.stack[++charPool.top] = (uint8_t)i;
    }
}

uint8_t assign_char() {
    if (charPool.top >= 0) return charPool.stack[charPool.top--];
    return 0; 
}

// --- System Functions ---

void wait_vsync() {
    while ((*(volatile uint8_t*)0xd011) & 0x80); 
    while ((*(volatile uint8_t*)0xd012) != 0xFF);
}

// Updated init_system with path coloring logic
void init_system() {
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    (*(volatile uint8_t*)0xd018) = 0x1C; 
    (*(volatile uint8_t*)0xd020) = 0;    
    (*(volatile uint8_t*)0xd021) = 6;    
    
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
    
    // Initial color render and path detection
    for (uint16_t i = 0; i < 1000; i++) {
        uint8_t tile = settlers_map[i];
        // Hardcoded path characters: 0, 1, 2, 3, 4, 5, 6
        if (tile <= 6) {
            COLOR_RAM[i] = 2; // Red
        } else {
            COLOR_RAM[i] = 1; // White (default)
        }
    }
}

// --- Drawing Engine ---

void prepare_temp(uint8_t x, uint8_t y, uint8_t t_idx) {
    if (x >= 40 || y >= 25 || t_idx == 0) return;
    uint16_t offset = (y * 40) + x;
    uint8_t bg_char = settlers_map[offset];
    memcpy(CHARSET_DEST + (t_idx << 3), CHARSET_DEST + (bg_char << 3), 8);
    SCREEN_RAM[offset] = t_idx;
    
    // Note: color is handled by the settler's own color during move
    COLOR_RAM[offset] = 1; // You can change this to match settler color if needed
}

void draw_settler(Settler* s) {
    if (s->t1 == 0) s->t1 = assign_char();
    if (s->t2 == 0) s->t2 = assign_char();

    uint8_t* src = CHARSET_DEST + (s->char_index << 3);
    uint8_t* dst1 = CHARSET_DEST + (s->t1 << 3);
    uint8_t* dst2 = CHARSET_DEST + (s->t2 << 3);

    if (s->off_x > 0) {
        prepare_temp(s->x, s->y, s->t1);
        prepare_temp(s->x + 1, s->y, s->t2);
        for (uint8_t i = 0; i < 8; i++) {
            uint16_t row = (uint16_t)src[i] << (8 - s->off_x);
            dst1[i] |= (uint8_t)(row >> 8);
            dst2[i] |= (uint8_t)(row & 0xFF);
        }
        COLOR_RAM[(s->y * 40) + s->x] = s->color;
        COLOR_RAM[(s->y * 40) + s->x + 1] = s->color;
    } 
    else if (s->off_y > 0) {
        prepare_temp(s->x, s->y, s->t1);
        prepare_temp(s->x, s->y + 1, s->t2);
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t target_y = i + s->off_y;
            if (target_y < 8) dst1[target_y] |= src[i];
            else             dst2[target_y - 8] |= src[i];
        }
        COLOR_RAM[(s->y * 40) + s->x] = s->color;
        COLOR_RAM[((s->y + 1) * 40) + s->x] = s->color;
    }
    else {
        prepare_temp(s->x, s->y, s->t1);
        for (uint8_t i = 0; i < 8; i++) dst1[i] |= src[i];
        COLOR_RAM[(s->y * 40) + s->x] = s->color;
    }
}

// --- Movement Logic ---

void start_move(Settler* s, int8_t dx, int8_t dy) {
    if (s->steps_remaining == 0) { 
        uint8_t nx = s->x + dx;
        uint8_t ny = s->y + dy;
        if (nx >= 39 || ny >= 24) return;

        s->old_x = s->x; s->old_y = s->y;
        s->dir_x = dx;   s->dir_y = dy;
        s->steps_remaining = 8; 
    }
}

// Helper to restore background tile and its path color
void restore_tile(uint8_t x, uint8_t y) {
    uint16_t offset = (y * 40) + x;
    uint8_t tile = settlers_map[offset];
    SCREEN_RAM[offset] = tile;
    if (tile <= 6) {
        COLOR_RAM[offset] = 2; // Red path
    } else {
        COLOR_RAM[offset] = 1; // White background
    }
}

void update_settler(Settler* s) {
    if (s->steps_remaining > 0) {
        int16_t px = (s->x * 8) + s->off_x + s->dir_x;
        int16_t py = (s->y * 8) + s->off_y + s->dir_y;
        s->x = px / 8; s->off_x = px % 8;
        s->y = py / 8; s->off_y = py % 8;
        s->steps_remaining--;

        if (s->steps_remaining == 0) {
            restore_tile(s->old_x, s->old_y);
            if (s->dir_x != 0) restore_tile(s->old_x + 1, s->old_y);
            if (s->dir_y != 0) restore_tile(s->old_x, s->old_y + 1);
            s->dir_x = 0; s->dir_y = 0;
        }
    }
}

// --- Input & AI ---

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

void handle_ai(Settler* s) {
    if (s->steps_remaining > 0) return;
    if ((rand() % 100) < 5) {
        int8_t r = rand() % 4;
        if (r == 0)      start_move(s, 0, -1);
        else if (r == 1) start_move(s, 0, 1);
        else if (r == 2) start_move(s, -1, 0);
        else             start_move(s, 1, 0);
    }
}

// --- Main ---

int main(void) {
    init_pool();
    init_system();

    Settler npc[3];
    npc[0] = {10, 10, 0, 0, 10, 10, 48, 1, 0, 0, 0, 0, 0}; 
    npc[1] = {20, 15, 0, 0, 20, 15, 48, 1, 0, 0, 0, 0, 0}; 
    npc[2] = {20, 18, 0, 0, 20, 18, 48, 1, 0, 0, 0, 0, 0}; 

    while (1) {
        wait_vsync();
        
        handle_input(&npc[0]);             
        for(uint8_t i = 0; i < 3; i++) {
            if(i > 0) handle_ai(&npc[i]);
            update_settler(&npc[i]);  
            draw_settler(&npc[i]);
        }
    }
    return 0;
}