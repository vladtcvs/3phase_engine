#include <stdio.h>

int main(int argc, char *argv[])
{
	FILE *f;
	FILE *fo; 
	int i;
	int c;
	if (argc != 5) {
		printf("Usage: mkee eesin.bin sin.txt ampl.txt freq.txt");
		return 1;
	}
	f = fopen(argv[2], "rt");
	if (f == NULL) {
		printf("File %s not found\n", argv[2]);
		return 1;
	}
	fo = fopen(argv[1], "wt");
	if (fo == NULL) {
		printf("Can not open file %s for write\n", argv[1]);
		return 1;
	}
	for (i = 0; i < 36; i++) {
		fscanf(f, "%i", &c);
		fprintf(fo, "%c", (char)c);
	}
	fclose(f);
	
	f = fopen(argv[3], "rt");
	if (f == NULL) {
		printf("File %s not found\n", argv[3]);
		return 1;
	}
	fscanf(f, "%i", &c);
	fprintf(fo, "%c", (char)c);
	fclose(f);

	f = fopen(argv[4], "rt");
	if (f == NULL) {
		printf("File %s not found\n", argv[4]);
		return 1;
	}
	fscanf(f, "%i", &c);
	fprintf(fo, "%c", (char)c);
	fclose(f);

	for (i = 38; i < 256; i++)
		fprintf(fo, "%c", 255);
	fclose(fo);
	return 0;
}
