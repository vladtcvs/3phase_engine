#include <math.h>
#include <stdio.h>

int
main(void)
{
	int max = 50;
	int i, num = 36;
	FILE *f = fopen("sin.txt", "wt");
	for (i = 0; i < num; i++) {
		double x = ((double)i)/num;
		int value = (sin(x*2*3.1415926535)+1)*max/2+0.5;
		fprintf(f, "%i\n", value);
	}
	fclose(f);
	return 0;
}
