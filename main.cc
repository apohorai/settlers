#include <string.h>
#include <stdlib.h>
#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// ============================================================================
// HARDWARE ADDRESSES
// ============================================================================
#define SCREEN_RAM      ((uint8_t*)0x0400)  // VIC-II screen memory (40x25)
#define COLOR_RAM       ((uint8_t*)0xD800)  // VIC-II color memory
#define CHARSET_DEST    ((uint8_t*)0x3000)  // Our custom character set location

// ============================================================================
// TEMPORARY CHARACTER POOL
// ============================================================================
#define TEMP_POOL_START 128  // Start of temp character pool
#define TEMP_POOL_SIZE  128  // 128 characters (128-255)
#define MAX_NPCS        20    // Maximum number of NPCs

// Bitmap tracking which temp chars are in use (128 bits = 16 bytes)
static uint8_t temp_used[16];  // 16 bytes * 8 bits = 128 bits

// Current allocations for NPCs
static uint8_t npc_temp1[MAX_NPCS];  // First temp char for each NPC
static uint8_t npc_temp2[MAX_NPCS];  // Second temp char for each NPC

// ============================================================================
// SETTLER STRUCTURE
// ============================================================================
typedef struct {
    // Position in tile coordinates (0-39, 0-24)
    uint8_t x, y;
    
    // Sub-pixel offset (0-7) for smooth movement
    uint8_t off_x, off_y;
    
    // Previous position (for cleanup after movement)
    uint8_t old_x, old_y;
    
    // Visual appearance
    uint8_t char_index;  // Base character from charset (usually 48)
    uint8_t color;       // C64 color (0-15)
    
    // Movement state
    int8_t  dir_x, dir_y;     // Direction vector (-1,0,1)
    uint8_t steps_remaining;   // 8 = moving, 0 = stationary
    
    // NPC ID for temp character lookup
    uint8_t npc_id;
} Settler;

// ============================================================================
// TEMPORARY CHARACTER ALLOCATOR FUNCTIONS
// ============================================================================

/**
 * Initialize the temp character allocator
 * Call once at startup
 */
void init_temp_allocator() {
    // Clear the bitmap (all chars free)
    memset(temp_used, 0, 16);
    
    // Initialize NPC allocations to 0 (unused)
    memset(npc_temp1, 0, MAX_NPCS);
    memset(npc_temp2, 0, MAX_NPCS);
}

/**
 * Allocate a temporary character
 * Returns: character index (128-255) or 0 if none available
 */
uint8_t allocate_temp() {
    // Scan bitmap for a free slot
    for (uint8_t byte = 0; byte < 16; byte++) {
        if (temp_used[byte] != 0xFF) {  // If byte has a free bit
            // Find first free bit in this byte
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (!(temp_used[byte] & (1 << bit))) {
                    // Mark as used
                    temp_used[byte] |= (1 << bit);
                    // Return character index
                    return TEMP_POOL_START + (byte * 8) + bit;
                }
            }
        }
    }
    return 0;  // No free temp chars
}

/**
 * Release a temporary character back to the pool
 * @param c Character index to release (128-255)
 */
void release_temp(uint8_t c) {
    if (c < TEMP_POOL_START || c >= TEMP_POOL_START + TEMP_POOL_SIZE) 
        return;
    
    uint8_t index = c - TEMP_POOL_START;
    uint8_t byte = index / 8;
    uint8_t bit = index % 8;
    
    // Clear the bit (mark as free)
    temp_used[byte] &= ~(1 << bit);
    
    // Clear the character bitmap to prevent garbage
    memset(CHARSET_DEST + (c << 3), 0, 8);
}

/**
 * Allocate temp chars for an NPC
 * @param npc_id NPC index (0-19)
 * @return 1 if successful, 0 if failed
 */
uint8_t allocate_npc_temps(uint8_t npc_id) {
    if (npc_id >= MAX_NPCS) return 0;
    
    // If NPC already has temps, release them first
    if (npc_temp1[npc_id] != 0) 
        release_temp(npc_temp1[npc_id]);
    if (npc_temp2[npc_id] != 0) 
        release_temp(npc_temp2[npc_id]);
    
    // Allocate two new temp chars
    npc_temp1[npc_id] = allocate_temp();
    npc_temp2[npc_id] = allocate_temp();
    
    // Check if both allocations succeeded
    if (npc_temp1[npc_id] == 0 || npc_temp2[npc_id] == 0) {
        // If either failed, release any that succeeded
        if (npc_temp1[npc_id] != 0) release_temp(npc_temp1[npc_id]);
        if (npc_temp2[npc_id] != 0) release_temp(npc_temp2[npc_id]);
        npc_temp1[npc_id] = 0;
        npc_temp2[npc_id] = 0;
        return 0;
    }
    
    return 1;
}

/**
 * Release both temp chars for an NPC
 */
void release_npc_temps(uint8_t npc_id) {
    if (npc_id >= MAX_NPCS) return;
    
    if (npc_temp1[npc_id] != 0) {
        release_temp(npc_temp1[npc_id]);
        npc_temp1[npc_id] = 0;
    }
    if (npc_temp2[npc_id] != 0) {
        release_temp(npc_temp2[npc_id]);
        npc_temp2[npc_id] = 0;
    }
}

/**
 * Get NPC's temp chars for drawing
 */
void get_npc_temps(uint8_t npc_id, uint8_t* t1, uint8_t* t2) {
    if (npc_id >= MAX_NPCS) {
        *t1 = 0;
        *t2 = 0;
        return;
    }
    *t1 = npc_temp1[npc_id];
    *t2 = npc_temp2[npc_id];
}

// ============================================================================
// DRAW INTEGER FUNCTION
// ============================================================================

/**
 * Draw integer with fixed width (leading zeros)
 */
void drawInt(uint16_t value, uint8_t width, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t digits[5];
    uint16_t temp = value;
    uint16_t offset;
    
    for (uint8_t i = 0; i < width; i++) {
        uint8_t digit = temp % 10;
        if (digit == 0) {
            digits[width - 1 - i] = 0x79;  // '0'
        } else {
            digits[width - 1 - i] = 0x70 + (digit - 1);  // '1' through '9'
        }
        temp = temp / 10;
    }
    
    for (uint8_t i = 0; i < width; i++) {
        offset = (y * 40) + x + i;
        SCREEN_RAM[offset] = digits[i];
        COLOR_RAM[offset] = color;
    }
}

// ============================================================================
// SYSTEM FUNCTIONS
// ============================================================================

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

// ============================================================================
// RENDERING HELPERS
// ============================================================================

void prepare_temp(uint8_t x, uint8_t y, uint8_t t_idx) {
    if (x >= 40 || y >= 25) return;
    uint8_t bg_char = settlers_map[(y * 40) + x];
    memcpy(CHARSET_DEST + (t_idx << 3), CHARSET_DEST + (bg_char << 3), 8);
    SCREEN_RAM[(y * 40) + x] = t_idx;
}

// ============================================================================
// MAIN DRAWING ENGINE
// ============================================================================

void draw_settler(Settler* s, uint8_t t1, uint8_t t2) {
    uint8_t* src = CHARSET_DEST + (s->char_index << 3);
    uint8_t* dst1 = CHARSET_DEST + (t1 << 3);
    uint8_t* dst2 = CHARSET_DEST + (t2 << 3);

    if (s->off_x > 0) {
        prepare_temp(s->x, s->y, t1);
        prepare_temp(s->x + 1, s->y, t2);
        
        for (uint8_t i = 0; i < 8; i++) {
            uint16_t row = (uint16_t)src[i] << (8 - s->off_x);
            dst1[i] |= (uint8_t)(row >> 8);
            dst2[i] |= (uint8_t)(row & 0xFF);
        }
    } 
    else if (s->off_y > 0) {
        prepare_temp(s->x, s->y, t1);
        prepare_temp(s->x, s->y + 1, t2);
        
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t target_y = i + s->off_y;
            if (target_y < 8) {
                dst1[target_y] |= src[i];
            } else {
                dst2[target_y - 8] |= src[i];
            }
        }
    } 
    else {
        prepare_temp(s->x, s->y, t1);
        for (uint8_t i = 0; i < 8; i++) {
            dst1[i] |= src[i];
        }
    }
    
    COLOR_RAM[(s->y * 40) + s->x] = s->color;
}

// ============================================================================
// MOVEMENT UPDATE
// ============================================================================

void update_settler(Settler* s) {
    if (s->steps_remaining > 0) {
        int16_t px = (s->x * 8) + s->off_x + s->dir_x;
        int16_t py = (s->y * 8) + s->off_y + s->dir_y;

        s->x = px / 8;     s->off_x = px % 8;
        s->y = py / 8;     s->off_y = py % 8;

        s->steps_remaining--;

        if (s->steps_remaining == 0) {
            uint16_t old_off = (s->old_y * 40) + s->old_x;
            SCREEN_RAM[old_off] = settlers_map[old_off];
            
            if (s->dir_x != 0) 
                SCREEN_RAM[old_off + 1] = settlers_map[old_off + 1];
            
            if (s->dir_y != 0) 
                SCREEN_RAM[old_off + 40] = settlers_map[old_off + 40];

            s->dir_x = 0; s->dir_y = 0;
        }
    }
}

// ============================================================================
// MOVEMENT INITIATION
// ============================================================================

void start_move(Settler* s, int8_t dx, int8_t dy) {
    if (s->steps_remaining == 0) { 
        s->old_x = s->x; s->old_y = s->y;
        s->dir_x = dx;   s->dir_y = dy;
        s->steps_remaining = 8; 
    }
}

// ============================================================================
// INPUT HANDLING - Modified to work with array of NPCs
// ============================================================================

void handle_input(Settler* npcs, uint8_t count) {
    // Check if any NPC can move
    uint8_t can_move = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (npcs[i].steps_remaining == 0) {
            can_move = 1;
            break;
        }
    }
    if (!can_move) return;
    
    (*(volatile uint8_t*)0xDC00) = 0xFD; 
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);
    
    int8_t dx = 0, dy = 0;
    
    if (!(row1 & 0x02)) {          // UP arrow
        dy = -1;
    }
    else if (!(row1 & 0x20)) {     // DOWN (DEL key)
        dy = 1;
    }
    else if (!(row1 & 0x04)) {     // LEFT arrow
        dx = -1;
    }
    else {
        (*(volatile uint8_t*)0xDC00) = 0xFB; 
        if (!((*(volatile uint8_t*)0xDC01) & 0x04)) {  // RIGHT arrow
            dx = 1;
        }
    }
    
    // Apply movement to all NPCs that aren't moving
    if (dx != 0 || dy != 0) {
        for (uint8_t i = 0; i < count; i++) {
            if (npcs[i].steps_remaining == 0) {
                start_move(&npcs[i], dx, dy);
            }
        }
    }
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

#define NUM_NPCS 2

int main(void) {
    // Initialize hardware and load assets
    init_system();
    init_temp_allocator();
    
    // Create array of NPCs
    Settler npcs[NUM_NPCS];
    
    // NPC 0 at (10,10)
    npcs[0].x = 10; npcs[0].y = 10;
    npcs[0].off_x = 0; npcs[0].off_y = 0;
    npcs[0].old_x = 10; npcs[0].old_y = 10;
    npcs[0].char_index = 48;
    npcs[0].color = 1;
    npcs[0].dir_x = 0; npcs[0].dir_y = 0;
    npcs[0].steps_remaining = 0;
    npcs[0].npc_id = 0;
    
    // NPC 1 at (15,10)
    npcs[1].x = 15; npcs[1].y = 10;
    npcs[1].off_x = 0; npcs[1].off_y = 0;
    npcs[1].old_x = 15; npcs[1].old_y = 10;
    npcs[1].char_index = 48;
    npcs[1].color = 1;
    npcs[1].dir_x = 0; npcs[1].dir_y = 0;
    npcs[1].steps_remaining = 0;
    npcs[1].npc_id = 1;
    
    // Allocate temp chars for all NPCs
    for (uint8_t i = 0; i < NUM_NPCS; i++) {
        allocate_npc_temps(i);
    }
    
    uint8_t t1, t2;
    
    // Main game loop
    while (1) {
        wait_vsync();
        
        // Handle input for all NPCs
        handle_input(npcs, NUM_NPCS);
        
        // Update all NPCs
        for (uint8_t i = 0; i < NUM_NPCS; i++) {
            update_settler(&npcs[i]);
        }
        
        // Draw all NPCs
        for (uint8_t i = 0; i < NUM_NPCS; i++) {
            get_npc_temps(npcs[i].npc_id, &t1, &t2);
            if (t1 != 0 && t2 != 0) {
                draw_settler(&npcs[i], t1, t2);
            }
        }
        
        // Draw coordinates for debugging
        drawInt(npcs[0].x, 2, 30, 19, 1); 
        drawInt(npcs[0].y, 2, 30, 20, 1);
        drawInt(npcs[1].x, 2, 35, 19, 1); 
        drawInt(npcs[1].y, 2, 35, 20, 1);
    }
    
    // Cleanup (never reached in infinite loop)
    for (uint8_t i = 0; i < NUM_NPCS; i++) {
        release_npc_temps(i);
    }
    
    return 0;
}