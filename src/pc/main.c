#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "miniglut.h"
#include "game.h"
#include "gba.h"
#include "input.h"
#include "debug.h"

static void display(void);
static void vblank(void);
static void idle(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);
static unsigned int next_pow2(unsigned int x);
static void set_fullscreen(int fs);
static void set_vsync(int vsync);

static unsigned int num_pressed;
static unsigned char keystate[256];

static unsigned long start_time;
static unsigned int modkeys;

static uint16_t bnstate, bnmask;

static float win_aspect;
static unsigned int tex;

static int tex_xsz, tex_ysz;
static uint32_t convbuf[240 * 160];

#ifdef __unix__
#include <GL/glx.h>
static Display *xdpy;
static Window xwin;

static void (*glx_swap_interval_ext)();
static void (*glx_swap_interval_sgi)();
#endif
#ifdef _WIN32
#include <windows.h>
static PROC wgl_swap_interval_ext;
#endif

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(960, 640);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("GBAjam22 PC build");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);

#ifdef __unix__
	xdpy = glXGetCurrentDisplay();
	xwin = glXGetCurrentDrawable();

	if(!(glx_swap_interval_ext = glXGetProcAddress((unsigned char*)"glXSwapIntervalEXT"))) {
		glx_swap_interval_sgi = glXGetProcAddress((unsigned char*)"glXSwapIntervalSGI");
	}
#endif
#ifdef _WIN32
	wgl_swap_interval_ext = wglGetProcAddress("wglSwapIntervalEXT");
#endif

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	tex_xsz = next_pow2(240);
	tex_ysz = next_pow2(160);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_xsz, tex_ysz, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(240.0f / tex_xsz, 160.0f / tex_ysz, 1);

	intr_disable();
	interrupt(INTR_VBLANK, vblank);
	unmask(INTR_VBLANK);
	intr_enable();

	if(init_screens() == -1) {
		fprintf(stderr, "failed to initialize screens");
		return 1;
	}

	if(change_screen(find_screen("game")) == -1) {
		fprintf(stderr, "failed to find game screen");
		return 1;
	}

	glutMainLoop();
	return 0;
}

void select_input(uint16_t bmask)
{
	bnstate = 0;
	bnmask = bmask;
}

uint16_t get_input(void)
{
	uint16_t s = bnstate;
	bnstate = 0;
	return s;
}

#define PACK_RGB32(r, g, b) \
	((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff) | 0xff000000)

#define UNPACK_R16(c)	(((c) << 3) & 0xf8)
#define UNPACK_G16(c)	(((c) >> 2) & 0xf8)
#define UNPACK_B16(c)	(((c) >> 7) & 0xf8)

void present(int buf)
{
	int i, npix = 240 * 160;
	uint32_t *dptr = convbuf;
	uint8_t *sptr = (uint8_t*)(buf ? gba_vram_lfb1 : gba_vram_lfb0);

	for(i=0; i<npix; i++) {
		int idx = *sptr++;
		int r = UNPACK_R16(gba_bgpal[idx]);
		int g = UNPACK_G16(gba_bgpal[idx]);
		int b = UNPACK_B16(gba_bgpal[idx]);
		*dptr++ = PACK_RGB32(b, g, r);
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 240, 160, GL_RGBA,
			GL_UNSIGNED_BYTE, convbuf);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(win_aspect >= 1.5f) {
		glScalef(1.5f / win_aspect, 1, 1);
	} else {
		glScalef(1, win_aspect / 1.5f, 1);
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1);
	glEnd();

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

static void display(void)
{
	if(curscr) {
		curscr->frame();
	}
}

static void vblank(void)
{
	vblperf_count++;

	if(curscr && curscr->vblank) {
		curscr->vblank();
	}
}

static void idle(void)
{
	glutPostRedisplay();
}

static void reshape(int x, int y)
{
	win_aspect = (float)x / (float)y;
	glViewport(0, 0, x, y);
}

static int bnmap(int key)
{
	switch(key) {
	case GLUT_KEY_LEFT:
		return BN_LEFT;
	case GLUT_KEY_RIGHT:
		return BN_RIGHT;
	case GLUT_KEY_UP:
		return BN_UP;
	case GLUT_KEY_DOWN:
		return BN_DOWN;
	case 'x':
		return BN_A;
	case 'z':
		return BN_B;
	case 'a':
		return BN_LT;
		break;
	case 's':
		return BN_RT;
	case ' ':
		return BN_SELECT;
	case '\n':
		return BN_START;
	default:
		break;
	}
	return -1;
}

static void keydown(unsigned char key, int x, int y)
{
	int bn = bnmap(key);
	if(bn != -1) {
		bnstate |= bn;
	}

	if(key == 27) {
		exit(0);
	}
}

static void keyup(unsigned char key, int x, int y)
{
	int bn = bnmap(key);
	if(bn != -1) {
		bnstate &= ~bn;
	}
}

static void skeydown(int key, int x, int y)
{
	int bn = bnmap(key);
	if(bn != -1) {
		bnstate |= bn;
	}
}

static void skeyup(int key, int x, int y)
{
	int bn = bnmap(key);
	if(bn != -1) {
		bnstate &= ~bn;
	}
}

static int mbstate[3];
static int prev_x, prev_y;

static void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	int press = st == GLUT_DOWN ? 1 : 0;

	if(bidx < 3) {
		mbstate[bidx] = press;
	}
	prev_x = x;
	prev_y = y;
}

static void motion(int x, int y)
{
	int dx, dy;

	dx = x - prev_x;
	dy = y - prev_y;
	prev_x = x;
	prev_y = y;

	if(!(dx | dy)) return;

	if(mbstate[0]) {
		view_dtheta -= dx * 0x100;
		view_dphi -= dy * 0x100;
	}
	if(mbstate[2]) {
		view_zoom += dy * 0x100;
		if(view_zoom < 0) view_zoom = 0;
	}
}

static unsigned int next_pow2(unsigned int x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

static void set_fullscreen(int fs)
{
	static int win_x, win_y;

	if(fs) {
		if(win_x == 0) {
			win_x = glutGet(GLUT_WINDOW_WIDTH);
			win_y = glutGet(GLUT_WINDOW_HEIGHT);
			glutFullScreen();
		}
	} else {
		if(win_x) {
			glutReshapeWindow(win_x, win_y);
			win_x = win_y = 0;
		}
	}
}

#ifdef __unix__
static void set_vsync(int vsync)
{
	vsync = vsync ? 1 : 0;
	if(glx_swap_interval_ext) {
		glx_swap_interval_ext(xdpy, xwin, vsync);
	} else if(glx_swap_interval_sgi) {
		glx_swap_interval_sgi(vsync);
	}
}
#endif
#ifdef WIN32
static void set_vsync(int vsync)
{
	if(wgl_swap_interval_ext) {
		wgl_swap_interval_ext(vsync ? 1 : 0);
	}
}
#endif
