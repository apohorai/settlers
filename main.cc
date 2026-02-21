#include <stdint.h>
#include <string.h>
#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// ============================================================================
// HARDWARE ADDRESSES
// ============================================================================
#define SCREEN_RAM      ((uint8_t*)0x4400)  // Moved to VIC-II Bank 1 (offset $0400)
#define COLOR_RAM       ((uint8_t*)0xD800)  // VIC-II color memory (Unchanged)
#define CHARSET_DEST    ((uint8_t*)0x6000)  // Our custom character set location
// ============================================================================
// SETTLER CONFIGURATION
// ============================================================================
#define NUM_NPCS      10   // Maximum number of settlers
#define TEMP_CHARS    128  // Pool of temporary characters (indices 128-255)


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

    // Temp character allocation
    uint8_t temp1, temp2;     // Allocated temp char indices (255 = none)
    uint8_t active;           // 1 = active, 0 = inactive
} Settler;

// ============================================================================
// GLOBAL STATE
// ============================================================================
Settler settlers[NUM_NPCS];
uint8_t selected_settler = 0;    // Currently selected settler (0-9)


// ============================================================================
// draw_int_fixed - Display integer with fixed width (leading zeros)
// 
// Parameters:
//   value: The number to display (0-65535)
//   width: Number of digits to show (1-5)
//   x: Screen column (0-39)
//   y: Screen row (0-24)
//   color: C64 color value (0-15)
// ============================================================================
void drawInt(uint16_t value, uint8_t width, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t digits[5];
    uint16_t temp = value;
    uint16_t offset;
    
    // ============================================================
    // STEP 1: Extract digits from the number
    // ============================================================
    for (uint8_t i = 0; i < width; i++) {
        uint8_t digit = temp % 10;
        if (digit == 0) {
            digits[width - 1 - i] = 0x79;  // '0'
        } else {
            digits[width - 1 - i] = 0x70 + (digit - 1);  // '1' through '9'
        }
        temp = temp / 10;
    }
    
    // ============================================================
    // STEP 2: Display all digits
    // ============================================================
    for (uint8_t i = 0; i < width; i++) {
        offset = (y * 40) + x + i;
        SCREEN_RAM[offset] = digits[i];
        COLOR_RAM[offset] = color;
    }
}

// ============================================================================
// TEMP CHARACTER ALLOCATION
// ============================================================================

// Forward declarations
void prepare_temp(uint8_t x, uint8_t y, uint8_t t_idx);
void draw_settler(Settler* s);
void release_npc_temps(Settler* s);




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
 * 6. Initialize settler array and temp char tracking
 */
void init_system() {
    // 1. Switch VIC-II to Bank 1 ($4000 - $7FFF)
    // Bits 0-1 of $DD00 control the bank (inverted). %10 = Bank 1.
    (*(volatile uint8_t*)0xdd00) = ((*(volatile uint8_t*)0xdd00) & 0xFC) | 0x02;

    // 2. Copy the charset to $6000
    memcpy(CHARSET_DEST, settlers_charset, 2048);

    // 3. Set VIC-II memory pointers for Bank 1
    // Screen offset: $0400 (bits 4-7 = %0001 -> 0x10)
    // Charset offset: $6000 is $2000 bytes into Bank 1 (bits 1-3 = %100 -> 0x08)
    // Combine them: 0x10 | 0x08 = 0x18
    (*(volatile uint8_t*)0xd018) = 0x18;
    
    (*(volatile uint8_t*)0xd020) = 0;
    (*(volatile uint8_t*)0xd021) = 6;
    
    // The rest of your map copying and array initialization logic remains identical!
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
    memset((void*)COLOR_RAM, 1, 1000);
    // Initialize settlers array and temp tracking
    memset(settlers, 0, sizeof(settlers));


    // Initialize all 10 settlers (spread across the screen)
    settlers[0].x = 5;  settlers[0].y = 5;
    settlers[1].x = 15; settlers[1].y = 5;
    settlers[2].x = 25; settlers[2].y = 5;
    settlers[3].x = 35; settlers[3].y = 5;
    settlers[4].x = 10; settlers[4].y = 12;
    settlers[5].x = 20; settlers[5].y = 12;
    settlers[6].x = 30; settlers[6].y = 12;
    settlers[7].x = 5;  settlers[7].y = 20;
    settlers[8].x = 15; settlers[8].y = 20;
    settlers[9].x = 25; settlers[9].y = 20;

    // Activate all settlers with hardcoded temp character assignments
    settlers[0].char_index = 48; settlers[0].color = 1; settlers[0].temp1 = 128; settlers[0].temp2 = 129; settlers[0].active = 1;
    settlers[1].char_index = 48; settlers[1].color = 1; settlers[1].temp1 = 130; settlers[1].temp2 = 131; settlers[1].active = 1;
    settlers[2].char_index = 48; settlers[2].color = 1; settlers[2].temp1 = 132; settlers[2].temp2 = 133; settlers[2].active = 1;
    settlers[3].char_index = 48; settlers[3].color = 1; settlers[3].temp1 = 134; settlers[3].temp2 = 135; settlers[3].active = 1;
    settlers[4].char_index = 48; settlers[4].color = 1; settlers[4].temp1 = 136; settlers[4].temp2 = 137; settlers[4].active = 1;
    settlers[5].char_index = 48; settlers[5].color = 1; settlers[5].temp1 = 138; settlers[5].temp2 = 139; settlers[5].active = 1;
    settlers[6].char_index = 48; settlers[6].color = 1; settlers[6].temp1 = 140; settlers[6].temp2 = 141; settlers[6].active = 1;
    settlers[7].char_index = 48; settlers[7].color = 1; settlers[7].temp1 = 142; settlers[7].temp2 = 143; settlers[7].active = 1;
    settlers[8].char_index = 48; settlers[8].color = 1; settlers[8].temp1 = 144; settlers[8].temp2 = 145; settlers[8].active = 1;
    settlers[9].char_index = 48; settlers[9].color = 1; settlers[9].temp1 = 146; settlers[9].temp2 = 147; settlers[9].active = 1;

    // One-time render of all settlers at startup
    for (uint8_t i = 0; i < NUM_NPCS; i++) {
        if (settlers[i].active) {
            draw_settler(&settlers[i]);
        }
    }
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
    if (t_idx < 128) return;  // Safety guard: only write to temp chars (128-255)
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
 *
 * The function handles three cases:
 * 1. Horizontal movement - sprite spans two adjacent tiles
 * 2. Vertical movement - sprite spans two stacked tiles
 * 3. Idle/stationary - sprite fits in one tile
 */
void draw_settler(Settler* s) {
    if (!s->active || s->temp1 == 255 || s->temp2 == 255) return;

    // Source: the original settler character bitmap
    uint8_t* src = CHARSET_DEST + (s->char_index << 3);

    // Destinations: temporary workspace characters
    uint8_t* dst1 = CHARSET_DEST + (s->temp1 << 3);
    uint8_t* dst2 = CHARSET_DEST + (s->temp2 << 3);

    // ========================================================================
    // CASE 1: HORIZONTAL MOVEMENT (off_x > 0)
    // ========================================================================
    // The settler is moving right and spans two tiles side-by-side.
    // We need to prepare BOTH tiles with their background, then
    // shift the settler's pixels across them.
    if (s->off_x > 0) {
        prepare_temp(s->x, s->y, s->temp1);          // Left tile
        prepare_temp(s->x + 1, s->y, s->temp2);      // Right tile

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
        prepare_temp(s->x, s->y, s->temp1);          // Top tile
        prepare_temp(s->x, s->y + 1, s->temp2);      // Bottom tile

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
    // Initialize temp char with background on first render, then just merge sprite.
    else {
        prepare_temp(s->x, s->y, s->temp1);
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
            COLOR_RAM[old_off] = 1;  // Restore color to white (background)

            // If moving horizontally, also restore the adjacent tile
            if (s->dir_x != 0) {
                SCREEN_RAM[old_off + 1] = settlers_map[old_off + 1];
                COLOR_RAM[old_off + 1] = 1;
            }

            // If moving vertically, restore the tile below
            if (s->dir_y != 0) {
                SCREEN_RAM[old_off + 40] = settlers_map[old_off + 40];
                COLOR_RAM[old_off + 40] = 1;
            }

            // Stop moving
            s->dir_x = 0; s->dir_y = 0;

            // Render settler at final position
            draw_settler(s);
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
 * Read number key presses (1-9, 0) to select settler.
 * Maps: 1-9 to settlers 0-8, 0 to settler 9
 *
 * Keyboard layout (verified by testing):
 * Row 7 ($7F): 1(bit0), 2(bit3)
 * Row 1 ($FD): 3(bit0), 4(bit3)
 * Row 2 ($FB): 5(bit0), 6(bit3)
 * Row 3 ($F7): 7(bit0), 8(bit3)
 * Row 4 ($EF): 9(bit0), 0(bit3)
 *
 * Returns settler index 0-9, or 255 if no key pressed
 */
uint8_t read_number_key(void) {
    // Row 7 ($7F): 1, 2
    (*(volatile uint8_t*)0xDC00) = 0x7F;
    uint8_t row7 = (*(volatile uint8_t*)0xDC01);
    if (!(row7 & 0x01)) return 0;  // 1 -> settler 0
    if (!(row7 & 0x08)) return 1;  // 2 -> settler 1

    // Row 1 ($FD): 3, 4
    (*(volatile uint8_t*)0xDC00) = 0xFD;
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);
    if (!(row1 & 0x01)) return 2;  // 3 -> settler 2
    if (!(row1 & 0x08)) return 3;  // 4 -> settler 3

    // Row 2 ($FB): 5, 6
    (*(volatile uint8_t*)0xDC00) = 0xFB;
    uint8_t row2 = (*(volatile uint8_t*)0xDC01);
    if (!(row2 & 0x01)) return 4;  // 5 -> settler 4
    if (!(row2 & 0x08)) return 5;  // 6 -> settler 5

    // Row 3 ($F7): 7, 8
    (*(volatile uint8_t*)0xDC00) = 0xF7;
    uint8_t row3 = (*(volatile uint8_t*)0xDC01);
    if (!(row3 & 0x01)) return 6;  // 7 -> settler 6
    if (!(row3 & 0x08)) return 7;  // 8 -> settler 7

    // Row 4 ($EF): 9, 0
    (*(volatile uint8_t*)0xDC00) = 0xEF;
    uint8_t row4 = (*(volatile uint8_t*)0xDC01);
    if (!(row4 & 0x01)) return 8;  // 9 -> settler 8
    if (!(row4 & 0x08)) return 9;  // 0 -> settler 9

    // Reset CIA to prevent interference
    (*(volatile uint8_t*)0xDC00) = 0xFF;

    return 255;  // No number key pressed
}

/**
 * Handle input from keyboard to select and move settlers.
 * Number keys (1-9, 0) select which settler to control.
 * Arrow keys move only the selected settler.
 *
 * Key mappings:
 * - Number keys: Select settler (1=settler0, 2=settler1, ..., 0=settler9)
 * - UP arrow    → Move up (dy = -1)
 * - DEL key     → Move down (used as substitute)
 * - LEFT arrow  → Move left (dx = -1)
 * - RIGHT arrow → Move right (dx = 1)
 *
 * Uses CIA #1 registers:
 * - $DC00: Select keyboard row
 * - $DC01: Read column status (0 = key pressed)
 */
void handle_input(void) {
    // Check for number key to select settler
    uint8_t num_key = read_number_key();
    if (num_key != 255 && num_key < NUM_NPCS) {
        selected_settler = num_key;
    }

    // Only proceed with movement if selected settler is active
    if (!settlers[selected_settler].active) {
        return;
    }
    int8_t dx = 0, dy = 0;

    // Check Row 1 (UP, LEFT, DEL)
    (*(volatile uint8_t*)0xDC00) = 0xFD;
    uint8_t row1 = (*(volatile uint8_t*)0xDC01);

    if (!(row1 & 0x02))           // UP arrow (bit 1)
        dy = -1;
    else if (!(row1 & 0x20))      // DEL key used as DOWN (bit 5)
        dy = 1;
    else if (!(row1 & 0x04))      // LEFT arrow (bit 2)
        dx = -1;
    else {
        // Check Row 2 for RIGHT arrow
        (*(volatile uint8_t*)0xDC00) = 0xFB;
        if (!((*(volatile uint8_t*)0xDC01) & 0x04))  // RIGHT arrow (bit 2)
            dx = 1;
    }

    // Apply movement only to the selected settler
    if (dx != 0 || dy != 0) {
        if (settlers[selected_settler].steps_remaining == 0) {
            start_move(&settlers[selected_settler], dx, dy);
        }
    }
}
// Trace and display the path between (2,2) and (7,11)
// Shows path characters on screen starting at (3,1)
void show_path(void) {
    // Hardcoded waypoints
    uint8_t start_x = 2;
    uint8_t start_y = 2;
    uint8_t target_x = 7;
    uint8_t target_y = 11;
    
    // Queue for BFS
    uint8_t queue_x[256];
    uint8_t queue_y[256];
    uint8_t head = 0;
    uint8_t tail = 0;
    
    // Visited array and parent tracking
    uint8_t visited[40][25] = {{0}};
    int8_t parent_x[40][25];
    int8_t parent_y[40][25];
    
    // Initialize parents to -1
    for (uint8_t i = 0; i < 40; i++) {
        for (uint8_t j = 0; j < 25; j++) {
            parent_x[i][j] = -1;
            parent_y[i][j] = -1;
        }
    }
    
    extern const uint8_t settlers_map[];
    
    // Check if start and target are valid
    if (settlers_map[(start_y * 40) + start_x] > 6) return;
    if (settlers_map[(target_y * 40) + target_x] > 6) return;
    
    // Start BFS
    queue_x[head] = start_x;
    queue_y[head] = start_y;
    head++;
    visited[start_x][start_y] = 1;
    
    // Directions: right, down, left, up
    int8_t dx[4] = {1, 0, -1, 0};
    int8_t dy[4] = {0, 1, 0, -1};
    
    uint8_t found = 0;
    
    while (tail < head && !found) {
        uint8_t x = queue_x[tail];
        uint8_t y = queue_y[tail];
        tail++;
        
        // Check neighbors
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t nx = x + dx[i];
            uint8_t ny = y + dy[i];
            
            if (nx >= 40 || ny >= 25) continue;
            if (visited[nx][ny]) continue;
            if (settlers_map[(ny * 40) + nx] > 6) continue;
            
            visited[nx][ny] = 1;
            parent_x[nx][ny] = x;
            parent_y[nx][ny] = y;
            
            if (nx == target_x && ny == target_y) {
                found = 1;
                break;
            }
            
            queue_x[head] = nx;
            queue_y[head] = ny;
            head++;
        }
    }
    
    if (!found) {
        SCREEN_RAM[(1 * 40) + 3] = 0x4E;  // 'N'
        SCREEN_RAM[(1 * 40) + 4] = 0x4F;  // 'O'
        return;
    }
    
    // Reconstruct path
    uint8_t path_x[100];
    uint8_t path_y[100];
    uint8_t path_len = 0;
    
    uint8_t cx = target_x;
    uint8_t cy = target_y;
    
    while (cx != start_x || cy != start_y) {
        path_x[path_len] = cx;
        path_y[path_len] = cy;
        path_len++;
        
        uint8_t px = parent_x[cx][cy];
        uint8_t py = parent_y[cx][cy];
        cx = px;
        cy = py;
    }
    path_x[path_len] = start_x;
    path_y[path_len] = start_y;
    path_len++;
    
    // Display path at (3,1) onward
    // Format: (X,Y) (X,Y) etc.
    uint8_t display_x = 3;
    
    
}
// ============================================================================
// MAIN PROGRAM
// ============================================================================

int main(void) {
    // Initialize hardware and load assets
    init_system();

    show_path();
    
    // Main game loop
    while (1) {
        wait_vsync();              // Sync with screen refresh
        handle_input();             // Check keyboard

        // Update all active settlers
        for (uint8_t i = 0; i < NUM_NPCS; i++) {
            if (settlers[i].active) {
                update_settler(&settlers[i]);
            }
        }

        // Draw only moving settlers (idle settlers don't need per-frame updates)
        for (uint8_t i = 0; i < NUM_NPCS; i++) {
            if (settlers[i].active && settlers[i].steps_remaining > 0) {
                draw_settler(&settlers[i]);
            }
        }

        // Display selected settler number at top
        drawInt(selected_settler + 1, 1, 0, 0, 1);

        // Display all settlers coordinates (compact, right-aligned)
        // Row 23: Settlers 1-5 (format: X,Y with 1 space between)
        for (uint8_t i = 0; i < 5 && i < NUM_NPCS; i++) {
            if (settlers[i].active) {
                uint8_t display_x = 2 + (i * 7);  // Start at col 2, 7 chars per settler
                drawInt(settlers[i].x, 2, display_x, 23, 1);
                drawInt(settlers[i].y, 2, display_x + 3, 23, 1);
            }
        }

        // Row 24: Settlers 6-10 (format: X,Y with 1 space between)
        for (uint8_t i = 5; i < 10 && i < NUM_NPCS; i++) {
            if (settlers[i].active) {
                uint8_t display_x = 2 + ((i - 5) * 7);  // Start at col 2, 7 chars per settler
                drawInt(settlers[i].x, 2, display_x, 24, 1);
                drawInt(settlers[i].y, 2, display_x + 3, 24, 1);
            }
        }
    }

    return 0;
}