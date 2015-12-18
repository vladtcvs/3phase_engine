#define F_CPU 18000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay_basic.h>

typedef unsigned char byte;
typedef unsigned short word;

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF - (1 << n)))

#define VU 5
#define VV 6
#define VW 7

#define XU 2
#define XV 3
#define XW 4

#define PH_NUM 36
byte sinus[PH_NUM];

byte ampl, period;
byte ph_u, ph_v, ph_w;
register byte pwm_u asm("r10"), pwm_v asm("r11"), pwm_w asm("r12");
byte rs232_len;
byte rs232_buf[4];
byte go;

#define STOP 'B'
#define START 'G'
#define CW 'R'
#define CCW 'L'
#define REV 'I'
#define PER 'T'
#define AMPL 'A'

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
	byte t;
	byte rcv = UDR0;
	UDR0 = rcv;
	rs232_buf[rs232_len++] = rcv;
	switch(rs232_buf[0]) {
	case 'B':
		go = 0;
		rs232_len = 0;
		break;
	case 'G':
		go = 1;
		rs232_len = 0;
		break;
	case 'C':
		ph_v = (ph_u + 12)%PH_NUM;
		ph_w = (ph_u + 24)%PH_NUM;
		rs232_len = 0;
		break;
	case 'W':
		ph_v = (ph_u + 24)%PH_NUM;
		ph_w = (ph_u + 12)%PH_NUM;
		rs232_len = 0;
		break;
	case 'R':
		t = ph_v;
		ph_v = ph_w;
		ph_w = t;
		rs232_len = 0;
		break;
	case 'P':
		if (rs232_len == 2) {
			period = rs232_buf[1];
			rs232_len = 0;
		}
		break;
	case 'A':
		if (rs232_len == 2) {
			ampl = rs232_buf[1];
			rs232_len = 0;
		}
		break;
	default:
		rs232_len = 0;
		break;
	}
}

void
setup_uart(unsigned long bitrate)
{
	word ubrr = F_CPU / (8 * bitrate) - 1;
	UBRR0H = (byte)(ubrr >> 8);
	UBRR0L = (byte)(ubrr & 0xFF);
	UCSR0A = 1 << U2X0;
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	UCSR0C = 3<<UCSZ00;
	rs232_len = 0;
	rs232_buf[0] = STOP;
}

void
read_config(void)
{
	byte i;
	for (i = 0; i < PH_NUM; i++) {
		sinus[i] = eeread(i);
	}
	ampl = eeread(PH_NUM);
	period = eeread(PH_NUM + 1);
}

int main(void)
{
	setup_ports();
	read_config();
	setup_timer();
	setup_uart(9600UL);
	go = 1;
	ph_u = 0;
	ph_v = 24;
	ph_w = 12;
	pwm_u = 10;
	pwm_v = 50;
	pwm_w = 50;

	sei();
	while (1) {
		register byte i;
		if (go) {
			SETBIT(PORTD, XU); 
			SETBIT(PORTD, XV); 
			SETBIT(PORTD, XW); 

			CLRBIT(PORTD, VU);
			CLRBIT(PORTD, VV);
			CLRBIT(PORTD, VW);

			for (i = 0; i < 100; i++) {
				if (i == pwm_u) {
					SETBIT(PORTD, VU); // DISABLE U
					CLRBIT(PORTD, XU);  // Set LOW
				}
				if (i == pwm_v) {
					SETBIT(PORTD, VV); // DISABLE VU
					CLRBIT(PORTD, XV);  // Set LOW
				}
				if (i == pwm_w) {
					SETBIT(PORTD, VW); // DISABLE W
					CLRBIT(PORTD, XW);  // Set LOW
				}
				_delay_loop_1(50);
				CLRBIT(PORTD, VU);
				CLRBIT(PORTD, VV);
				CLRBIT(PORTD, VW);
			}
		}
		PORTD = (1 << VU) | (1 << VV) | (1 << VW);
		_delay_loop_1(50);
	}
	return 0;
}
