#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// ============================================================================
// HARDWARE MEMORY DEFINITIONS
// ============================================================================
// SCREEN_RAM: Pointer to VIC-II video memory (40x25 characters = 1000 bytes)
// This is where character codes are stored to define what appears on screen
#define SCREEN_RAM      ((uint8_t*)0x0400)

// COLOR_RAM: Pointer to VIC-II color memory (1000 bytes)
// Each byte corresponds to a screen position and sets the color of that character
#define COLOR_RAM       ((uint8_t*)0xD800)

// CHARSET_DEST: Destination address for our custom character set
// We're relocating the character set to $3000 to avoid conflict with BASIC ROM
// Each character is 8 bytes (8x8 pixels, 1 bit per pixel)
#define CHARSET_DEST    ((uint8_t*)0x3000)

// ============================================================================
// CHARACTER POOL CONSTANTS
// ============================================================================
// The C64 has 256 character definitions (0-255)
// We reserve indices 150-255 for dynamic use (smooth movement, temporary effects)
// These can be modified at runtime without affecting static map tiles
#define POOL_START 150   // First dynamic character index
#define POOL_END   255   // Last dynamic character index
#define POOL_SIZE  (POOL_END - POOL_START + 1)  // Total: 106 characters

// ============================================================================
// TILE PROPERTY FLAGS
// ============================================================================
#define FLAG_NONE       0          // Default: not walkable
#define FLAG_PATH       (1 << 0)   // Bit 0 set = walkable road/path

// ============================================================================
// GLOBAL MAP DATA ARRAYS
// ============================================================================
// Properties for each tile type (0-255)
// Stores flags like FLAG_PATH to determine walkability
static uint8_t tileProps[256];

// Waypoint map: 40x25 grid (1000 bytes)
// 1 = this tile is a waypoint (displayed in color 5/green)
static uint8_t waypointMap[1000]; 

// Path overlay: 40x25 grid (1000 bytes)
// 1 = this tile is part of a calculated path (displayed in color 7/yellow)
static uint8_t pathOverlay[1000]; 

// Parent map for pathfinding: 40x25 grid (1000 bytes)
// Stores the direction to the previous tile in the path
// Values: 0=up, 1=down, 2=left, 3=right, 255=unvisited, 4=start
static uint8_t parentMap[1000];   

// ============================================================================
// SETTLER (NPC) STRUCTURE DEFINITION
// ============================================================================
typedef struct {
    // Position and movement data
    uint8_t x, y;           // Current tile coordinates (0-39, 0-24)
    uint8_t offX, offY;     // Sub-pixel offset within current tile (0-7)
    uint8_t oldX, oldY;     // Previous tile position (for cleanup after move)
    uint8_t charIndex;      // Base character index for this NPC's appearance (usually 48)
    uint8_t color;          // NPC color (C64 color value 0-15)
    
    // Movement state
    int8_t  dirX, dirY;     // Current movement direction (-1, 0, or 1)
    uint8_t stepsRemaining; // Steps left in current move (8 = full tile, 0 = stationary)
    
    // Temporary character slots (from pool 150-255)
    // t1 covers the main tile, t2 covers adjacent tile during smooth movement
    uint8_t t1[2], t2[2];   // [0]=char index, [1]=unused/reserved
    
    // Path following data
    uint8_t pathActions[64]; // Forward path movement commands
    uint8_t pathLen;         // Length of forward path
    uint8_t pathBack[64];    // Return path movement commands
    uint8_t backLen;         // Length of return path
    uint8_t currentStep;     // Current step index in active path
    uint8_t returning;       // Flag: 0=following forward path, 1=following return path
} Settler;

// ============================================================================
// TEMPORARY CHARACTER POOL STRUCTURE
// ============================================================================
typedef struct { 
    uint8_t stack[POOL_SIZE];  // Stack of available character indices (150-255)
    int16_t top;                // Stack pointer (-1 = empty)
} TempPool;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
static TempPool charPool;  // Pool of temporary character indices
static Settler npc[5];     // Array of 5 NPCs (0-4)

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// ============================================================================
// getMapColor - Determine the color for a map tile based on its properties
// Parameters: x, y - tile coordinates (0-39, 0-24)
// Returns: C64 color value (0-15)
// Color legend:
// - 5 (green): Waypoint tile
// - 7 (yellow): Path overlay tile
// - 2 (red): Road tile (FLAG_PATH set)
// - 1 (white): Default tile
// ============================================================================
uint8_t getMapColor(uint8_t x, uint8_t y) {
    uint16_t off = (y * 40) + x;  // Convert 2D coordinates to linear offset
    
    if (waypointMap[off]) return 5;      // Waypoints are green
    if (pathOverlay[off]) return 7;      // Path overlay is yellow
    if (tileProps[settlers_map[off]] & FLAG_PATH) return 2;  // Roads are red
    return 1;  // Default white
}

// ============================================================================
// setWaypoint - Mark a tile as a waypoint
// Parameters: x, y - tile coordinates to mark
// Bounds checking ensures we don't write outside the map
// ============================================================================
void setWaypoint(uint8_t x, uint8_t y) { 
    if (x < 40 && y < 25) waypointMap[(y * 40) + x] = 1; 
}

// ============================================================================
// PATHFINDING: calculateYellowPathOnRoad
// ============================================================================
// Breadth-First Search (BFS) pathfinding algorithm
// Finds shortest path between two points using only road tiles (FLAG_PATH)
// Parameters: x1,y1 - start coordinates, x2,y2 - target coordinates
// Results stored in parentMap[] and pathOverlay[]
// ============================================================================
void calculateYellowPathOnRoad(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    static uint16_t queue[400];  // BFS queue (max 400 tiles = whole map)
    uint16_t head = 0, tail = 0; // Queue head and tail pointers
    
    // Initialize arrays: parentMap to 255 (unvisited), pathOverlay to 0 (clear)
    memset(parentMap, 255, 1000);
    memset(pathOverlay, 0, 1000);
    
    // Calculate start and target linear offsets
    uint16_t start = (y1 * 40) + x1;
    uint16_t target = (y2 * 40) + x2;
    
    // Initialize BFS with start node
    queue[tail++] = start;  // Push start to queue
    parentMap[start] = 4;   // Mark start with special value 4 (not a direction)
    
    // BFS main loop
    while (head < tail) {
        uint16_t curr = queue[head++];  // Pop next tile from queue
        if (curr == target) break;      // Found target, exit early
        
        // Get current tile coordinates
        uint8_t cx = curr % 40;
        uint8_t cy = curr / 40;
        
        // Check all four neighbors (up, down, left, right)
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t nx = cx, ny = cy;
            
            // Set neighbor coordinates based on direction
            if (i == 0 && cy > 0) ny--;           // Up
            else if (i == 1 && cy < 24) ny++;     // Down
            else if (i == 2 && cx > 0) nx--;      // Left
            else if (i == 3 && cx < 39) nx++;     // Right
            else continue;  // Skip if out of bounds
            
            uint16_t next = (ny * 40) + nx;  // Linear offset of neighbor
            
            // If neighbor is a road tile AND not yet visited
            if ((tileProps[settlers_map[next]] & FLAG_PATH) && parentMap[next] == 255) {
                parentMap[next] = i;        // Record direction from current to next
                queue[tail++] = next;       // Add to queue for later exploration
            }
        }
    }
    
    // If path found (target was reached), reconstruct and mark the path
    if (parentMap[target] != 255) {
        uint16_t curr = target;
        
        // Trace back from target to start using parentMap
        while (curr != start) {
            pathOverlay[curr] = 1;  // Mark this tile as part of path
            
            // Get current coordinates and direction to parent
            uint8_t cx = curr % 40;
            uint8_t cy = curr / 40;
            uint8_t dir = parentMap[curr];
            
            // Move to parent tile (reverse direction)
            if (dir == 0) cy++;      // Came from up, so parent is down
            else if (dir == 1) cy--; // Came from down, so parent is up
            else if (dir == 2) cx++; // Came from left, so parent is right
            else if (dir == 3) cx--; // Came from right, so parent is left
            
            curr = (cy * 40) + cx;   // Update current to parent
        }
        pathOverlay[start] = 1;  // Mark start tile as part of path
    }
    // If no path found, pathOverlay remains all zeros
}

// ============================================================================
// loadPathToNpc - Convert parentMap path to NPC movement commands
// Parameters:
// - s: Pointer to Settler structure
// - targetX, targetY: Destination coordinates
// - startNode: Linear offset of starting position
// - mode: 0 = load forward path (pathActions), 1 = load return path (pathBack)
// ============================================================================
void loadPathToNpc(Settler* s, uint8_t targetX, uint8_t targetY, uint16_t startNode, uint8_t mode) {
    uint8_t temp[64];          // Temporary buffer for path commands
    uint8_t count = 0;          // Number of steps in path
    uint8_t cx = targetX, cy = targetY;  // Start at target and work backwards
    
    // Trace path from target back to start
    while (((cy * 40) + cx) != startNode && count < 64) {
        uint16_t off = (cy * 40) + cx;
        uint8_t move = parentMap[off];  // Get direction from parentMap
        
        if (move == 255) break;  // Invalid path, exit
        
        temp[count++] = move;    // Store direction
        
        // Move to parent tile (reverse direction)
        if (move == 0) cy++;      // Up direction → parent is down
        else if (move == 1) cy--; // Down direction → parent is up
        else if (move == 2) cx++; // Left direction → parent is right
        else if (move == 3) cx--; // Right direction → parent is left
    }
    
    // Store path in NPC structure (reversing order to get forward direction)
    if (mode == 0) {
        s->pathLen = count;
        // Reverse the path so it goes from start to target
        for (uint8_t i = 0; i < count; i++) 
            s->pathActions[i] = temp[(count - 1) - i];
    } else {
        s->backLen = count;
        // Reverse for return path
        for (uint8_t i = 0; i < count; i++) 
            s->pathBack[i] = temp[(count - 1) - i];
    }
    
    s->currentStep = 0;  // Start at beginning of path
}

// ============================================================================
// initPool - Initialize the temporary character pool
// Creates a stack of available character indices from POOL_START to POOL_END
// These indices are reserved for dynamic use (smooth movement, effects)
// ============================================================================
void initPool() {
    // Initialize the character pool as an empty stack
    // Set top to -1 to indicate the stack is empty (will become 0 after first push)
    charPool.top = -1;
    
    // Fill the pool with character indices from POOL_START (150) to POOL_END (255)
    // These are "dynamic" character slots that can be modified at runtime for animation
    for (int16_t i = POOL_START; i <= POOL_END; i++) {
        // Push each character index onto the stack
        // Pre-increment (++charPool.top) advances pointer before storing
        charPool.stack[++charPool.top] = (uint8_t)i;
    }
}

// ============================================================================
// assignChar - Allocate a temporary character from the pool
// Returns: Character index (150-255) or 0 if pool empty
// ============================================================================
uint8_t assignChar() { 
    return (charPool.top >= 0) ? charPool.stack[charPool.top--] : 0; 
}

// ============================================================================
// waitVsync - Wait for vertical blanking period
// Ensures smooth animation by synchronizing with the display refresh
// First waits for bit 7 of $d011 to be 0 (not in VBLANK)
// Then waits for $d012 = $FF (last raster line)
// ============================================================================
void waitVsync() { 
    while ((*(volatile uint8_t*)0xd011) & 0x80); 
    while ((*(volatile uint8_t*)0xd012) != 0xFF); 
}

// ============================================================================
// initSystem - Initialize C64 hardware and load assets
// Steps:
// 1. Copy custom charset to $3000 (2048 bytes = 256 characters × 8 bytes)
// 2. Set VIC-II register $d018 to point to our charset at $3000
//    Value $1C = %00011100 -> charset at $3000, screen RAM at $0400
// 3. Set border color ($d020) to black (0)
// 4. Set background color ($d021) to blue (6)
// 5. Copy map data to screen RAM
// ============================================================================
void initSystem() {
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    (*(volatile uint8_t*)0xd018) = 0x1C; 
    (*(volatile uint8_t*)0xd020) = 0; 
    (*(volatile uint8_t*)0xd021) = 6;    
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
}

// ============================================================================
// refreshAllColors - Update all color RAM entries based on map properties
// Iterates through every screen position and sets the appropriate color
// ============================================================================
void refreshAllColors() { 
    for (uint8_t y = 0; y < 25; y++) 
        for (uint8_t x = 0; x < 40; x++) 
            COLOR_RAM[(y * 40) + x] = getMapColor(x, y); 
}

// ============================================================================
// prepareTemp - Prepare a temporary character slot with background tile data
// Parameters:
// - x, y: Tile coordinates to prepare
// - tIdx: Array containing temporary character index [0]
// 
// This function copies the current screen character's bitmap to a temporary
// character slot, then redirects the screen to show the temporary character.
// This preserves the background while we draw NPCs on top.
// ============================================================================
void prepareTemp(uint8_t x, uint8_t y, uint8_t tIdx[2]) {
    if (x >= 40 || y >= 25 || tIdx[0] == 0) return;
    
    // Get the character index currently displayed on the screen at this spot
    uint8_t currentChar = SCREEN_RAM[(y * 40) + x];
    
    // If the current character is already one of OUR temp characters from the pool,
    // we should look at the settlers_map instead to avoid "recursive" merging.
    if (currentChar >= POOL_START) {
        currentChar = settlers_map[(y * 40) + x];
    }

    // Copy the bitmap of the CURRENT character to our workspace
    memcpy(CHARSET_DEST + (tIdx[0] << 3), CHARSET_DEST + (currentChar << 3), 8);
    
    // Tell the screen to show our dynamic workspace character
    SCREEN_RAM[(y * 40) + x] = tIdx[0];
}

// ============================================================================
// releaseChar - Return a temporary character to the pool
// Parameters: c - Character index to release
// 
// Steps:
// 1. Verify it's a pool character (150-255)
// 2. Clear its bitmap data (8 bytes) to prevent garbage
// 3. Push it back onto the pool stack
// ============================================================================
void releaseChar(uint8_t c) {
    // Only put it back if it's actually a pool character
    if (c >= POOL_START && c <= POOL_END) {
        // Clear the character bitmap data to prevent garbage accumulation
        memset(CHARSET_DEST + (c << 3), 0, 8);
        charPool.stack[++charPool.top] = c;
    }
}

// ============================================================================
// drawSettler - Draw an NPC at its current position with smooth movement
// Parameters: s - Pointer to Settler structure
// 
// This is the core rendering function that creates the illusion of smooth
// movement by pixel-shifting the NPC sprite across tile boundaries.
// ============================================================================
void drawSettler(Settler* s) {
    // --- STEP 1: DYNAMIC RESOURCE ALLOCATION ---
    // If this NPC doesn't have temporary character slots yet, 
    // grab them from the pool (indices 150-255).
    if (s->t1[0] == 0) s->t1[0] = assignChar(); 
    if (s->t2[0] == 0) s->t2[0] = assignChar();

    // Calculate memory pointers for the source (original NPC look) 
    // and the destinations (the dynamic workspace in the charset).
    // Each character is 8 bytes, so multiply index by 8 for offset
    uint8_t* src = CHARSET_DEST + (s->charIndex << 3);
    uint8_t* dst1 = CHARSET_DEST + (s->t1[0] << 3); 
    uint8_t* dst2 = CHARSET_DEST + (s->t2[0] << 3);

    // --- STEP 2: PIXEL-SHIFTING LOGIC ---
    
    // Case A: Smooth Horizontal Movement (off_x > 0)
    // NPC is moving right, spanning two tiles horizontally
    if (s->offX > 0) {
        // Prepare the background by copying the underlying tile into our temp slots
        prepareTemp(s->x, s->y, s->t1); 
        prepareTemp(s->x + 1, s->y, s->t2);
        
        // For each row of the character (0-7)
        for (uint8_t i = 0; i < 8; i++) {
            // Shift the source byte across two characters using a 16-bit register
            // Left shift by (8 - offX) moves pixels to the right
            uint16_t row = (uint16_t)src[i] << (8 - s->offX); 
            dst1[i] |= (uint8_t)(row >> 8);   // Left half of shift (high byte)
            dst2[i] |= (uint8_t)(row & 0xFF); // Right half of shift (low byte)
        }
    } 
    // Case B: Smooth Vertical Movement (off_y > 0)
    // NPC is moving down, spanning two tiles vertically
    else if (s->offY > 0) {
        prepareTemp(s->x, s->y, s->t1); 
        prepareTemp(s->x, s->y + 1, s->t2);
        
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t ty = i + s->offY;
            // If the shifted pixel is still in the first tile, draw it there
            if (ty < 8) dst1[ty] |= src[i]; 
            // Otherwise, wrap it into the tile below
            else dst2[ty - 8] |= src[i]; 
        }
    } 
    // Case C: Snap to Grid (No offset)
    // NPC is stationary or at exact tile boundary
    else { 
        prepareTemp(s->x, s->y, s->t1); 
        for (uint8_t i = 0; i < 8; i++) dst1[i] |= src[i]; 
    }

    // --- STEP 3: COLORING ---
    // Set the hardware color RAM for the current screen position
    COLOR_RAM[(s->y * 40) + s->x] = s->color;
}

// ============================================================================
// restoreTile - Restore a tile to its original map state
// Parameters: x, y - Tile coordinates to restore
// 
// Replaces the current screen character with the original map tile
// and sets its color based on map properties.
// ============================================================================
void restoreTile(uint8_t x, uint8_t y) { 
    uint16_t off = (y * 40) + x; 
    SCREEN_RAM[off] = settlers_map[off]; 
    COLOR_RAM[off] = getMapColor(x, y); 
}

// ============================================================================
// startMove - Initiate a movement for an NPC
// Parameters:
// - s: Pointer to Settler structure
// - dx, dy: Direction vector (-1, 0, or 1)
// 
// Validates move (bounds checking) and sets up movement state
// Each tile move takes 8 steps for smooth animation
// ============================================================================
void startMove(Settler* s, int8_t dx, int8_t dy) {
    if (s->stepsRemaining == 0) { 
        uint8_t nx = s->x + dx, ny = s->y + dy; 
        // Bounds check (keep within map, avoid edges)
        if (nx >= 39 || ny >= 24 || nx < 1 || ny < 1) return; 
        s->oldX = s->x; 
        s->oldY = s->y; 
        s->dirX = dx; 
        s->dirY = dy; 
        s->stepsRemaining = 8;  // 8 steps per tile
    }
}

// ============================================================================
// updateSettler - Update NPC position during smooth movement
// Parameters: s - Pointer to Settler structure
// 
// This function handles the pixel-by-pixel smooth movement of an NPC across the screen
// Called once per frame to advance the animation
// ============================================================================
void updateSettler(Settler* s) {
    // ============================================================
    // STEP 1: CHECK IF NPC IS CURRENTLY MOVING
    // ============================================================
    // Only process movement if there are steps remaining in the current move
    // Each tile movement takes 8 steps for smooth pixel-by-pixel animation
    // If stepsRemaining = 0, the NPC is stationary on a tile
    // ============================================================
    if (s->stepsRemaining > 0) {
        
        // ============================================================
        // STEP 2: CALCULATE NEW PIXEL POSITION
        // ============================================================
        // Convert tile coordinates to pixel coordinates:
        // - s->x * 8: Convert tile column to absolute pixel X (0-319)
        // - s->y * 8: Convert tile row to absolute pixel Y (0-199)
        // - s->offX: Add current sub-pixel offset within tile (0-7)
        // - s->offY: Add current sub-pixel offset within tile (0-7)
        // - s->dirX/s->dirY: Add movement direction (-1, 0, or +1)
        // 
        // The result is a new pixel position that may cross tile boundaries
        // ============================================================
        int16_t px = (s->x * 8) + s->offX + s->dirX;
        int16_t py = (s->y * 8) + s->offY + s->dirY;
        
        // ============================================================
        // STEP 3: UPDATE TILE POSITION AND SUB-PIXEL OFFSET
        // ============================================================
        // Integer division by 8 gives the new tile coordinate
        // Modulo 8 gives the new sub-pixel offset within that tile
        // This maintains precision while moving between tiles
        // Example: px = 83 → x = 10, offX = 3 (pixel 3 within tile 10)
        // ============================================================
        s->x = px / 8;      // New tile column
        s->offX = px % 8;   // New horizontal offset within tile (0-7)
        s->y = py / 8;      // New tile row
        s->offY = py % 8;   // New vertical offset within tile (0-7)
        
        // ============================================================
        // STEP 4: DECREMENT STEPS AND CHECK IF MOVE IS COMPLETE
        // ============================================================
        // Count down from 8 to 0. When stepsRemaining reaches 0:
        // - The NPC has completed moving to the new tile
        // - All 8 sub-pixel positions have been displayed
        // - Time to clean up and prepare for next move
        // ============================================================
        if (--s->stepsRemaining == 0) { 
            
            // ============================================================
            // STEP 5: RESTORE BACKGROUND TILES
            // ============================================================
            // The NPC was drawn using temporary characters that overwrote
            // the background tiles. Now we need to restore the original
            // map tiles underneath where the NPC used to be.
            // 
            // Always restore the main tile at the old position
            restoreTile(s->oldX, s->oldY); 
            
            // If moving horizontally, the NPC spanned TWO tiles side-by-side
            // (due to the pixel shifting technique). Restore the adjacent tile.
            if (s->dirX != 0) restoreTile(s->oldX + 1, s->oldY); 
            
            // If moving vertically, the NPC spanned TWO tiles stacked
            // (due to the pixel shifting technique). Restore the tile below.
            if (s->dirY != 0) restoreTile(s->oldX, s->oldY + 1); 
            
            // ============================================================
            // STEP 6: RESET MOVEMENT STATE
            // ============================================================
            // Clear direction vectors since the move is complete
            // The NPC is now stationary on its new tile
            // ============================================================
            s->dirX = 0; 
            s->dirY = 0; 

            // ============================================================
            // STEP 7: RELEASE TEMPORARY CHARACTERS BACK TO POOL
            // ============================================================
            // The NPC used two character slots from the pool (indices 150-255)
            // to create the smooth movement effect. These need to be:
            // 1. Cleared of any drawn pixels (to prevent garbage)
            // 2. Returned to the pool stack for other NPCs to use
            // 
            // This is critical for preventing character exhaustion!
            // ============================================================
            releaseChar(s->t1[0]); 
            releaseChar(s->t2[0]);
            
            // ============================================================
            // STEP 8: MARK TEMP SLOTS AS AVAILABLE
            // ============================================================
            // Reset the NPC's temporary character indices to 0
            // This signals to drawSettler() that new temp chars need to be
            // assigned from the pool on the next drawing cycle
            // ============================================================
            s->t1[0] = 0; 
            s->t2[0] = 0;
        }
    }
}

// ============================================================================
// handleAi - Simple random movement AI for NPCs
// Parameters: s - Pointer to Settler structure
// 
// 5% chance per frame to start moving in a random direction
// Only acts if NPC is not already moving
// ============================================================================
void handleAi(Settler* s) {
    if (s->stepsRemaining > 0) return;
    if ((rand() % 100) < 5) { 
        int8_t r = rand() % 4; 
        if (r == 0) startMove(s, 0, -1); 
        else if (r == 1) startMove(s, 0, 1); 
        else if (r == 2) startMove(s, -1, 0); 
        else startMove(s, 1, 0); 
    }
}

// ============================================================================
// handleNpcPathing - Path following for NPCs with pre-calculated routes
// Parameters: s - Pointer to Settler structure
// 
// Moves NPC along a path, switching between forward and return paths
// when reaching the end. Path commands: 0=up, 1=down, 2=left, 3=right
// ============================================================================
void handleNpcPathing(Settler* s) {
    if (s->stepsRemaining > 0) return;
    
    // Determine current path length (forward or return)
    uint8_t max = s->returning ? s->backLen : s->pathLen;
    
    // Check if reached end of current path
    if (s->currentStep >= max) {
        s->returning = !s->returning;  // Switch direction
        s->currentStep = 0;              // Reset to start of new path
        return;
    }
    
    // Get next movement command
    uint8_t move = s->returning ? s->pathBack[s->currentStep++] : s->pathActions[s->currentStep++];
    
    // Convert command to movement
    if (move == 0) startMove(s, 0, -1); 
    else if (move == 1) startMove(s, 0, 1);
    else if (move == 2) startMove(s, -1, 0); 
    else if (move == 3) startMove(s, 1, 0);
}

// ============================================================================
// handleInput - Read C64 keyboard for player control
// Parameters: s - Pointer to Settler structure (usually NPC[0])
// 
// Maps cursor keys to movement:
// - UP arrow: Move up
// - DEL key: Move down (used as substitute)
// - LEFT arrow: Move left
// - RIGHT arrow: Move right
// 
// Uses CIA #1 registers $DC00 (row select) and $DC01 (column read)
// ============================================================================
void handleInput(Settler* s) {
    // Don't process input if settler is currently moving
    if (s->stepsRemaining > 0) return;
    
    // C64 Keyboard Reading:
    // - 0xDC00 = CIA 1 Port A (selects which keyboard row to read)
    // - 0xDC01 = CIA 1 Port B (reads column status - bit is 0 when key pressed)
    
    // Select Row 1 (bit pattern 11111101 = 0xFD)
    // This row contains: UP arrow (bit 1), LEFT arrow (bit 2), HOME (bit 3), DEL (bit 5)
    (*(volatile uint8_t*)0xDC00) = 0xFD;
    uint8_t row1Result = (*(volatile uint8_t*)0xDC01);
    
    // Check UP arrow (bit 1) - move up (dy = -1)
    if (!(row1Result & 0x02)) {
        startMove(s, 0, -1);
    }
    // Check DEL key (bit 5) - used as DOWN arrow (dy = 1)
    else if (!(row1Result & 0x20)) {
        startMove(s, 0, 1);
    }
    // Check LEFT arrow (bit 2) - move left (dx = -1)
    else if (!(row1Result & 0x04)) {
        startMove(s, -1, 0);
    }
    
    // Select Row 2 (bit pattern 11111011 = 0xFB)
    // This row contains: 7, 4, 1, DOWN arrow (bit 2), RIGHT arrow (bit 4), 2, 5, 6
    (*(volatile uint8_t*)0xDC00) = 0xFB;
    uint8_t row2Result = (*(volatile uint8_t*)0xDC01);
    
    // Check RIGHT arrow (bit 2 in column port) - move right (dx = 1)
    if (!(row2Result & 0x04)) {
        startMove(s, 1, 0);
    }
}

// ============================================================================
// waitForSpace - Wait for SPACE key press and release
// Used for debugging and pausing the game
// 
// Steps:
// 1. Select keyboard row 4 (SPACE key is in this row, bit 4)
// 2. Wait while key is NOT pressed (bit HIGH)
// 3. Wait while key IS pressed (bit LOW) - debounce/release
// 4. Reset CIA port to idle
// ============================================================================
void waitForSpace() {
    // 1. Select Row 4 (1110 1111)
    (*(volatile uint8_t*)0xDC00) = 0xEF;

    // 2. Wait while Bit 4 is HIGH (1 means NOT pressed)
    while ((*(volatile uint8_t*)0xDC01) & 0x10);

    // 3. Wait while Bit 4 is LOW (Wait for release/debounce)
    while (!((*(volatile uint8_t*)0xDC01) & 0x10));

    // 4. Reset CIA 1 to idle (Very important to prevent charset glitches)
    (*(volatile uint8_t*)0xDC00) = 0xFF;
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================
int main(void) {
    
    // ============================================================
    // INITIALIZATION PHASE
    // ============================================================
    
    // Initialize the character pool (indices 150-255)
    initPool(); 
    
    // Initialize system (copy charset, set VIC registers, load map)
    initSystem();
    
    // ============================================================
    // TILE PROPERTIES SETUP
    // ============================================================
    // Set tile properties: all tiles start as non-walkable
    memset(tileProps, FLAG_NONE, 256);
    
    // Set tiles 0-6 as walkable paths (FLAG_PATH)
    // These correspond to road/path tiles in the map data
    for (uint8_t i = 0; i <= 6; i++) {
        tileProps[i] = FLAG_PATH;
    }

    // ============================================================
    // WAYPOINT DEFINITIONS
    // ============================================================
    // Define waypoints as coordinate pairs
    static uint8_t waypoint1[] = {2, 2};    // Start point for NPC 1
    static uint8_t waypoint2[] = {7, 11};   // End point for NPC 1
    static uint8_t waypoint3[] = {21, 2};   // (Commented out)
    static uint8_t waypoint4[] = {25, 20};  // (Commented out)

    // Initialize waypoint map (clear all waypoints)
    memset(waypointMap, 0, 1000);
    
    // Set waypoints on the map (will appear green)
    setWaypoint(waypoint1[0], waypoint1[1]); // Set waypoint at (2, 2)
    setWaypoint(waypoint2[0], waypoint2[1]); // Set waypoint at (7, 11)
    setWaypoint(waypoint3[0], waypoint3[1]); // Set waypoint at (21, 2)
    setWaypoint(waypoint4[0], waypoint4[1]); // Set waypoint at (25, 20)
    
    // ============================================================
    // NPC INITIALIZATION
    // ============================================================
    // Clear all NPC structures to zero
    memset(npc, 0, sizeof(npc));

    // NPC 0: Player controlled (white)
    npc[0].x = 10;
    npc[0].y = 10;
    npc[0].charIndex = 48;  // Default settler graphic
    npc[0].color = 1;        // White

    // NPC 3: Random AI (white) - actually uses handleAi
    npc[3].x = 20;
    npc[3].y = 10;
    npc[3].charIndex = 48;
    npc[3].color = 1;        // White

    // NPC 1: Path following (yellow) - patrols between waypoint1 and waypoint2
    npc[1].x = 2;
    npc[1].y = 2;
    npc[1].oldX = 2;
    npc[1].oldY = 2;
    npc[1].charIndex = 48;
    npc[1].color = 7;        // Yellow

    // ============================================================
    // PATH CALCULATION FOR NPC 1
    // ============================================================
    // Calculate forward path: waypoint1 (2,2) → waypoint2 (7,11)
    calculateYellowPathOnRoad(waypoint1[0], waypoint1[1], waypoint2[0], waypoint2[1]);
    
    // Load forward path into NPC 1 (mode 0 = pathActions)
    loadPathToNpc(&npc[1], waypoint2[0], waypoint2[1], (waypoint1[0] * 40) + waypoint1[1], 0);

    // Calculate return path: waypoint2 (7,11) → waypoint1 (2,2)
    calculateYellowPathOnRoad(waypoint2[0], waypoint2[1], waypoint1[0], waypoint1[1]);
    
    // Load return path into NPC 1 (mode 1 = pathBack)
    loadPathToNpc(&npc[1], waypoint1[0], waypoint1[1], (waypoint2[0] * 40) + waypoint2[1], 1);

    // NPC 4 is commented out - would patrol between waypoint3 and waypoint4
    // npc[4].x = 21;
    // npc[4].y = 2;
    // npc[4].oldX = 21;
    // npc[4].oldY = 2;
    // npc[4].charIndex = 48;
    // npc[4].color = 7;
    // calculateYellowPathOnRoad(waypoint3[0], waypoint3[1], waypoint4[0], waypoint4[1]);
    // loadPathToNpc(&npc[4], waypoint4[0], waypoint4[1], (waypoint3[0] * 40) + waypoint3[1], 0);
    // calculateYellowPathOnRoad(waypoint4[0], waypoint4[1], waypoint3[0], waypoint3[1]);
    // loadPathToNpc(&npc[4], waypoint3[0], waypoint3[1], (waypoint4[0] * 40) + waypoint4[1], 1);

    // ============================================================
    // FINAL INITIALIZATION
    // ============================================================
    // Refresh all colors on the screen based on map properties
    refreshAllColors();

    // ============================================================
    // MAIN GAME LOOP
    // ============================================================
    while (1) {
        // Handle player input for NPC 0 (player character)
        handleInput(&npc[0]);

        // Handle path following for NPC 1 (yellow patroller)
        handleNpcPathing(&npc[1]);

        // Handle random AI for NPC 3 (white random mover)
        handleAi(&npc[3]);

        // Update and draw all 5 NPCs (0,1,2,3,4)
        // Note: NPC 2 was never initialized (all zeros), so it's inactive
        // NPC 4 is all zeros due to being commented out
        for (uint8_t i = 0; i < 5; i++) {
            updateSettler(&npc[i]);  // Update position if moving
            drawSettler(&npc[i]);    // Draw at current position
        }
        // debugHandler(); (commented out)
        // waitForSpace(); (commented out)
    }
    
    return 0;  // Never reached
}