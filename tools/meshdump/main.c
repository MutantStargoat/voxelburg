#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmesh.h"

int dump(struct cmesh *cm);

int main(int argc, char **argv)
{
	int i;
	struct cmesh *cm;

	for(i=1; i<argc; i++) {
		if(!(cm = cmesh_alloc())) {
			perror("failed to allocate mesh");
			return 1;
		}
		if(cmesh_load(cm, argv[i]) == -1) {
			fprintf(stderr, "failed to load: %s\n", argv[i]);
			return 1;
		}

		dump(cm);

		cmesh_free(cm);
	}
	return 0;
}

static int nverts, nidx, voffs;
static const float *varr, *narr, *tarr;
static unsigned int *iarr;

static int zcmp(const void *a, const void *b)
{
	unsigned int *aidx = (unsigned int*)a;
	unsigned int *bidx = (unsigned int*)b;

	float az = varr[aidx[0] * 3 + 2] + varr[aidx[1] * 3 + 2] + varr[aidx[2] * 3 + 2];
	float bz = varr[bidx[0] * 3 + 2] + varr[bidx[1] * 3 + 2] + varr[bidx[2] * 3 + 2];

	return az - bz;
}

int dump(struct cmesh *cm)
{
	int i;

	varr = cmesh_attrib_ro(cm, CMESH_ATTR_VERTEX);
	narr = cmesh_attrib_ro(cm, CMESH_ATTR_NORMAL);
	tarr = cmesh_attrib_ro(cm, CMESH_ATTR_TEXCOORD);
	iarr = cmesh_index(cm);
	nverts = cmesh_attrib_count(cm, CMESH_ATTR_VERTEX);
	nidx = cmesh_index_count(cm);

	qsort(iarr, nidx / 3, sizeof *iarr * 3, zcmp);

	printf("static struct xvertex mesh[] = {\n");
	for(i=0; i<nidx; i++) {
		voffs = iarr[i] * 3;
		printf("\t{%7d, %7d, %7d,", (int)(varr[voffs] * 65536.0f),
				(int)(varr[voffs + 1] * 65536.0f), -(int)(varr[voffs + 2] * 65536.0f));
		printf("  %7d, %7d, %7d,", (int)(narr[voffs] * 65536.0f),
				(int)(narr[voffs + 1] * 65536.0f), -(int)(narr[voffs + 2] * 65536.0f));
		printf("  %7d, %7d,", (int)(tarr[voffs] * 65536.0f), (int)(tarr[voffs] * 65536.0f));
		printf("  0xff}%c\n", i < nidx - 1 ? ',' : '\n');
	}
	printf("};\n");

	return 0;
}
