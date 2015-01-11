#define __AVR_ATmega48PA__
#define F_CPU 18000000UL
#include <avr/io.h>

typedef unsigned char byte;
typedef unsigned short word;


const byte sinus[36] = {127, 149, 170, 191, 209, 224,
			237, 246, 252, 254, 252, 246,
			237, 224, 209, 191, 170, 149,
			127, 105, 84,  63,  45,  30,
			17,  8,   2,   0,   2,   8,
			17,  30,  45,  63,  84,  105};

void
setup_ports(void)
{
	DDRC = 0x03;
	PORTC = 0x00;
	DDRD = 0xFF;
	PORTD = 0x00;
}

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF ^ (1 << n)))

#define UL 5
#define VL 6
#define WL 7

#define UH 2
#define VH 3
#define WH 4

byte ampl, period, phase;
register byte pwm_u, pwm_v, pwm_w;

int main(void)
{
	register byte i;
	setup_ports();
	phase = 0;
	pwm_u = 10;
	pwm_v = 50;
	pwm_w = 100;
	while (1) {
		byte set = 0;
		CLRBIT(PORTD, UL);
		CLRBIT(PORTD, VL);
		CLRBIT(PORTD, WL);
		SETBIT(PORTD, UH);
		SETBIT(PORTD, VH);
		SETBIT(PORTD, WH);
		for (i = 0; i < 100; i++) {
			if (i == pwm_u) {
				CLRBIT(PORTD, UH);
				SETBIT(set, UL);
			}
			if (i == pwm_v) {
				CLRBIT(PORTD, VH);
				SETBIT(set, VL);
			}
			if (i == pwm_w) {
				CLRBIT(PORTD, WH);
				SETBIT(set, WL);
			}
			PORTD |= set;
		}
		PORTD = 0;
	}
	return 0;
}
