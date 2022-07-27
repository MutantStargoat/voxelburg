#include <stdio.h>
#include <math.h>

int main(void)
{
	int i;

	puts("\t.data");
	puts("\t.globl sinlut");
	puts("sinlut:");
	for(i=0; i<256; i++) {
		float x = sin((float)i / 128.0f * M_PI);
		printf("\t.short %d\n", (int)(x * 256.0f));
	}
	return 0;
}
