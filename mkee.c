#include <stdio.h>

int main(int argc, char *argv[])
{
	FILE *f = fopen(argv[2], "rt");
	FILE *fo = fopen(argv[1], "wt");
	int i;
	int c;
	for (i = 0; i < 36; i++) {
		fscanf(f, "%i", &c);
		fprintf(fo, "%c", (char)c);
	}
	fclose(f);
	
	f = fopen(argv[3], "rt");
	fscanf(f, "%i", &c);
	fprintf(fo, "%c", (char)c);
	fclose(f);

	f = fopen(argv[4], "rt");
	fscanf(f, "%i", &c);
	fprintf(fo, "%c", (char)c);
	fclose(f);

	for (i = 38; i < 256; i++)
		fprintf(fo, "%c", 255);
	fclose(fo);
	return 0;
}
