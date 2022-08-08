/*
gbajam22 entry for the Gameboy Advance
Copyright (C) 2022  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef POLYFILL_H_
#define POLYFILL_H_

#include <stdint.h>

struct pvertex {
	int32_t x, y;
};

void polyfill_framebuffer(unsigned char *fb, int width, int height);
void polyfill_flat(struct pvertex *v, int vnum, unsigned char col);

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax);
void draw_line(int x0, int y0, int x1, int y1, unsigned short color);

#endif	/* POLYFILL_H_ */
