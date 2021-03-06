#include <stdlib.h>
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

#define WALL_OFFS_1 (16 * 12)
#define WALL_OFFS_2 (WALL_OFFS_1 + 8 * 8)
#define WALL_OFFS_3 (WALL_OFFS_2 + 4 * 4)

#define SIDE_OFFS_0 (0)
#define SIDE_OFFS_1 (8 * 12)
#define SIDE_OFFS_2 (SIDE_OFFS_1 + 4 * 12)

#define WALL_TOP_1 (2 << 5)
#define WALL_TOP_2 (4 << 5)
#define WALL_TOP_3 (5 << 5)

#define MAP_SHIFT 3

#define BLOCK_SHIFT 5

#define DIR_NORTH 0
#define DIR_EAST 1
#define DIR_SOUTH 2
#define DIR_WEST 3

#define set_bkg_map(src, x, y, width, height) SMS_loadTileMapArea(x, y, src, width, height);

unsigned char get_map(int x, int y);

//#define HIDE_SIDE_WALLS

/*
unsigned char map[] = {
	0, 0, 1, 1, 1, 1, 1, 1,
	0, 0, 1, 0, 0, 0, 0, 1,
	0, 0, 1, 0, 1, 1, 1, 1,
	0, 0, 1, 0, 1, 0, 1, 1,
	0, 0, 1, 0, 0, 0, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
};
*/

/*
unsigned char map[] = {
	0, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 0, 0, 1, 0, 1,
	0, 1, 0, 0, 0, 1, 1, 1,
	0, 1, 0, 0, 0, 1, 1, 1,
	0, 1, 0, 0, 0, 1, 1, 1,
	0, 0, 1, 1, 1, 0, 0, 0,
};
*/

const unsigned char map[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 0, 1, 0, 1, 1,
	1, 0, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 0, 1,
	1, 1, 0, 1, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 1,
	1, 1, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
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
	return map[(y << MAP_SHIFT) + x];
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

//	memset(bkg, 0, VIEW_WIDTH*VIEW_HEIGHT*2);
	top = test_flr;
	p = bkg;
	for (i = 0; i != VIEW_HEIGHT; i++) {
		for (j = 0, p2 = p + (VIEW_WIDTH-1); j != (VIEW_WIDTH >> 1); j++, p2--) {
			*p = *top ^ TILE_USE_SPRITE_PALETTE;
			*p2 = *top ^ (TILE_USE_SPRITE_PALETTE | TILE_FLIPPED_X);

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
				mask = TILE_USE_SPRITE_PALETTE;
				if (i > 16) {
					tx = 0x0F - tx;
					mask |= TILE_FLIPPED_X;
				}
				ofs = sidewall_offs1[tx];
				h = VIEW_HEIGHT - (ofs << 1);
				tx = (tx << 3) + (tx << 2); // Same as tx *= 12;

				for (j = 0, p = top + (ofs << 5), p2 = test_bkg + SIDE_OFFS_0 + tx + ofs; j != h; j++, p += VIEW_WIDTH, p2++) {
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
				mask = TILE_USE_SPRITE_PALETTE;
				if (tx & 0x08) {
					tx = 0x0F - tx;
					mask |= TILE_FLIPPED_X;
				}
				tx <<= 3;

				for (j = 0, p = top + WALL_TOP_1, p2 = test_bkg + WALL_OFFS_1 + tx; j != 8; j++, p += VIEW_WIDTH, p2++) {
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
				mask = TILE_USE_SPRITE_PALETTE;
				if (i > 16) {
					tx = 0x07 - tx;
					mask |= TILE_FLIPPED_X;
				}
				ofs = sidewall_offs2[tx];
				h = VIEW_HEIGHT - (ofs << 1);
				tx = (tx << 3) + (tx << 2); // Same as tx *= 12;

				for (j = 0, p = top + (ofs << 5), p2 = test_bkg + SIDE_OFFS_1 + tx + ofs; j != h; j++, p += VIEW_WIDTH, p2++) {
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
				mask = TILE_USE_SPRITE_PALETTE;
				if (tx & 0x04) {
					tx = 0x07 - tx;
					mask |= TILE_FLIPPED_X;
				}
				tx <<= 2;

				for (j = 0, p = top + WALL_TOP_2, p2 = test_bkg + WALL_OFFS_2 + tx; j != 4; j++, p += VIEW_WIDTH, p2++) {
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
				mask = TILE_USE_SPRITE_PALETTE;
				if (i > 16) {
					tx = 0x03 - tx;
					mask |= TILE_FLIPPED_X;
				}
				ofs = 4;
				h = VIEW_HEIGHT - (ofs << 1);
				tx = (tx << 3) + (tx << 2); // Same as tx *= 12;

				for (j = 0, p = top + (ofs << 5), p2 = test_bkg + SIDE_OFFS_2 + tx + ofs; j != h; j++, p += VIEW_WIDTH, p2++) {
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
				mask = TILE_USE_SPRITE_PALETTE;
				if (tx & 0x02) {
					tx = 0x03 - tx;
					mask |= TILE_FLIPPED_X;
				}
				tx <<= 1;

				for (j = 0, p = top + WALL_TOP_3, p2 = test_bkg + WALL_OFFS_3 + tx; j != 2; j++, p += VIEW_WIDTH, p2++) {
					*p = *p2 ^ mask;
				}
				found = 1;
			}
		}
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
//	set_bkg_map(test_flr, 0, 16, VIEW_WIDTH >> 1, VIEW_HEIGHT); // *** DEBUG ***
}

void test_spr(int x, int y, int *sprnum, int tile) {
	int i, j;
	int til = tile;
	int spr = *sprnum;
	int sx, sy;

	for (i = 0; i != 4; i++) {
		sx = x + (i << 3);
		if ((sx < 0) || (sx > (256 - 8))) {  // TODO: Improve this. Rewrite the loop, instead.
			til += 6;
		} else {
			for (j = 0; j != 3; j++) {
				sy = y + (j << 4);
				if ((sy < 0) || (sy > 104)) { // TODO: Improve this. Rewrite the loop, instead.
				} else {
					set_sprite(spr, sx, sy, til);
					spr++;
				}
				til += 2;
			}
		}
	}

	*sprnum = spr;
}

void test_spr_persp(int x, int y, int dir, int *sprnum, int tile) {
	rotate_dir(&x, &y, dir);

	y += 16;

	if (y < 0) {
		return;
	} else if (y > 63) {
		return;
	}

	/*
	x = (x * 192) / (y + 32);
	y = (48 * 32) / (y + 32);
	*/
	x = (x * persptab_dat[y]) >> 5;
	y = ytab_dat[y]; // Supposes that the z coordinate is fixed.

	test_spr(x, y, sprnum, tile);
}

void main() {
	int x = 3;
	int y = 1;
	int tx, ty;

	int i, j;

	int dir = DIR_SOUTH;
	int walked = -1;
	int tmr = 0;
	int sprnum;
	int joy;
	unsigned int bkg[VIEW_WIDTH*VIEW_HEIGHT];

	set_vdp_reg(VDP_REG_FLAGS0, VDP_REG_FLAGS0_CHANGE/* | VDP_REG_FLAGS0_LHS*/);
	set_vdp_reg(VDP_REG_FLAGS1, VDP_REG_FLAGS1_SCREEN | VDP_REG_FLAGS1_8x16);
	load_palette(test_pal, 0, 16);
	load_palette(ega_pal, 16, 16);

	load_tiles(player_til, 16, 32, 4);
	load_tiles(monster_til, 48, 32, 4);
	load_tiles(test_til, 256, 192, 4);

	for (;;) {
		joy = read_joypad1();

		if (joy & JOY_UP) {
			walk_dir(&x, &y, 0, 1, dir);
			walked = 1;
		} else if (joy & JOY_DOWN) {
			walk_dir(&x, &y, 0, -1, dir);
			walked = 1;
		}
		if (joy & JOY_LEFT) {
			dir = (dir - 1) & 0x03;
			walked = 1;
		} else if (joy & JOY_RIGHT) {
			dir = (dir + 1) & 0x03;
			walked = 1;
		}

		SMS_waitForVBlank();

		if (walked) {
			draw_view(x, y, dir, bkg);
			set_bkg_map(bkg, 0, 1, VIEW_WIDTH, VIEW_HEIGHT);

			walked = 0;
		}

		sprnum = 0;
		/*
		test_spr_persp(px-mx-16, py-my-16, dir, &sprnum, 16);
		test_spr_persp(ex-mx-16, ey-my-16, dir, &sprnum, 48);
		*/
		for (i = 0, j = (128 - 40); i != 2; i++, j += 48) {
			test_spr(j, 48, &sprnum, 48);
		}
		set_sprite(sprnum, 208, 208, 0);

		tmr++;
	}
}
