#include "player.h"
#include "gbaregs.h"
#include "xgl.h"

void player_input(struct player *p, uint16_t bnstate)
{
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
		if(p->theta > X_2PI) p->theta -= X_2PI;
	}
	if(bnstate & KEY_RIGHT) {
		p->theta -= 0x800;
		if(p->theta < X_2PI) p->theta += X_2PI;
	}
	if(bnstate & KEY_A) {
		p->y += 0x1000;
	}
	if(bnstate & KEY_B) {
		p->y -= 0x1000;
	}
}
