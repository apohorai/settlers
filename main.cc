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

// --- Terrain Properties ---
#define FLAG_NONE       0
#define FLAG_PATH       (1 << 0) 

static uint8_t tile_props[256];
static uint8_t waypoint_map[1000]; 
static uint8_t path_overlay[1000]; 
static uint8_t parent_map[1000];   

typedef struct {
    uint8_t x, y, off_x, off_y, old_x, old_y, char_index, color;
    int8_t  dir_x, dir_y;    
    uint8_t steps_remaining; 
    uint8_t t1, t2; 
    
    // --- PATH OBJECT ---
    uint8_t path_actions[64]; 
    uint8_t path_len;         
    uint8_t current_step;     
} Settler;

typedef struct {
    uint8_t stack[POOL_SIZE];
    int16_t top;
} TempPool;

static TempPool charPool;
static Settler npc[3];

// --- Helpers ---

uint8_t get_map_color(uint8_t x, uint8_t y) {
    uint16_t off = (y * 40) + x;
    if (waypoint_map[off]) return 5;
    if (path_overlay[off]) return 7;
    uint8_t tile = settlers_map[off];
    if (tile_props[tile] & FLAG_PATH) return 2;
    return 1;
}

void set_waypoint(uint8_t x, uint8_t y) {
    if (x < 40 && y < 25) waypoint_map[(y * 40) + x] = 1;
}

void calculate_yellow_path_on_road(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    static uint16_t queue[1000];
    uint16_t head = 0, tail = 0;
    memset(parent_map, 255, 1000);
    memset(path_overlay, 0, 1000);
    uint16_t start = (y1 * 40) + x1, target = (y2 * 40) + x2;
    queue[tail++] = start;
    parent_map[start] = 4; 
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
            if ((tile_props[settlers_map[next]] & FLAG_PATH) && parent_map[next] == 255) {
                parent_map[next] = i; queue[tail++] = next;
            }
        }
    }
    if (parent_map[target] != 255) {
        uint16_t curr = target;
        while (curr != start) {
            path_overlay[curr] = 1;
            uint8_t cx = curr % 40, cy = curr / 40, dir = parent_map[curr];
            if (dir == 0) cy++; else if (dir == 1) cy--; else if (dir == 2) cx++; else if (dir == 3) cx--; 
            curr = (cy * 40) + cx;
        }
        path_overlay[start] = 1;
    }
}

// Convert parent_map to forward sequence in Settler object
void load_path_to_npc(Settler* s, uint8_t target_x, uint8_t target_y, uint16_t start_node) {
    uint8_t temp[64];
    uint8_t count = 0;
    uint8_t cx = target_x, cy = target_y;
    while (((cy * 40) + cx) != start_node && count < 64) {
        uint16_t off = (cy * 40) + cx;
        uint8_t move = parent_map[off];
        if (move == 255) break;
        temp[count++] = move;
        if (move == 0) cy++; else if (move == 1) cy--; else if (move == 2) cx++; else if (move == 3) cx--; 
    }
    s->path_len = count;
    s->current_step = 0;
    for (uint8_t i = 0; i < count; i++) s->path_actions[i] = temp[(count - 1) - i];
}

void init_pool() {
    charPool.top = -1;
    for (int16_t i = POOL_START; i <= POOL_END; i++) charPool.stack[++charPool.top] = (uint8_t)i;
}

uint8_t assign_char() { return (charPool.top >= 0) ? charPool.stack[charPool.top--] : 0; }

void wait_vsync() {
    while ((*(volatile uint8_t*)0xd011) & 0x80); 
    while ((*(volatile uint8_t*)0xd012) != 0xFF);
}

void init_system() {
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    (*(volatile uint8_t*)0xd018) = 0x1C; 
    (*(volatile uint8_t*)0xd020) = 0; (*(volatile uint8_t*)0xd021) = 6;    
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
}

void refresh_all_colors() {
    for (uint8_t y = 0; y < 25; y++) {
        for (uint8_t x = 0; x < 40; x++) COLOR_RAM[(y * 40) + x] = get_map_color(x, y);
    }
}

void prepare_temp(uint8_t x, uint8_t y, uint8_t t_idx) {
    if (x >= 40 || y >= 25 || t_idx == 0) return;
    uint16_t offset = (y * 40) + x;
    memcpy(CHARSET_DEST + (t_idx << 3), CHARSET_DEST + (settlers_map[offset] << 3), 8);
    SCREEN_RAM[offset] = t_idx;
}

void draw_settler(Settler* s) {
    if (s->t1 == 0) s->t1 = assign_char();
    if (s->t2 == 0) s->t2 = assign_char();
    uint8_t* src = CHARSET_DEST + (s->char_index << 3);
    uint8_t* dst1 = CHARSET_DEST + (s->t1 << 3), *dst2 = CHARSET_DEST + (s->t2 << 3);
    if (s->off_x > 0) {
        prepare_temp(s->x, s->y, s->t1); prepare_temp(s->x + 1, s->y, s->t2);
        for (uint8_t i = 0; i < 8; i++) {
            uint16_t row = (uint16_t)src[i] << (8 - s->off_x);
            dst1[i] |= (uint8_t)(row >> 8); dst2[i] |= (uint8_t)(row & 0xFF);
        }
    } else if (s->off_y > 0) {
        prepare_temp(s->x, s->y, s->t1); prepare_temp(s->x, s->y + 1, s->t2);
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t ty = i + s->off_y;
            if (ty < 8) dst1[ty] |= src[i]; else dst2[ty - 8] |= src[i];
        }
    } else {
        prepare_temp(s->x, s->y, s->t1);
        for (uint8_t i = 0; i < 8; i++) dst1[i] |= src[i];
    }
    COLOR_RAM[(s->y * 40) + s->x] = s->color;
}

void restore_tile(uint8_t x, uint8_t y) {
    uint16_t off = (y * 40) + x;
    SCREEN_RAM[off] = settlers_map[off];
    COLOR_RAM[off] = get_map_color(x, y);
}

void start_move(Settler* s, int8_t dx, int8_t dy) {
    if (s->steps_remaining == 0) { 
        uint8_t nx = s->x + dx, ny = s->y + dy;
        if (nx >= 39 || ny >= 24 || nx < 1 || ny < 1) return;
        s->old_x = s->x; s->old_y = s->y; s->dir_x = dx; s->dir_y = dy; s->steps_remaining = 8; 
    }
}

void update_settler(Settler* s) {
    if (s->steps_remaining > 0) {
        int16_t px = (s->x * 8) + s->off_x + s->dir_x, py = (s->y * 8) + s->off_y + s->dir_y;
        s->x = px / 8; s->off_x = px % 8; s->y = py / 8; s->off_y = py % 8;
        if (--s->steps_remaining == 0) {
            restore_tile(s->old_x, s->old_y);
            if (s->dir_x != 0) restore_tile(s->old_x + 1, s->old_y);
            if (s->dir_y != 0) restore_tile(s->old_x, s->old_y + 1);
            s->dir_x = 0; s->dir_y = 0;
        }
    }
}

void handle_ai(Settler* s) {
    if (s->steps_remaining > 0) return;
    if ((rand() % 100) < 5) {
        int8_t r = rand() % 4;
        if (r == 0) start_move(s, 0, -1); else if (r == 1) start_move(s, 0, 1);
        else if (r == 2) start_move(s, -1, 0); else start_move(s, 1, 0);
    }
}

void handle_npc_pathing(Settler* s) {
    if (s->steps_remaining > 0 || s->current_step >= s->path_len) return;
    uint8_t move = s->path_actions[s->current_step++];
    if (move == 0) start_move(s, 0, -1); else if (move == 1) start_move(s, 0, 1);
    else if (move == 2) start_move(s, -1, 0); else if (move == 3) start_move(s, 1, 0);
}

void handle_input(Settler* s) {
    if (s->steps_remaining > 0) return; 
    (*(volatile uint8_t*)0xDC00) = 0xFD; 
    uint8_t r = (*(volatile uint8_t*)0xDC01);
    if (!(r & 0x02)) start_move(s, 0, -1); else if (!(r & 0x20)) start_move(s, 0, 1);
    else if (!(r & 0x04)) start_move(s, -1, 0);
    else { (*(volatile uint8_t*)0xDC00) = 0xFB; if (!((*(volatile uint8_t*)0xDC01) & 0x04)) start_move(s, 1, 0); }
}

int main(void) {
    init_pool(); init_system();
    memset(tile_props, FLAG_NONE, 256);
    for(uint8_t i = 0; i <= 6; i++) tile_props[i] = FLAG_PATH;
    memset(waypoint_map, 0, 1000);
    set_waypoint(2, 2); set_waypoint(7, 11);
    calculate_yellow_path_on_road(2, 2, 7, 11);
    refresh_all_colors();

    npc[0] = {10, 10, 0, 0, 10, 10, 48, 1, 0, 0, 0, 0, 0, {0}, 0, 0}; 
    npc[1] = {2, 2, 0, 0, 2, 2, 48, 7, 0, 0, 0, 0, 0, {0}, 0, 0}; 
    npc[2] = {20, 18, 0, 0, 20, 18, 48, 3, 0, 0, 0, 0, 0, {0}, 0, 0}; 

    load_path_to_npc(&npc[1], 7, 11, (2 * 40) + 2);

    while (1) {
        wait_vsync();
        handle_input(&npc[0]); 

        // --- T DUMP HANDLER ---
        (*(volatile uint8_t*)0xDC00) = 0xFB; 
        if (!((*(volatile uint8_t*)0xDC01) & 0x40)) { 
            __asm__("sei"); 
            (*(volatile uint8_t*)0xd011) = 0x1B; (*(volatile uint8_t*)0xd018) = 0x14; 
            (*(volatile uint8_t*)0xd020) = 1; (*(volatile uint8_t*)0xd021) = 0;    
            for(uint16_t i = 0; i < 1000; i++) { SCREEN_RAM[i] = 0x20; COLOR_RAM[i] = 0x01; }
            for(uint8_t i = 0; i < npc[1].path_len; i++) {
                uint8_t move = npc[1].path_actions[i];
                if (move == 0) SCREEN_RAM[i] = 21; else if (move == 1) SCREEN_RAM[i] = 4;
                else if (move == 2) SCREEN_RAM[i] = 12; else if (move == 3) SCREEN_RAM[i] = 18;
            }
            while(1); 
        }

        handle_npc_pathing(&npc[1]);
        for(uint8_t i = 0; i < 3; i++) {
            if (i != 0 && i != 1) handle_ai(&npc[i]); 
            update_settler(&npc[i]); draw_settler(&npc[i]);
        }
    }
    return 0;
}