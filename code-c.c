#define F_CPU 18000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

typedef unsigned char byte;
typedef unsigned short word;

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF ^ (1 << n)))

#define UL 5
#define VL 6
#define WL 7

#define UH 2
#define VH 3
#define WH 4

#define PH_NUM 36
byte sinus[PH_NUM];

byte ampl, period;
byte ph_u, ph_v, ph_w;
register byte pwm_u asm("r10"), pwm_v asm("r11"), pwm_w asm("r12");

void
setup_ports(void)
{
	DDRC  = 0x03;
	PORTC = 0x00;
	DDRD  = 0xFF;
	PORTD = 0x00;
}

byte
eeread(word addr)
{
	EEARL = addr & 0xFF;
	EECR |= EERE;
	return EEDR;
}

void
setup_timer(void)
{
	TCCR0A = 0x00;
	TCCR0B = 0x05; //prescale /1024
	TIMSK0 = 0x02; // OCR0A
	OCR0A = period;
}

ISR(TIMER0_COMPA_vect)
{
	TCNT0 = 0;
	OCR0A = period;
	ph_u++;
	if (ph_u == PH_NUM)
		ph_u = 0;
	ph_v++;
	if (ph_v == PH_NUM)
		ph_v = 0;
	ph_w++;
	if (ph_w == PH_NUM)
		ph_w = 0;
	pwm_u = (sinus[ph_u]*((word)ampl))>>8;
	pwm_v = (sinus[ph_v]*((word)ampl))>>8;
	pwm_w = (sinus[ph_w]*((word)ampl))>>8;
}

void
read_config(void)
{
	byte i;
	for (i = 0; i < PH_NUM; i++)
		sinus[i] = eeread(i);
	ampl = eeread(PH_NUM);
	period = eeread(PH_NUM + 1);
}

int main(void)
{
	setup_ports();
	read_config();

	ph_u = 0;
	ph_v = 12;
	ph_w = 24;
	pwm_u = 0;
	pwm_v = 0;
	pwm_w = 0;

	sei();
	while (1) {
		byte set = 0;
		register byte i;
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
