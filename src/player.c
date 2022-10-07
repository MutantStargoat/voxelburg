#include <string.h>
#include "game.h"
#include "player.h"
#include "level.h"
#include "gbaregs.h"
#include "xgl.h"

void init_player(struct player *p, struct level *lvl)
{
	memset(p, 0, sizeof *p);
	p->cx = lvl->orgx;
	p->cy = lvl->orgy;
	cell_to_pos(lvl, lvl->orgx, lvl->orgy, &p->x, &p->y);
	p->cell = level_cell(lvl, lvl->orgx, lvl->orgy);
}

void player_input(struct player *p, uint16_t bnstate)
{
#ifndef BUILD_GBA
	p->theta = (p->theta + view_dtheta) % X_2PI;
	if(p->theta < 0) p->theta += X_2PI;
	p->phi += view_dphi;
	if(p->phi > X_HPI) p->phi = X_HPI;
	if(p->phi < -X_HPI) p->phi = -X_HPI;

	view_dtheta = 0;
	view_dphi = 0;
#endif

	if(bnstate & KEY_UP) {
		p->phi += 0x800;
		if(p->phi > X_HPI) p->phi = X_HPI;
	}
	if(bnstate & KEY_DOWN) {
		p->phi -= 0x800;
		if(p->phi < -X_HPI) p->phi = -X_HPI;
	}
	if(bnstate & KEY_LEFT) {
		p->theta += 0x800;
		if(p->theta >= X_2PI) p->theta -= X_2PI;
	}
	if(bnstate & KEY_RIGHT) {
		p->theta -= 0x800;
		if(p->theta < 0) p->theta += X_2PI;
	}
	if(bnstate & KEY_A) {
		p->y += 0x2000;
	}
	if(bnstate & KEY_B) {
		p->y -= 0x2000;
	}
}
