#define F_CPU 18000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay_basic.h>

typedef unsigned char byte;
typedef unsigned short word;

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF - (1 << n)))

#define EU 0
#define EV 2
#define EW 4

#define UU 1
#define UV 3
#define UW 5

#define PH_NUM 36
byte sinus[PH_NUM];

byte ampl, period;
byte ph_u, ph_v, ph_w;
register byte pwm_u asm("r10"), pwm_v asm("r11"), pwm_w asm("r12");
byte rs232_len = 0;
byte rs232_buf[4];

#define TXS 8
byte rs232_buf_tx[TXS];
byte rs232_len_tx = 0;
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
	DDRC  = 0x3F;
	PORTC = 0x00;
	DDRD  = 0xF0;
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

static void set_cw(void)
{
	ph_v = (ph_u + 12)%PH_NUM;
	ph_w = (ph_u + 24)%PH_NUM;
}

static void set_ccw(void)
{
	ph_w = (ph_u + 12)%PH_NUM;
	ph_v = (ph_u + 24)%PH_NUM;
}

static void inverse(void)
{
	byte t = ph_v;
	ph_v = ph_w;
	ph_w = t;
}

static void start_stop(byte en)
{
	go = en;
	if (en)
		PORTD |= 0x10;
	else
		PORTD &= ~0x10;
}

static uint16_t read_uint(byte *arr, byte len)
{
	byte i = 0;
	uint16_t res = 0;
	while (i < len) {
		res *= 10;
		res += arr[i] - '0';
		i++;
	}
	return res;
}

static void set_amplitude(void)
{
	uint16_t rw = read_uint(&rs232_buf[1], rs232_len - 1);
	ampl = rw < 256 ? rw : 255;
}

static void set_freq(void)
{
	uint16_t rw = read_uint(&rs232_buf[1], rs232_len - 1);
	period = rw < 256 ? rw : 255;
}

void shell_handle(void)
{
	switch (rs232_buf[0]) {
	case 'B':
		start_stop(0);
		break;
	case 'G':
		start_stop(1);
		break;
	case 'C':
		set_cw();
 		break;
	case 'W':
		set_ccw();
		break;
	case 'I':
		inverse();
		break;
	case 'A':
		set_amplitude();
		break;
	case 'F':
		set_freq();
		break;
	}
}

ISR(USART_TX_vect)
{
	byte i;
	for (i = 0; i < rs232_len_tx - 1; i++)
		rs232_buf_tx[i] = rs232_buf_tx[i+1];
	if (rs232_len_tx > 0)
		rs232_len_tx--;
	if (rs232_len_tx > 0)
		UDR0 = rs232_buf_tx[0];
	else
		UCSR0B &= ~(1<<TXCIE0);
}

static void tx_byte(byte c)
{
	UCSR0B &= ~(1<<TXCIE0);
	if (rs232_len_tx == 0) {
		rs232_buf_tx[0] = c;
		UDR0 = c;
		rs232_len_tx = 1;
	} else if (rs232_len_tx < TXS) {
		rs232_buf_tx[rs232_len_tx++] = c;
		UCSR0B |= (1<<TXCIE0);
	}
}

ISR(USART_RX_vect)
{
	byte rcv = UDR0;
	UCSR0B &= ~(1<<RXCIE0);
	switch (rcv) {
	case '\r':
		shell_handle();
		rs232_len = 0;
		tx_byte('\r');
		tx_byte('\n');
		tx_byte('>');
		tx_byte(' ');
		break;
	case '\n':
		break;
	default:
		rs232_buf[rs232_len++] = rcv;
		tx_byte(rcv);
	}
	UCSR0B |= (1<<RXCIE0);
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
	pwm_u = 0;
	pwm_v = 0;
	pwm_w = 0;

	sei();
	while (1) {
		register byte i;
		if (go) {
			CLRBIT(PORTC, EU); 
			CLRBIT(PORTC, EV); 
			CLRBIT(PORTC, EW); 

			SETBIT(PORTC, UU);
			SETBIT(PORTC, UV);
			SETBIT(PORTC, UW);

			_delay_loop_1(50);

			SETBIT(PORTC, EU); 
			SETBIT(PORTC, EV); 
			SETBIT(PORTC, EW); 

			for (i = 0; i < 100; i++) {
				if (i == pwm_u) {
					CLRBIT(PORTC, EU); // DISABLE U
					CLRBIT(PORTC, UU);  // Set LOW
				}
				if (i == pwm_v) {
					CLRBIT(PORTC, EV); // DISABLE VU
					CLRBIT(PORTC, UV);  // Set LOW
				}
				if (i == pwm_w) {
					SETBIT(PORTC, EW); // DISABLE W
					CLRBIT(PORTC, UW);  // Set LOW
				}
				_delay_loop_1(50);
				SETBIT(PORTC, EU);
				SETBIT(PORTC, EV);
				SETBIT(PORTC, EW);
			}
		}
		_delay_loop_1(50);
	}
	return 0;
}
