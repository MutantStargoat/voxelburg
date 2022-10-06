#include "game.h"

static int menuscr_start(void);
static void menuscr_stop(void);
static void menuscr_frame(void);

static struct screen menuscr = {
	"menu",
	menuscr_start,
	menuscr_stop,
	menuscr_frame,
	0
};

struct screen *init_menu_screen(void)
{
	return &menuscr;
}

static int menuscr_start(void)
{
	return 0;
}

static void menuscr_stop(void)
{
}

static void menuscr_frame(void)
{
}
