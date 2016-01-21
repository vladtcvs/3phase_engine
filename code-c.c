#include "defines.h"

#include "uart.h"
#include "pwm.h"

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF - (1 << n)))

#define EU 1
#define EV 3
#define EW 5

#define UU 0
#define UV 2
#define UW 4

#define DELAY_STEP 100
#define DELAY_NUM 5

static byte sinus[PH_NUM];

byte ampl = 0, period;
byte ph_u, ph_v, ph_w;
byte go;

register byte pwm_u asm("r10"), pwm_v asm("r11"), pwm_w asm("r12");

#define DISABLED 0x00

void
setup_ports(void)
{
	DDRC  = 0x3F;
	PORTC = 0x00;
	DDRD  = 0xF0;
	PORTD = 0x00;
}

/* freq rotation = 488 / period */
void
setup_timer(void)
{
	TCCR0A = 0x00;
	TCCR0B = 0x05; //prescale /1024
	TIMSK0 = 0x02; // OCR0A
	OCR0A = period;
}

ISR(__vector_default, ISR_NAKED)
{
	reti();
}

#define min(a, b) (((a) < (b)) ? (a) : (b))

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
	pwm_u = min((sinus[ph_u]*((word)ampl))>>8, 90);
	pwm_v = min((sinus[ph_v]*((word)ampl))>>8, 90);
	pwm_w = min((sinus[ph_w]*((word)ampl))>>8, 90);
}


void
read_config(void)
{
	byte i;
	for (i = 0; i < PH_NUM; i++) {
		sinus[i] = eeread(i);
	}
	ampl = eeread(AMPL_START);
	period = FREQ_MAX/eeread(FREQ_START);
}

int main(void)
{
	setup_ports();
	read_config();
	setup_timer();
	PORTC = DISABLED;
	setup_uart(9600UL);
	go = 0;
	ph_u = 0;
	ph_v = 24;
	ph_w = 12;
	pwm_u = 0;
	pwm_v = 0;
	pwm_w = 0;

	sei();
	while (1) {
		register byte i;
		if (go) {
			int8_t cnt_u, cnt_v, cnt_w;
			int u_u, u_v, u_w;
			/* Disable all phases */
			PORTC &= ~((1 << EU) | (1 << EV) | (1 << EW));

			/* Wait to turn mosfets off */
			_delay_loop_2(DELAY_NUM * DELAY_STEP);

			/* Switch all bridges to HIGH */
			PORTC |= ((1<<UU) | (1<<UV) | (1<<UW));

			/* Enable all phases */
			PORTC |= ((1<<EU) | (1<<EV) | (1<<EW));
			//_delay_loop_2(50);

			cnt_u = cnt_v = cnt_w = -1;
			u_u = u_v = u_w = 1;
			for (i = 0; i < 100; i++) {
				if (i == pwm_u || (i > pwm_u && u_u)) {
					CLRBIT(PORTC, EU); // DISABLE U
					u_u = 0;
					cnt_u = DELAY_NUM;
				}
				if (i == pwm_v || (i > pwm_v && u_v)) {
					CLRBIT(PORTC, EV); // DISABLE V
					u_v = 0;
					cnt_v = DELAY_NUM;
				}
				if (i == pwm_w  || (i > pwm_w && u_w)) {
					CLRBIT(PORTC, EW); // DISABLE W
					u_w = 0;
					cnt_w = DELAY_NUM;
				}
				_delay_loop_2(DELAY_STEP);
				if (cnt_u == 0) {
					CLRBIT(PORTC, UU);  // Set U to LOW
					PORTC |= (1<<EU);
					cnt_u = -1;
				}
				if (cnt_v == 0) {
					CLRBIT(PORTC, UV);  // Set V to LOW
					PORTC |= (1<<EV);
					cnt_v = -1;
				}
				if (cnt_w == 0) {
					CLRBIT(PORTC, UW);  // Set W to LOW
					PORTC |= (1<<EW);
					cnt_w = -1;
				}
				if (cnt_u > 0)
					cnt_u--;
				if (cnt_v > 0)
					cnt_v--;
				if (cnt_w > 0)
					cnt_w--;
			}
		} else {
			PORTC = DISABLED;
			_delay_loop_1(250);
		}
	}
	return 0;
}
