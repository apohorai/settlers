#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// --- Hardware Memory Definitions ---
#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

#define POOL_START 150
#define POOL_END   255
#define POOL_SIZE  (POOL_END - POOL_START + 1)

#define FLAG_NONE       0
#define FLAG_PATH       (1 << 0) 

static uint8_t tileProps[256];
static uint8_t waypointMap[1000]; 
static uint8_t pathOverlay[1000]; 
static uint8_t parentMap[1000];   

typedef struct {
    uint8_t x, y, offX, offY, oldX, oldY, charIndex, color;
    int8_t  dirX, dirY;    
    uint8_t stepsRemaining; 
    uint8_t t1[2], t2[2]; // Temporary characters for each NPC
    
    // --- PATH OBJECTS ---
    uint8_t pathActions[64]; 
    uint8_t pathLen;         
    uint8_t pathBack[64];    
    uint8_t backLen;         
    uint8_t currentStep;     
    uint8_t returning;        
} Settler;

typedef struct { uint8_t stack[POOL_SIZE]; int16_t top; } TempPool;
static TempPool charPool;
static Settler npc[5];

// --- Helpers ---

uint8_t getMapColor(uint8_t x, uint8_t y) {
    uint16_t off = (y * 40) + x;
    if (waypointMap[off]) return 5;
    if (pathOverlay[off]) return 7;
    if (tileProps[settlers_map[off]] & FLAG_PATH) return 2;
    return 1;
}

void setWaypoint(uint8_t x, uint8_t y) { if (x < 40 && y < 25) waypointMap[(y * 40) + x] = 1; }

void calculateYellowPathOnRoad(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    static uint16_t queue[400];
    uint16_t head = 0, tail = 0;
    memset(parentMap, 255, 1000);
    memset(pathOverlay, 0, 1000);
    uint16_t start = (y1 * 40) + x1, target = (y2 * 40) + x2;
    queue[tail++] = start;
    parentMap[start] = 4; 
    while (head < tail) {
        uint16_t curr = queue[head++];
        if (curr == target) break;
        uint8_t cx = curr % 40, cy = curr / 40;
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t nx = cx, ny = cy;
            if (i == 0 && cy > 0) ny--; else if (i == 1 && cy < 24) ny++; 
            else if (i == 2 && cx > 0) nx--; else if (i == 3 && cx < 39) nx++; 
            else continue;
            uint16_t next = (ny * 40) + nx;
            if ((tileProps[settlers_map[next]] & FLAG_PATH) && parentMap[next] == 255) {
                parentMap[next] = i; queue[tail++] = next;
            }
        }
    }
    if (parentMap[target] != 255) {
        uint16_t curr = target;
        while (curr != start) {
            pathOverlay[curr] = 1;
            uint8_t cx = curr % 40, cy = curr / 40, dir = parentMap[curr];
            if (dir == 0) cy++; else if (dir == 1) cy--; else if (dir == 2) cx++; else if (dir == 3) cx--; 
            curr = (cy * 40) + cx;
        }
        pathOverlay[start] = 1;
    }
}

void loadPathToNpc(Settler* s, uint8_t targetX, uint8_t targetY, uint16_t startNode, uint8_t mode) {
    uint8_t temp[64]; uint8_t count = 0; uint8_t cx = targetX, cy = targetY;
    while (((cy * 40) + cx) != startNode && count < 64) {
        uint16_t off = (cy * 40) + cx; uint8_t move = parentMap[off];
        if (move == 255) break;
        temp[count++] = move;
        if (move == 0) cy++; else if (move == 1) cy--; else if (move == 2) cx++; else if (move == 3) cx--; 
    }
    if (mode == 0) {
        s->pathLen = count;
        for (uint8_t i = 0; i < count; i++) s->pathActions[i] = temp[(count - 1) - i];
    } else {
        s->backLen = count;
        for (uint8_t i = 0; i < count; i++) s->pathBack[i] = temp[(count - 1) - i];
    }
    s->currentStep = 0;
}

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
uint8_t assignChar() { return (charPool.top >= 0) ? charPool.stack[charPool.top--] : 0; }
void waitVsync() { while ((*(volatile uint8_t*)0xd011) & 0x80); 
    while ((*(volatile uint8_t*)0xd012) != 0xFF); }
void initSystem() {
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    (*(volatile uint8_t*)0xd018) = 0x1C; (*(volatile uint8_t*)0xd020) = 0; (*(volatile uint8_t*)0xd021) = 6;    
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
}
void refreshAllColors() { for (uint8_t y = 0; y < 25; y++) for (uint8_t x = 0; x < 40; x++) COLOR_RAM[(y * 40) + x] = getMapColor(x, y); }
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
void releaseChar(uint8_t c) {
    // Only put it back if it's actually a pool character
    if (c >= POOL_START && c <= POOL_END) {
        // Clear the character bitmap data to prevent garbage accumulation
        memset(CHARSET_DEST + (c << 3), 0, 8);
        charPool.stack[++charPool.top] = c;
    }
}
void drawSettler(Settler* s) {
    // --- STEP 1: DYNAMIC RESOURCE ALLOCATION ---
    // If this NPC doesn't have temporary character slots yet, 
    // grab them from the pool (indices 150-255).
    if (s->t1[0] == 0) s->t1[0] = assignChar(); 
    if (s->t2[0] == 0) s->t2[0] = assignChar();

    // Calculate memory pointers for the source (original NPC look) 
    // and the destinations (the dynamic workspace in the charset).
    uint8_t* src = CHARSET_DEST + (s->charIndex << 3);
    uint8_t* dst1 = CHARSET_DEST + (s->t1[0] << 3); 
    uint8_t* dst2 = CHARSET_DEST + (s->t2[0] << 3);

    // --- STEP 2: PIXEL-SHIFTING LOGIC ---
    
    // Case A: Smooth Horizontal Movement (off_x)
    if (s->offX > 0) {
        // Prepare the background by copying the underlying tile into our temp slots
        prepareTemp(s->x, s->y, s->t1); 
        prepareTemp(s->x + 1, s->y, s->t2);
        
        for (uint8_t i = 0; i < 8; i++) {
            // Shift the source byte across two characters using a 16-bit register
            uint16_t row = (uint16_t)src[i] << (8 - s->offX); 
            dst1[i] |= (uint8_t)(row >> 8);   // Left half of shift
            dst2[i] |= (uint8_t)(row & 0xFF); // Right half of shift
        }
    } 
    // Case B: Smooth Vertical Movement (off_y)
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
    else { 
        prepareTemp(s->x, s->y, s->t1); 
        for (uint8_t i = 0; i < 8; i++) dst1[i] |= src[i]; 
    }

    // --- STEP 3: COLORING ---
    // Set the hardware color RAM for the current screen position
    COLOR_RAM[(s->y * 40) + s->x] = s->color;
}
void restoreTile(uint8_t x, uint8_t y) { uint16_t off = (y * 40) + x; SCREEN_RAM[off] = settlers_map[off]; COLOR_RAM[off] = getMapColor(x, y); }
void startMove(Settler* s, int8_t dx, int8_t dy) {
    if (s->stepsRemaining == 0) { uint8_t nx = s->x + dx, ny = s->y + dy; if (nx >= 39 || ny >= 24 || nx < 1 || ny < 1) return; s->oldX = s->x; s->oldY = s->y; s->dirX = dx; s->dirY = dy; s->stepsRemaining = 8; }
}
void updateSettler(Settler* s) {
    if (s->stepsRemaining > 0) {
        int16_t px = (s->x * 8) + s->offX + s->dirX, py = (s->y * 8) + s->offY + s->dirY;
        s->x = px / 8; s->offX = px % 8; s->y = py / 8; s->offY = py % 8;
        
        if (--s->stepsRemaining == 0) { 
            // The NPC has arrived at its destination tile
            restoreTile(s->oldX, s->oldY); 
            if (s->dirX != 0) restoreTile(s->oldX + 1, s->oldY); 
            if (s->dirY != 0) restoreTile(s->oldX, s->oldY + 1); 
            
            s->dirX = 0; s->dirY = 0; 

            // --- THE MISSING LINK ---
            // Give the characters back to the pool so other NPCs can use them
            releaseChar(s->t1[0]); 
            releaseChar(s->t2[0]);
            
            // Reset the NPC's temp variables so it knows to ask for new ones next time
            s->t1[0] = 0; 
            s->t2[0] = 0;
        }
    }
}
void handleAi(Settler* s) {
    if (s->stepsRemaining > 0) return;
    if ((rand() % 100) < 5) { int8_t r = rand() % 4; if (r == 0) startMove(s, 0, -1); else if (r == 1) startMove(s, 0, 1); else if (r == 2) startMove(s, -1, 0); else startMove(s, 1, 0); }
}
void handleNpcPathing(Settler* s) {
    if (s->stepsRemaining > 0) return;
    uint8_t max = s->returning ? s->backLen : s->pathLen;
    if (s->currentStep >= max) {
        s->returning = !s->returning;
        s->currentStep = 0;
        return;
    }
    uint8_t move = s->returning ? s->pathBack[s->currentStep++] : s->pathActions[s->currentStep++];
    if (move == 0) startMove(s, 0, -1); else if (move == 1) startMove(s, 0, 1);
    else if (move == 2) startMove(s, -1, 0); else if (move == 3) startMove(s, 1, 0);
}

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
int main(void) {
    
    initPool(); // Initialize the character pool
  
    initSystem();
    

   // Set tile properties
   memset(tileProps, FLAG_NONE, 256);
   for (uint8_t i = 0; i <= 6; i++) {
       tileProps[i] = FLAG_PATH;
   }

   // Define waypoints as arrays
   static uint8_t waypoint1[] = {2, 2};
   static uint8_t waypoint2[] = {7, 11};
      static uint8_t waypoint3[] = {21, 2};
   static uint8_t waypoint4[] = {25, 20};

   // Initialize waypoint map
   memset(waypointMap, 0, 1000);
   setWaypoint(waypoint1[0], waypoint1[1]); // Set waypoint at (2, 2)
   setWaypoint(waypoint2[0], waypoint2[1]); // Set waypoint at (7, 11)
   setWaypoint(waypoint3[0], waypoint3[1]); // Set waypoint at (2, 2)
   setWaypoint(waypoint4[0], waypoint4[1]); // Set waypoint at (7, 11)
   // Initialize NPCs
   memset(npc, 0, sizeof(npc));

   // NPC 0: Player controlled
   npc[0].x = 10;
   npc[0].y = 10;
   npc[0].charIndex = 48;
   npc[0].color = 1;

    npc[3].x = 20;
   npc[3].y = 10;
   npc[3].charIndex = 48;
   npc[3].color = 1;
 

   // NPC 1: Controlled by player
   npc[1].x = 2;
   npc[1].y = 2;
   npc[1].oldX = 2;
   npc[1].oldY = 2;
   npc[1].charIndex = 48;
   npc[1].color = 7;



  calculateYellowPathOnRoad(waypoint1[0], waypoint1[1], waypoint2[0], waypoint2[1]);
  loadPathToNpc(&npc[1], waypoint2[0], waypoint2[1], (waypoint1[0] * 40) + waypoint1[1], 0);

     
   calculateYellowPathOnRoad(waypoint2[0], waypoint2[1], waypoint1[0], waypoint1[1]);
  loadPathToNpc(&npc[1], waypoint1[0], waypoint1[1], (waypoint2[0] * 40) + waypoint2[1], 1);

//     npc[4].x = 21;
//    npc[4].y = 2;
//    npc[4].oldX = 21;
//    npc[4].oldY = 2;
//    npc[4].charIndex = 48;
//    npc[4].color = 7;
 
//    calculateYellowPathOnRoad(waypoint3[0], waypoint3[1], waypoint4[0], waypoint4[1]);
//    loadPathToNpc(&npc[4], waypoint4[0], waypoint4[1], (waypoint3[0] * 40) + waypoint3[1], 0);

 
    
//calculateYellowPathOnRoad(waypoint4[0], waypoint4[1], waypoint3[0], waypoint3[1]);
//loadPathToNpc(&npc[4], waypoint3[0], waypoint3[1], (waypoint4[0] * 40) + waypoint4[1], 1);

   // Refresh all colors on the screen

   refreshAllColors();

   // NPC 2: Random movement
   npc[2].x = 20;
   npc[2].y = 18;
   npc[2].charIndex = 48;
   npc[2].color = 3;

   // Main game loop
   
   while (1) {

   //    waitVsync();

       // Handle input for NPC 0
       handleInput(&npc[0]);
        //handleInput(&npc[4]);
       // Handle NPC pathing for NPC 1
       handleNpcPathing(&npc[1]);

       // Handle AI for NPC 2
       handleAi(&npc[3]);

       // Update and draw all NPCs
       for (uint8_t i = 0; i < 5; i++) {
           updateSettler(&npc[i]);
           drawSettler(&npc[i]);
       }
       //debugHandler();

   //    waitForSpace();
  
   }
   return 0;
}
