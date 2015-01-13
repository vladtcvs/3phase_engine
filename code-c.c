#define F_CPU 18000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay_basic.h>

typedef unsigned char byte;
typedef unsigned short word;

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF - (1 << n)))

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
byte rs232_len;
byte rs232_buf[4];

void
setup_ports(void)
{
	DDRC  = 0x03;
	PORTC = 0x00;
	DDRD  = 0xFF;
	PORTD = 0x00;
}

#define EEARH   _SFR_IO8(0x22)

#define eeread(addr)  eeprom_read_byte((uint8_t*)((int)(addr)))

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

ISR(USART_RX_vect)
{
	byte rcv = UDR0;
	UDR0 = rcv;
}

void
setup_uart(unsigned long bitrate)
{
	word ubrr = F_CPU / (16 * bitrate) - 1;
	UBRR0H = (byte)(ubrr >> 8);
	UBRR0L = (byte)(ubrr & 0xFF);
	UCSR0A = 0;
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	UCSR0C = 3<<UCSZ00;
}

void
read_config(void)
{
	byte i;
	for (i = 0; i < PH_NUM; i++) {
		sinus[i] = eeread(i);
		//sinus[i] = i*100/PH_NUM;
	}
	ampl = eeread(PH_NUM);
	period = eeread(PH_NUM + 1);
}

int main(void)
{
	setup_ports();
	read_config();
	setup_timer();
	setup_uart(115200UL);

	ph_u = 0;
	ph_v = 24;
	ph_w = 12;
	pwm_u = 10;
	pwm_v = 50;
	pwm_w = 50;

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
			_delay_loop_1(50);
			PORTD |= set;
		}
		PORTD = 0;
		_delay_loop_1(50);
	}
	return 0;
}
