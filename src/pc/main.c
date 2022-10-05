#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "miniglut.h"
#include "game.h"
#include "gba.h"

static void display(void);
static void idle(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static int translate_special(int skey);
static unsigned int next_pow2(unsigned int x);
static void set_fullscreen(int fs);
static void set_vsync(int vsync);

static unsigned int num_pressed;
static unsigned char keystate[256];

static unsigned long start_time;
static unsigned int modkeys;

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
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("GBAjam22 PC build");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);

	glutSetCursor(GLUT_CURSOR_NONE);

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

	gamescr();
	return 0;
}

void blit_frame(void *pixels, int vsync)
{
	int i, npix = fb_width * fb_height;
	uint32_t *dptr = convbuf;
	uint16_t *sptr = pixels;

	for(i=0; i<npix; i++) {
		int r = UNPACK_R16(*sptr);
		int g = UNPACK_G16(*sptr);
		int b = UNPACK_B16(*sptr);
		*dptr++ = PACK_RGB32(b, g, r);
		sptr++;
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb_width, fb_height, GL_RGBA,
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

int kb_isdown(int key)
{
	switch(key) {
	case KB_ANY:
		return num_pressed;

	case KB_ALT:
		return keystate[KB_LALT] + keystate[KB_RALT];

	case KB_CTRL:
		return keystate[KB_LCTRL] + keystate[KB_RCTRL];
	}

	if(isalpha(key)) {
		key = tolower(key);
	}
	return keystate[key];
}

static void display(void)
{
	inp_update();

	time_msec = get_msec();
	draw();
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

static void keydown(unsigned char key, int x, int y)
{
	modkeys = glutGetModifiers();

	keystate[key] = 1;
	//game_key(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	keystate[key] = 0;
	//game_key(key, 0);
}

static void skeydown(int key, int x, int y)
{
	key = translate_special(key);
	keystate[key] = 1;
	//game_key(key, 1);
}

static void skeyup(int key, int x, int y)
{
	key = translate_special(key);
	keystate[key] = 0;
	//game_key(key, 0);
}

static int translate_special(int skey)
{
	switch(skey) {
	case 127:
		return 127;
/*	case GLUT_KEY_LEFT:
		return KB_LEFT;
	case GLUT_KEY_RIGHT:
		return KB_RIGHT;
	case GLUT_KEY_UP:
		return KB_UP;
	case GLUT_KEY_DOWN:
		return KB_DOWN;
	case GLUT_KEY_PAGE_UP:
		return KB_PGUP;
	case GLUT_KEY_PAGE_DOWN:
		return KB_PGDN;
	case GLUT_KEY_HOME:
		return KB_HOME;
	case GLUT_KEY_END:
		return KB_END;
	default:
		if(skey >= GLUT_KEY_F1 && skey <= GLUT_KEY_F12) {
			return KB_F1 + skey - GLUT_KEY_F1;
		}
		*/
	}
	return 0;
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
