#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "data.h"

/**
	TODO
		- Create a version of set_bkg_map that's row-based instead of column-based (it can draw columns, but not very efficiently)
		- Adapt the routines to store the background map as columns instead of rows (Depends on the above)
		- Create a routine to read from the screen's background map
		- The routines above will allow the drawing to be more efficient
		- Cleanup the first-person view drawing routine
		- The coordinate system is al f***ed up.
		- Speed up the coordinate translation routines
 **/

#define VIEW_WIDTH (32)
#define VIEW_HEIGHT (12)

#define MINIMAP_WIDTH (7)
#define MINIMAP_HEIGHT (7)

#define WALL_OFFS_1 (16 * 12)
#define WALL_OFFS_2 (WALL_OFFS_1 + 8 * 8)
#define WALL_OFFS_3 (WALL_OFFS_2 + 4 * 4)

#define SIDE_OFFS_0 (0)
#define SIDE_OFFS_1 (8 * 12)
#define SIDE_OFFS_2 (SIDE_OFFS_1 + 4 * 12)

#define WALL_TOP_1 (2 << 5)
#define WALL_TOP_2 (4 << 5)
#define WALL_TOP_3 (5 << 5)

#define BLOCK_SHIFT 5

#define DIR_NORTH 0
#define DIR_EAST 1
#define DIR_SOUTH 2
#define DIR_WEST 3

#define BKG_PALETTE 0x100

#define MAP_WIDTH (24)
#define MAP_HEIGHT (24)

#define SIZE_FULL (0)
#define SIZE_HALF (1)
#define SIZE_QUARTER (2)

#define set_bkg_map(src, x, y, width, height) SMS_loadTileMapArea(x, y, src, width, height);

unsigned char get_map(int x, int y);

const unsigned int *test_map_2 = test_map;
const unsigned int *test_bkg_2 = test_bkg;

char map[32][32];
unsigned int bkg[VIEW_WIDTH*VIEW_HEIGHT];
	
struct player {
	int x, y;
	int dir;
} player;

struct monster {
	int x, y;	
	unsigned char move_ctl;
	
	unsigned int anim, anim_y;
	
	unsigned char palette[16];
	char plt_frame_1, plt_frame_2;
} monster;

const unsigned char monster_pal_eye_anim[] = {
  0x05, 0x06, 0x0A, 0x0B, 0x0F, 0x0F, 0x1F, 0x1F, 0x2F, 0x3F, 
  0x2F, 0x1F, 0x1F, 0x0F, 0x0F, 0x0B, 0x0A, 0x06
};

const unsigned char monster_pal_brain_anim[] = {
  0x01, 0x02, 0x02, 0x03, 0x03, 0x17, 0x2B,
  0x17, 0x03, 0x03, 0x02
};

const char sidewall_offs1[] = {
	0, 0, 0, 0,	0, 0, 1, 1
};

const char sidewall_offs2[] = {
	2, 2, 3, 3
};

void rotate_dir(int *x, int *y, int dir) {
	int tmp;

	switch (dir) {
	case DIR_NORTH:
		*y = -(*y);
		break;

	case DIR_SOUTH:
		*x = -(*x);
		break;

	case DIR_EAST:
		tmp = *x;
		*x = *y;
		*y = tmp;
		break;

	case DIR_WEST:
		tmp = *x;
		*x = -(*y);
		*y = -tmp;
		break;
	}
}

int walk_dir(int *x, int *y, int dx, int dy, int dir) {
	rotate_dir(&dx, &dy, dir);

	if (!get_map(*x + dx, *y + dy)) {
		*x += dx;
		*y += dy;
		return 1;
	}
	return 0;
}

int walk_spr_dir(int *x, int *y, int dx, int dy, int dir) {
	rotate_dir(&dx, &dy, dir);

	if (!get_map((*x + dx) >> BLOCK_SHIFT, (*y + dy) >> BLOCK_SHIFT)) {
		*x += dx;
		*y += dy;
		return 1;
	}
	return 0;
}

unsigned char get_map(int x, int y) {
	if (x < 0 || x >= MAP_WIDTH ||
		y < 0 || y >= MAP_HEIGHT) {
		return 0;
	}
	return map[y][x];
}

unsigned char get_map_r(int x, int y, int rx, int ry, int dir) {
	rotate_dir(&rx, &ry, dir);

	rx += x;
	ry += y;

	return get_map(rx, ry);
}

void draw_view(int x, int y, int dir, unsigned int *bkg) { // TODO: Some extensive code cleanup. There's too much replicated code below.  =|
	int i, j;
	int ofs, h;
	unsigned int *top, *p, *p2;

	int rx, ry;
	int tx;
	unsigned int mask;
	int found, ok;

	top = test_flr;
	p = bkg;
	for (i = 0; i != VIEW_HEIGHT; i++) {
		for (j = 0, p2 = p + (VIEW_WIDTH-1); j != (VIEW_WIDTH >> 1); j++, p2--) {
			*p = *top ^ BKG_PALETTE;
			*p2 = *top ^ (BKG_PALETTE | TILE_FLIPPED_X);

			top++;
			p++;
		}
		p += (VIEW_WIDTH >> 1);
	}

	for (i = 0, top = bkg; i != VIEW_WIDTH; i++, top++) {
		ry = 0;

		found = 0;

		// 0 block depth side wall
		if (!found) {
			if (i < 16) {
				rx = ((i - 8) >> 5);
			} else {
				rx = ((i + 8) >> 5);
			}
			if (get_map_r(x, y, rx, ry, dir)) {
				tx = i & 0x0F;
				mask = BKG_PALETTE;
				if (i > 16) {
					tx = 0x0F - tx;
					mask |= TILE_FLIPPED_X;
				}
				ofs = sidewall_offs1[tx];
				h = VIEW_HEIGHT - (ofs << 1);
				tx = (tx << 3) + (tx << 2); // Same as tx *= 12;

				for (j = 0, p = top + (ofs << 5), p2 = test_bkg_2 + SIDE_OFFS_0 + tx + ofs; j != h; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}

		// 1 block depth
		if (!found) {
			ry++;
			rx = ((i - 8) >> 4);
			if (get_map_r(x, y, rx, ry, dir)) {
				tx = (i - 8) & 0x0F;
				mask = BKG_PALETTE;
				if (tx & 0x08) {
					tx = 0x0F - tx;
					mask |= TILE_FLIPPED_X;
				}
				tx <<= 3;

				for (j = 0, p = top + WALL_TOP_1, p2 = test_bkg_2 + WALL_OFFS_1 + tx; j != 8; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}

		// 1 block depth side wall
		if (!found) {
			if (i < 16) {
				rx = ((i - 8) >> 4) - 1;
				ok = !(i & 0x04);
			} else {
				rx = ((i + 8) >> 4);
				ok = (i & 0x04);
			}
			if (ok && get_map_r(x, y, rx, ry, dir)) {
				tx = i & 0x07;
				mask = BKG_PALETTE;
				if (i > 16) {
					tx = 0x07 - tx;
					mask |= TILE_FLIPPED_X;
				}
				ofs = sidewall_offs2[tx];
				h = VIEW_HEIGHT - (ofs << 1);
				tx = (tx << 3) + (tx << 2); // Same as tx *= 12;

				for (j = 0, p = top + (ofs << 5), p2 = test_bkg_2 + SIDE_OFFS_1 + tx + ofs; j != h; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}

		// 2 blocks depth
		if (!found) {
			ry++;
			rx = ((i - 12) >> 3);
			if (get_map_r(x, y, rx, ry, dir)) {
				tx = (i - 12) & 0x07;
				mask = BKG_PALETTE;
				if (tx & 0x04) {
					tx = 0x07 - tx;
					mask |= TILE_FLIPPED_X;
				}
				tx <<= 2;

				for (j = 0, p = top + WALL_TOP_2, p2 = test_bkg_2 + WALL_OFFS_2 + tx; j != 4; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}

		// 2 blocks depth side wall
		if (!found) {
			if (i < 16) {
				rx = ((i - 14) >> 3);
				ok = !(i & 0x02);
			} else {
				rx = ((i - 10) >> 3);
				ok = (i & 0x02);
			}
			if (ok && get_map_r(x, y, rx, ry, dir)) {
				tx = i & 0x03;
				mask = BKG_PALETTE;
				if (i > 16) {
					tx = 0x03 - tx;
					mask |= TILE_FLIPPED_X;
				}
				ofs = 4;
				h = VIEW_HEIGHT - (ofs << 1);
				tx = (tx << 3) + (tx << 2); // Same as tx *= 12;

				for (j = 0, p = top + (ofs << 5), p2 = test_bkg_2 + SIDE_OFFS_2 + tx + ofs; j != h; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}

		// 3 blocks depth
		if (!found) {
			ry++;
			rx = ((i - 14) >> 2);
			if (get_map_r(x, y, rx, ry, dir)) {
				tx = (i - 2) & 0x03;
				mask = BKG_PALETTE;
				if (tx & 0x02) {
					tx = 0x03 - tx;
					mask |= TILE_FLIPPED_X;
				}
				tx <<= 1;

				for (j = 0, p = top + WALL_TOP_3, p2 = test_bkg_2 + WALL_OFFS_3 + tx; j != 2; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}
	}
}

void draw_mini_map(int x, int y) {
	int min_x = x - (MINIMAP_WIDTH >> 1);
	int min_y = y - (MINIMAP_HEIGHT >> 1);
	unsigned int buffer[MINIMAP_WIDTH];
	
	for (int i = 0; i != MINIMAP_HEIGHT; i++) {
		for (int j = 0; j != MINIMAP_WIDTH; j++) {
			buffer[j] = get_map(min_x + j, min_y + i) ? 266 : 256;
		}

		set_bkg_map(buffer, 32 - MINIMAP_WIDTH - 1, i + 16, MINIMAP_WIDTH, 1);
	}
}

void fade_bkg(unsigned int *bg1, unsigned int *bg2, int fade) {
	int i, j;
	unsigned int *p1, *p2, *p3;

	if (fade == 3) {
		for (i = 0, p1 = bg1 + VIEW_WIDTH - 1, p2 = bg2; i != VIEW_WIDTH - 2; i += 2, p1 -= 2) {
			SMS_waitForVBlank();
			for (j = 0, p3 = p1; j != VIEW_HEIGHT; j++, p3 += VIEW_WIDTH) {
				set_bkg_map(p3, 0, j + 1, i + 1, 1);
			}
			for (j = 0, p3 = p2; j != VIEW_HEIGHT; j++, p3 += VIEW_WIDTH) {
				set_bkg_map(p3, i, j + 1, VIEW_WIDTH - i, 1);
			}
		}
	} else if (fade == 4) {
		for (i = VIEW_WIDTH - 2, p1 = bg2, p2 = bg1; i; i -= 2, p1 += 2) {
			SMS_waitForVBlank();
			for (j = 0, p3 = p1; j != VIEW_HEIGHT; j++, p3 += VIEW_WIDTH) {
				set_bkg_map(p3, 0, j + 1, i + 1, 1);
			}
			for (j = 0, p3 = p2; j != VIEW_HEIGHT; j++, p3 += VIEW_WIDTH) {
				set_bkg_map(p3, i, j + 1, VIEW_WIDTH - i, 1);
			}
		}
	}

	set_bkg_map(bg1, 0, 1, VIEW_WIDTH, VIEW_HEIGHT);
}

void generate_map() {
	int x, y;
	int dx, dy;
	int dx2, dy2;
	
	// Fills the map with ones.
	for (y = 0; y != MAP_HEIGHT; y++) {
		for (x = 0; x != MAP_WIDTH; x++) {
			map[y][x] = 1;
		}
	}

	// Puts a hole on every other coordinate.
	for (y = 1; y < MAP_HEIGHT - 1; y += 2) {
		for (x = 1; x < MAP_WIDTH - 1; x += 2) {
			// Put a hole there
			map[y][x] = 0;
			
			// Dig a tunnel in a random direction
			dx = dx2 = x;
			dy = dy2 = y;
			switch (rand() & 0x03) {
			case DIR_NORTH: dy--; dy2 -= 2; break;
			case DIR_EAST: dx++; dx2 += 2; break;
			case DIR_SOUTH: dy++; dy2 += 2; break;
			case DIR_WEST: dx--; dx2 -= 2; break;
			}
			
			// Dig the tunnel
			if (dx2 >= 0 && dx2 < MAP_WIDTH &&
				dy2 >= 0 && dy2 < MAP_HEIGHT) {
				map[dy][dx] = 0;
				map[dy2][dx2] = 0;
			}
		}
	}
	
	char found_unreachable = 1;
	
	while (found_unreachable) {
		found_unreachable = 0;
		
		// Flood fills to find reachable cells in the map
		map[1][1] = 2;
		char expanded = 1;
		while (expanded) {
			expanded = 0;		
			for (y = 1; y < MAP_HEIGHT - 1; y += 2) {
				for (x = 1; x < MAP_WIDTH - 1; x += 2) {
					
					// If this one is reachable, checks neighbouring cells to see if there's anything that can be reached further
					if (map[y][x] == 2) {
						for (char dir = 0; dir <= DIR_WEST; dir++) {
							dx = dx2 = x;
							dy = dy2 = y;
							switch (dir) {
							case DIR_NORTH: dy--; dy2 -= 2; break;
							case DIR_EAST: dx++; dx2 += 2; break;
							case DIR_SOUTH: dy++; dy2 += 2; break;
							case DIR_WEST: dx--; dx2 -= 2; break;
							}
							
							if (dx2 >= 0 && dx2 < MAP_WIDTH &&
								dy2 >= 0 && dy2 < MAP_HEIGHT &&
								!map[dy][dx] && map[dy2][dx2] != 2) {
								// Found a reachable, but unmarked cell. Mark it.
								map[dy2][dx2] = 2;
								expanded = 1;
							}
						}
					}
					
				}
			}
		}

		// For each unreachable cell, checks if there are unreachable neighbors
		for (y = 1; y < MAP_HEIGHT - 1; y += 2) {
			for (x = 1; x < MAP_WIDTH - 1; x += 2) {
				
				// If this one is reachable, checks neighbouring cells to see if there's any unreachable neighbor
				if (map[y][x] == 2) {
					expanded = 0;
					for (char tries = 3; tries && !expanded; tries--) {
						dx = dx2 = x;
						dy = dy2 = y;
						switch (rand() & 0x03) {
						case DIR_NORTH: dy--; dy2 -= 2; break;
						case DIR_EAST: dx++; dx2 += 2; break;
						case DIR_SOUTH: dy++; dy2 += 2; break;
						case DIR_WEST: dx--; dx2 -= 2; break;
						}
						
						if (dx2 >= 0 && dx2 < MAP_WIDTH &&
							dy2 >= 0 && dy2 < MAP_HEIGHT &&
							map[dy][dx] && !map[dy2][dx2]) {
							// Found a unreachable one; tunnel and mark it.
							map[dy][dx] = 0;
							map[dy2][dx2] = 2;
							expanded = 1;
							found_unreachable = 1;
						}
					}
				}
				
			}
		}
	}

	// Puts a hole on every other coordinate.
	for (y = 1; y < MAP_HEIGHT - 1; y += 2) {
		for (x = 1; x < MAP_WIDTH - 1; x += 2) {
			map[y][x] = 0;
		}
	}
	
	// Generate exit
	if (rand() & 1) {
		// Exit is on the right side
		y = 1 + ((rand() % ((MAP_HEIGHT - 2) >> 1)) << 1);
		for (x = MAP_WIDTH - 1; map[y][x]; x--) {
			map[y][x] = 0;
		}
	} else {
		// Exit is on the bottom side
		x = 1 + ((rand() % ((MAP_WIDTH - 2) >> 1)) << 1);
		for (y = MAP_HEIGHT - 1; map[y][x]; y--) {
			map[y][x] = 0;
		}
	}
	
	player.x = 1;
	player.y = 1;
	player.dir = DIR_SOUTH;
	
	monster.x = MAP_WIDTH - 3;
	monster.y = MAP_HEIGHT - 3;
}

void draw_meta_sprite(int x, int y, int w, int h, unsigned char tile) {
	for (char i = h; i; i--) {
		int sx = x;
		for (char j = w; j; j--) {
			SMS_addSprite(sx, y, tile);
			sx += 8;
			tile += 2;
		}
		y += 16;
	}
}

void animate_monster() {
	monster.anim++;

	if (monster.plt_frame_1 >= sizeof(monster_pal_eye_anim)) {
	  monster.plt_frame_1 = 0;
	}
	if (monster.anim & 0x01) {
		monster.plt_frame_1++;
	}
	
	if (monster.plt_frame_2 >= sizeof(monster_pal_brain_anim)) {
	  monster.plt_frame_2 = 0;
	}
	if (!(monster.anim & 0x03)) {
		monster.plt_frame_2++;
	}

	unsigned int delta = (monster.anim >> 3);	
	monster.anim_y = delta & 0x08 ? delta & 0x07 : 7 - (delta & 0x07);
	
	memcpy(monster.palette, monster_full_palette_bin, 16);
	monster.palette[15] = monster_pal_eye_anim[monster.plt_frame_1];
	monster.palette[13] = monster_pal_brain_anim[monster.plt_frame_2];
}

void draw_monster_size(char size) {
	animate_monster();	
	switch (size) {
	case SIZE_FULL:
		draw_meta_sprite(
			(256 - 56) / 2, 
			8 + monster.anim_y, 
			7, 5, 2);
		break;
		
	case SIZE_HALF:
		draw_meta_sprite(
			(256 - 24) / 2, 
			8 + (VIEW_HEIGHT * 8 - 48) / 2 + (monster.anim_y >> 1), 
			3, 3, 72);
		break;
	
	case SIZE_QUARTER:
		draw_meta_sprite(
			(256 - 16) / 2, 
			8 + (VIEW_HEIGHT * 8 - 32) / 2 + (monster.anim_y >> 2), 
			2, 2, 90);
		break;
	}
}

draw_monster() {
	// Check player's line of sight to verify if it can see the monster
	for (int dist = 1; dist < 4; dist++) {
		int x = 0;
		int y = dist;		
		rotate_dir(&x, &y, player.dir);
		
		x += player.x;
		y += player.y;
		
		if (get_map(x, y)) {
			// Found a wall: abort.
			return;
		}
		
		if (monster.x == x && monster.y == y) {
			// Found the monster: draw it.
			draw_monster_size(dist - 1);
			return;
		}
	}
}

move_monster() {
	// Move at 66% of player's speed.
	monster.move_ctl += 256 / 3;
	if (monster.move_ctl < 256 * 2 / 3) {	
		int tx = monster.x;
		int ty = monster.y;
		char tried = 0;
		char moved = 0;
		
		// Try to move in the X axis
		if (player.x < monster.x) {
			tx--;
			tried = 1;
		} else if (player.x > monster.x) {
			tx++;
			tried = 1;
		}
		
		if (tried) {
			if (get_map(tx, ty)) {
				// Hit a wall: abort.
				tx = monster.x;
				tried = 0;
			} else {
				// No wall: move there.
				moved = 1;
			}
		}
		
		if (!moved) {
			// Try to move in the Y axis
			if (player.y < monster.y) {
				ty--;
				tried = 1;
			} else if (player.y > monster.y) {
				ty++;
				tried = 1;
			}
			
			if (!get_map(tx, ty)) {
				// No wall: move there.
				moved = 1;
			}
		}
		
		if (moved) {
			// Can move: do it.
			monster.x = tx;
			monster.y = ty;
		}
	}
}

void fade_to_red() {
	unsigned char palettes[2][16];
	char phase = 0;
	
	memcpy(palettes[0], test_pal, 16);
	memcpy(palettes[1], monster_full_palette_bin, 16);

	while (phase < 2) {
		char colorsComplete = 0;
		
		for (int i = 0; i != 2; i++) {
			for (int j = 0; j != 16; j++) {
				if (phase == 0) {
					unsigned char red = palettes[i][j] & 0x03;
					red++;
					if (red > 0x03) {
						red = 0x03;
						colorsComplete++;
					}
					
					palettes[i][j] = palettes[i][j] & 0x3C | red;
				} else {
					unsigned char red = palettes[i][j] & 0x03;
					unsigned char green = (palettes[i][j] & 0x0C) >> 2;
					unsigned char blue = (palettes[i][j] & 0x30) >> 4;
					
					if (!green && !blue) {
						colorsComplete++;						
					}
					
					if (green) green--;
					if (blue) blue--;
					
					palettes[i][j] = red | (green << 2) | (blue << 4);
				}
			}
			
			SMS_waitForVBlank();	
			SMS_loadBGPalette(palettes[0]);
			SMS_loadSpritePalette(palettes[1]);
		}
		
		if (colorsComplete == 32) {
			phase++;
		}		
	}
	
	/*
	SMS_loadBGPalette(test_pal);
	SMS_loadSpritePalette(monster_full_palette_bin);
	*/
}

void display_debug_info() {
	SMS_setNextTileatXY(1, 17);
	printf("Player X %d, Y %d    \n", player.x, player.y);
	SMS_setNextTileatXY(1, 18);
	printf("Monster X %d, Y %d    ", monster.x, monster.y);
}

void main() {
	int walked = -1;
	int player_moved = 0;
	int tmr = 0;
	int sprnum;
	int joy;

	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode (SPRITEMODE_TALL);
	
	SMS_loadBGPalette(test_pal);
	SMS_loadSpritePalette(monster_full_palette_bin);

	SMS_loadTiles(test_til, 256, test_til_size);
	
	SMS_load1bppTiles(font_1bpp, 320, font_1bpp_size, 0, 1);
	SMS_configureTextRenderer(320 - 32);
	
	SMS_loadPSGaidencompressedTiles(monster_full_tiles_psgcompr, 2);
	SMS_loadPSGaidencompressedTiles(monster_half_tiles_psgcompr, 72);
	SMS_loadPSGaidencompressedTiles(monster_quarter_tiles_psgcompr, 90);
		
	SMS_initSprites();
	draw_monster_size(SIZE_FULL);
	SMS_finalizeSprites();
	SMS_copySpritestoSAT();

	SMS_displayOn();	

	generate_map();

	for (;;) {
		joy = SMS_getKeysStatus();

		player_moved = 0;
		if (joy & PORT_A_KEY_UP) {
			walk_dir(&player.x, &player.y, 0, 1, player.dir);
			walked = 1;
			player_moved = 1;
		} else if (joy & PORT_A_KEY_DOWN) {
			walk_dir(&player.x, &player.y, 0, -1, player.dir);
			walked = 1;
			player_moved = 1;
		}
		
		if (joy & PORT_A_KEY_LEFT) {
			player.dir = (player.dir - 1) & 0x03;
			walked = 1;
		} else if (joy & PORT_A_KEY_RIGHT) {
			player.dir = (player.dir + 1) & 0x03;
			walked = 1;
		}
		
		if (player_moved) {
			move_monster();
			if (monster.x == player.x && monster.y == player.y) {
				fade_to_red();
				
				SMS_initSprites();
				SMS_finalizeSprites();				
				SMS_copySpritestoSAT();

				SMS_loadPSGaidencompressedTiles(defeat_tiles_psgcompr, 0);
				SMS_loadTileMap(0, 0, defeat_tilemap_bin, defeat_tilemap_bin_size);
				SMS_loadBGPalette(defeat_palette_bin);
				
				while(1) {
					SMS_waitForVBlank();
					SMS_setBGScrollX(rand() & 0x07);
					SMS_setBGScrollY(rand() & 0x07);
				}
			}
		}

		SMS_initSprites();
		draw_monster();
		SMS_finalizeSprites();
		
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		SMS_loadSpritePalette(monster.palette);

		if (walked) {
			draw_view(player.x, player.y, player.dir, bkg);
			set_bkg_map(bkg, 0, 1, VIEW_WIDTH, VIEW_HEIGHT);			
			
			display_debug_info();
			
			draw_mini_map(player.x, player.y);
			
			walked = 0;
		}

		sprnum = 0;

		tmr++;
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2021,3,07, "Haroldo-OK\\2021", "3D Alien Maze",
  "A first person survival horror.\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
