#include <stdint.h>
#include <string.h>
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
// TEMPORARY CHARACTER SLOTS
// ============================================================================
#define TEMP_A    254 // $FE - First temp character for sprite drawing
#define TEMP_B    255 // $FF - Second temp character (for smooth movement)

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
} Settler;

// ============================================================================
// SYSTEM FUNCTIONS
// ============================================================================

/**
 * Wait for vertical blanking period.
 * Ensures smooth animation by syncing with screen refresh.
 * First waits for bit 7 of $d011 to be 0 (not in VBLANK),
 * then waits for raster line $FF (last line).
 */
void wait_vsync() {
    while ((*(volatile uint8_t*)0xd011) & 0x80); 
    while ((*(volatile uint8_t*)0xd012) != 0xFF);
}

/**
 * Initialize the C64 hardware and load our custom assets.
 * Steps:
 * 1. Copy custom character set to $3000 (2048 bytes)
 * 2. Set VIC-II to use our charset ($d018 = $1C = charset at $3000)
 * 3. Set border black ($d020 = 0) and background blue ($d021 = 6)
 * 4. Copy map data to screen memory
 * 5. Initialize all colors to white (1)
 */
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

/**
 * Prepare a temporary character slot with a clean background tile.
 * This preserves the original map tile while we draw on top of it.
 * 
 * @param x,y     Tile coordinates to prepare
 * @param t_idx   Temporary character index (TEMP_A or TEMP_B)
 * 
 * Steps:
 * 1. Get the original map tile character at this position
 * 2. Copy its 8-byte bitmap to the temporary character slot
 * 3. Redirect the screen to show the temporary character
 */
void prepare_temp(uint8_t x, uint8_t y, uint8_t t_idx) {
    if (x >= 40 || y >= 25) return;
    uint8_t bg_char = settlers_map[(y * 40) + x];
    memcpy(CHARSET_DEST + (t_idx << 3), CHARSET_DEST + (bg_char << 3), 8);
    SCREEN_RAM[(y * 40) + x] = t_idx;
}

// ============================================================================
// MAIN DRAWING ENGINE
// ============================================================================

/**
 * Draw a settler at its current position with smooth movement.
 * This is the core rendering function that creates the illusion of
 * smooth movement by pixel-shifting the sprite across tile boundaries.
 * 
 * @param s    Pointer to the settler to draw
 * @param t1   First temporary character slot (usually TEMP_A)
 * @param t2   Second temporary character slot (usually TEMP_B)
 * 
 * The function handles three cases:
 * 1. Horizontal movement - sprite spans two adjacent tiles
 * 2. Vertical movement - sprite spans two stacked tiles
 * 3. Idle/stationary - sprite fits in one tile
 */
void draw_settler(Settler* s, uint8_t t1, uint8_t t2) {
    // Source: the original settler character bitmap
    uint8_t* src = CHARSET_DEST + (s->char_index << 3);
    
    // Destinations: temporary workspace characters
    uint8_t* dst1 = CHARSET_DEST + (t1 << 3);
    uint8_t* dst2 = CHARSET_DEST + (t2 << 3);

    // ========================================================================
    // CASE 1: HORIZONTAL MOVEMENT (off_x > 0)
    // ========================================================================
    // The settler is moving right and spans two tiles side-by-side.
    // We need to prepare BOTH tiles with their background, then
    // shift the settler's pixels across them.
    if (s->off_x > 0) {
        prepare_temp(s->x, s->y, t1);          // Left tile
        prepare_temp(s->x + 1, s->y, t2);      // Right tile
        
        for (uint8_t i = 0; i < 8; i++) {
            // Shift the source row right by (8 - off_x) pixels
            // Using 16-bit register to handle overflow between tiles
            uint16_t row = (uint16_t)src[i] << (8 - s->off_x);
            
            // High byte goes to left tile (pixels shifted out)
            dst1[i] |= (uint8_t)(row >> 8);
            
            // Low byte goes to right tile (remaining pixels)
            dst2[i] |= (uint8_t)(row & 0xFF);
        }
    } 
    // ========================================================================
    // CASE 2: VERTICAL MOVEMENT (off_y > 0)
    // ========================================================================
    // The settler is moving down and spans two tiles vertically stacked.
    // We prepare both tiles and shift rows between them.
    else if (s->off_y > 0) {
        prepare_temp(s->x, s->y, t1);          // Top tile
        prepare_temp(s->x, s->y + 1, t2);      // Bottom tile
        
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t target_y = i + s->off_y;
            
            if (target_y < 8) {
                // Row still fits in top tile
                dst1[target_y] |= src[i];
            } else {
                // Row has moved into bottom tile
                dst2[target_y - 8] |= src[i];
            }
        }
    } 
    // ========================================================================
    // CASE 3: IDLE / SNAPPED TO GRID
    // ========================================================================
    // No movement offset - settler fits perfectly in one tile.
    else {
        prepare_temp(s->x, s->y, t1);
        for (uint8_t i = 0; i < 8; i++) {
            dst1[i] |= src[i];
        }
    }
    
    // Set the color for this tile (color RAM is not affected by pixel shifting)
    COLOR_RAM[(s->y * 40) + s->x] = s->color;
}

// ============================================================================
// MOVEMENT UPDATE
// ============================================================================

/**
 * Update settler position during smooth movement.
 * Called once per frame to advance the animation.
 * 
 * @param s    Pointer to the settler to update
 * 
 * This function:
 * 1. Calculates new pixel position based on current direction
 * 2. Updates tile coordinates and sub-pixel offsets
 * 3. When movement completes, restores background tiles
 */
void update_settler(Settler* s) {
    if (s->steps_remaining > 0) {
        // Calculate new pixel position (0-319, 0-199)
        int16_t px = (s->x * 8) + s->off_x + s->dir_x;
        int16_t py = (s->y * 8) + s->off_y + s->dir_y;

        // Update tile position and sub-pixel offset
        s->x = px / 8;     s->off_x = px % 8;
        s->y = py / 8;     s->off_y = py % 8;

        s->steps_remaining--;

        // If movement complete, clean up
        if (s->steps_remaining == 0) {
            // Restore background to vacated tiles
            uint16_t old_off = (s->old_y * 40) + s->old_x;
            SCREEN_RAM[old_off] = settlers_map[old_off];
            
            // If moving horizontally, also restore the adjacent tile
            if (s->dir_x != 0) 
                SCREEN_RAM[old_off + 1] = settlers_map[old_off + 1];
            
            // If moving vertically, restore the tile below
            if (s->dir_y != 0) 
                SCREEN_RAM[old_off + 40] = settlers_map[old_off + 40];

            // Stop moving
            s->dir_x = 0; s->dir_y = 0;
        }
    }
}

// ============================================================================
// MOVEMENT INITIATION
// ============================================================================

/**
 * Start a new movement for a settler.
 * 
 * @param s    Pointer to the settler
 * @param dx   X direction (-1, 0, 1)
 * @param dy   Y direction (-1, 0, 1)
 * 
 * Only starts if not already moving. Each tile move takes 8 steps
 * for smooth pixel-by-pixel animation.
 */
void start_move(Settler* s, int8_t dx, int8_t dy) {
    if (s->steps_remaining == 0) { 
        s->old_x = s->x; s->old_y = s->y;
        s->dir_x = dx;   s->dir_y = dy;
        s->steps_remaining = 8; 
    }
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

/**
 * Read C64 keyboard and control the settler with cursor keys.
 * 
 * @param s    Pointer to the player-controlled settler
 * 
 * Key mappings:
 * - UP arrow    → Move up (dy = -1)
 * - DEL key     → Move down (used as substitute)
 * - LEFT arrow  → Move left (dx = -1)
 * - RIGHT arrow → Move right (dx = 1)
 * 
 * Uses CIA #1 registers:
 * - $DC00: Select keyboard row
 * - $DC01: Read column status (0 = key pressed)
 */
void handle_input(Settler* s) {
    if (s->steps_remaining > 0) return; 
    
    // Check Row 1 (UP, LEFT, DEL)
    (*(volatile uint8_t*)0xDC00) = 0xFD; 
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);
    
    if (!(row1 & 0x02))           // UP arrow (bit 1)
        start_move(s, 0, -1);
    else if (!(row1 & 0x20))      // DEL key used as DOWN (bit 5)
        start_move(s, 0, 1);
    else if (!(row1 & 0x04))      // LEFT arrow (bit 2)
        start_move(s, -1, 0);
    else {
        // Check Row 2 for RIGHT arrow
        (*(volatile uint8_t*)0xDC00) = 0xFB; 
        if (!((*(volatile uint8_t*)0xDC01) & 0x04))  // RIGHT arrow (bit 2)
            start_move(s, 1, 0);
    }
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

int main(void) {
    // Initialize hardware and load assets
    init_system();
    
    // Create a player-controlled settler at (10,10)
    // Using character 48 from charset, white color (1)
    Settler npc = {10, 10, 0, 0, 10, 10, 48, 1, 0, 0, 0}; 

    // Main game loop
    while (1) {
        wait_vsync();                    // Sync with screen refresh
        handle_input(&npc);              // Check keyboard
        update_settler(&npc);            // Update position if moving
        draw_settler(&npc, TEMP_A, TEMP_B);  // Draw at current position
    }
    
    return 0;
}