#include <stdio.h>

int main(void)
{
	FILE *f = fopen("/dev/ttyS0", "rw");
	while (1) {
		unsigned int x;
		unsigned char c;
		scanf("%x", &x);
		c = x & 0xFF;
		printf("send: %x\n", (int)c);
		fprintf(f, "%c", c);
		fscanf(f, "%c", &c);
		printf("rcv: %x\n", (int)c);
	}
	return 0;
}