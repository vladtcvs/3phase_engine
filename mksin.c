#include <math.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	int max = 100;
	int i, num = 36;
	FILE *f = fopen(argv[1], "wt");
	for (i = 0; i < num; i++) {
		double x = ((double)i)/num;
		int value = (sin(x*2*3.1415926535)+1)*max/2+0.5;
		fprintf(f, "%i\n", value);
	}
	fclose(f);
	return 0;
}
