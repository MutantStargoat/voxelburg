#include <stdio.h>
#include <math.h>

#define SINLUT_SIZE		256
#define SINLUT_SCALE	32767.0

int main(void)
{
	int i;

	puts("\t.data");
	puts("\t.globl sinlut");
	puts("sinlut:");
	for(i=0; i<SINLUT_SIZE; i++) {
		float t = (float)i / SINLUT_SIZE;
		float theta = t * (M_PI * 2);
		printf("\t.short %d\n", (int)(sin(theta) * SINLUT_SCALE));
	}
	return 0;
}
