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

#define BN_DPAD	(BN_RIGHT | BN_LEFT | BN_UP | BN_DOWN)

void select_input(uint16_t bmask);
uint16_t get_input(void);

#ifdef BUILD_GBA
#define read_input()	(~REG_KEYINPUT)
#else
#define read_input()	get_input()
#endif

#endif	/* INPUT_H_ */
