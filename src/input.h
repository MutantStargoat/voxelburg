#ifndef INPUT_H_
#define INPUT_H_

#include <stdint.h>

enum {
	BN_A		= 0x0001,
	BN_B		= 0x0002,
	BN_SELECT	= 0x0004,
	BN_START	= 0x0008,
	BN_RIGHT	= 0x0010,
	BN_LEFT		= 0x0020,
	BN_UP		= 0x0040,
	BN_DOWN		= 0x0080,
	BN_RT		= 0x0100,
	BN_LT		= 0x0200
};

#ifdef BUILD_GBA
#define keyb_vblank()	(keystate = ~REG_KEYINPUT)
#endif

#define KEYPRESS(key)	((keystate & (key)) && (keydelta & (key)))
#define KEYRELEASE(key)	((keystate & (key)) == 0 && (keydelta & (key)))

volatile uint16_t keystate, keydelta;

/*void key_repeat(int start, int rep, uint16_t mask);*/

void update_keyb(void);


#endif	/* INPUT_H_ */
