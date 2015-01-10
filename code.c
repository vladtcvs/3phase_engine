#define __AVR_ATmega48PA__
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
	PORTD = 0xFF;
}

void
delay(short time)
{
	short i;
	for (i = 0; i < time; i++)
		;
}

void
flt_clr(void)
{
	PORTD |= 0xFC;
	delay(0x300);
	PORTD &= 0x1F;
	delay(0x300);
	PORTD |= 0xE0;
	delay(0x300);
}

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF ^ (1 << n)))

void
set_bridge_u(byte U)
{
	byte port = PORTD;
	if (U == 0) {
		SETBIT(port, 2);
		CLRBIT(port, 5);
	} else {
		SETBIT(port, 5);
		CLRBIT(port, 2);
	}
	PORTD = port;
}

void
set_bridge_v(byte V)
{
	byte port = PORTD;

	if (V == 0) {
		SETBIT(port, 3);
		CLRBIT(port, 6);
	} else {
		SETBIT(port, 6);
		CLRBIT(port, 3);
	}
	PORTD = port;
}

void
set_bridge_w(byte W)
{
	byte port = PORTD;

	if (W == 0) {
		SETBIT(port, 4);
		CLRBIT(port, 7);
	} else {
		SETBIT(port, 7);
		CLRBIT(port, 4);
	}
	PORTD = port;
}

void
block_bridge(void)
{
	PORTD |= 0xFC;
}

byte ampl, period, phase, pwm_u, pwm_v, pwm_w;

int main(void)
{
	int i;
	setup_ports();
	flt_clr();
	phase = 0;
	pwm_u = 10;
	pwm_v = 100;
	pwm_w = 255;
	while (1) {
		set_bridge_u(0);
		set_bridge_v(0);
		set_bridge_w(0);
		for (i = 0; i < 256; i++) {
			if (i == pwm_u)
				set_bridge_u(1);
			if (i == pwm_v)
				set_bridge_v(1);
			if (i == pwm_w)
				set_bridge_w(1);
		}
		block_bridge();
	}
	return 0;
}
