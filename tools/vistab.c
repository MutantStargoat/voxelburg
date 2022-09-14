#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_DIRS	8
#define MAX_DIST	4
#define FOV			60

struct cell {
	int dx, dy;
};

void calc_vis(int dir);
int is_visible(float x, float y, float dir);

int main(void)
{
	int i;

	printf("struct {int dx, dy;} visoffs[%d][32] = {\n", NUM_DIRS);

	for(i=0; i<NUM_DIRS; i++) {

		printf("\t/* dir %d */\n", i);
		calc_vis(i);
	}

	printf("};\n");
	return 0;
}

int cellcmp(const void *a, const void *b)
{
	const struct cell *ca = a;
	const struct cell *cb = b;
	int za = ca->dx * ca->dx + ca->dy * ca->dy;
	int zb = cb->dx * cb->dx + cb->dy * cb->dy;
	return zb - za;
}

void calc_vis(int dir)
{
	int i, j, count, col;
	struct cell vis[512];
	int x, y;
	float theta = (float)(dir + 2) / NUM_DIRS * M_PI * 2.0f;

	fprintf(stderr, "DIR: %d (angle: %g)\n", dir, theta * 180.0f / M_PI);
	count = 0;
	for(i=0; i<MAX_DIST*2+1; i++) {
		y = i - MAX_DIST;
		for(j=0; j<MAX_DIST*2+1; j++) {
			x = j - MAX_DIST;

			if(is_visible(x, y, theta)) {
				vis[count].dx = x;
				vis[count].dy = y;
				count++;

				fputc('#', stderr);
			} else {
				fputc('.', stderr);
			}
		}
		fputc('\n', stderr);
	}
	fputc('\n', stderr);

	qsort(vis, count, sizeof *vis, cellcmp);

	printf("\t{");
	col = 5;
	for(i=0; i<count; i++) {
		col += printf("{%d,%d}", vis[i].dx, vis[i].dy);
		if(i < count - 1) {
			if(col > 72) {
				fputs(",\n\t ", stdout);
				col = 5;
			} else {
				fputs(", ", stdout);
				col += 2;
			}
		}
	}
	fputs(dir < NUM_DIRS - 1 ? "},\n" : "}\n", stdout);
}

#define RAD 1.42

float ptlinedist(float x, float y, float dir)
{
	return -y * cos(dir) + sin(dir) * x;
}

int is_visible(float x, float y, float dir)
{
	static const char voffs[][2] = {{-0.5, -0.5}, {0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}};
	float d0, d1;
	int i;
	float hfov = 0.5f * FOV * M_PI / 180.0f;

	d0 = ptlinedist(x, y, dir - hfov);
	d1 = ptlinedist(x, y, dir + hfov);
	if(d0 >= 0 && d1 <= 0) {
		return 1;
	}

	for(i=0; i<4; i++) {
		d0 = ptlinedist(x + voffs[i][0], y + voffs[i][1], dir - hfov);
		d1 = ptlinedist(x + voffs[i][0], y + voffs[i][1], dir + hfov);

		if(d0 >= 0 && d1 <= 0) {
			return 1;
		}
	}
	return 0;
}
