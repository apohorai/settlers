#include <stdint.h>
#include <string.h>
#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// --- Hardware Memory Definitions ---
#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

// --- Configuration ---
#define TEMP_START 200      // Indices 200-255 for software-sprites
#define TEMP_COUNT 56

// --- Software Sprite State ---
static uint8_t temp_slots_used = 0;

typedef struct {
    uint8_t x, y;          // Grid position (0-39, 0-24)
    uint8_t off_x, off_y;  // Pixel offset (0-7)
    uint8_t char_index;    // Source character index
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

uint8_t allocate_temp_char() {
    if (temp_slots_used < TEMP_COUNT) {
        return TEMP_START + (temp_slots_used++);
    }
    return 0; 
}

void reset_temp_pool() {
    temp_slots_used = 0;
}

void draw_merged_tile(uint8_t x, uint8_t y, uint8_t temp_idx) {
    if (x >= 40 || y >= 25) return;
    
    uint16_t offset = (y * 40) + x;
    uint8_t bg_char = settlers_map[offset]; 
    
    uint8_t* bg_ptr = CHARSET_DEST + (bg_char << 3);
    uint8_t* ov_ptr = CHARSET_DEST + (temp_idx << 3);

    for (uint8_t i = 0; i < 8; i++) {
        ov_ptr[i] |= bg_ptr[i]; 
    }

    SCREEN_RAM[offset] = temp_idx;
    COLOR_RAM[offset] = 1; 
}

void draw_settler_smooth(Settler* s) {
    uint8_t t[4];
    for (uint8_t i = 0; i < 4; i++) {
        t[i] = allocate_temp_char();
        if (!t[i]) return;
        memset(CHARSET_DEST + (t[i] << 3), 0, 8); 
    }

    uint8_t* src_gfx = CHARSET_DEST + (s->char_index << 3);

    for (uint8_t i = 0; i < 8; i++) {
        uint16_t row = (uint16_t)src_gfx[i] << (8 - s->off_x);
        uint8_t left_bits = (uint8_t)(row >> 8);
        uint8_t right_bits = (uint8_t)(row & 0xFF);

        uint8_t target_y = i + s->off_y;
        if (target_y < 8) {
            (CHARSET_DEST + (t[0] << 3))[target_y] = left_bits;
            (CHARSET_DEST + (t[1] << 3))[target_y] = right_bits;
        } else {
            (CHARSET_DEST + (t[2] << 3))[target_y - 8] = left_bits;
            (CHARSET_DEST + (t[3] << 3))[target_y - 8] = right_bits;
        }
    }

    draw_merged_tile(s->x,     s->y,     t[0]);
    draw_merged_tile(s->x + 1, s->y,     t[1]);
    draw_merged_tile(s->x,     s->y + 1, t[2]);
    draw_merged_tile(s->x + 1, s->y + 1, t[3]);
}

void move_settler(Settler* s, int8_t dx, int8_t dy) {
    int16_t px = (s->x * 8) + s->off_x + dx;
    int16_t py = (s->y * 8) + s->off_y + dy;
    
    if (px < 0) px = 0; if (px > 310) px = 310;
    if (py < 0) py = 0; if (py > 190) py = 190;

    s->x = px / 8;
    s->off_x = px % 8;
    s->y = py / 8;
    s->off_y = py % 8;
}

// --- Keyboard Input ---

void readKeys(Settler* npc) {
    // Select CIA1 Row 1
    (*(volatile uint8_t*)0xDC00) = 0xFD; 
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);

    // W: Up
    if (!(row1 & 0x02)) {
        // moveSprite(0, -1);
        move_settler(npc, 0, -1); 
    }
    
    // A: Left
    if (!(row1 & 0x04)) {
        // moveSprite(-1, 0);
        move_settler(npc, -1, 0); 
    }
    
    // S: Down
    if (!(row1 & 0x20)) {
        // moveSprite(0, 1);
        move_settler(npc, 0, 1);
    }

    // Select CIA1 Row 2
    (*(volatile uint8_t*)0xDC00) = 0xFB; 
    uint8_t row2 = (*(volatile uint8_t*)0xDC01);
    
    // D: Right
    if (!(row2 & 0x04)) {
        // moveSprite(1, 0);  
        move_settler(npc, 1, 0); 
    }
}

// --- Main Loop ---

int main(void) {
    init_system();
    // initSprite0();

    Settler npc = {10, 10, 0, 0, 48}; 

    while (1) {
        wait_vsync();

        // 1. CLEANUP: Redraw 4x4 area around NPC (increased from 3x3 for safety)
        for (uint8_t j = 0; j < 4; j++) {
            for (uint8_t i = 0; i < 4; i++) {
                uint8_t tx = npc.x + i;
                uint8_t ty = npc.y + j;
                if (tx < 40 && ty < 25) {
                    uint16_t off = (ty * 40) + tx;
                    SCREEN_RAM[off] = settlers_map[off];
                }
            }
        }

        reset_temp_pool();
        
        // 2. INPUT & LOGIC
        readKeys(&npc);             

        // 3. DRAW
        draw_settler_smooth(&npc);
    }
    return 0;
}