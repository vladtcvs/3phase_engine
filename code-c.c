#define F_CPU 18000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay_basic.h>

typedef unsigned char byte;
typedef unsigned short word;

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF - (1 << n)))

#define EU 1
#define EV 3
#define EW 5

#define UU 0
#define UV 2
#define UW 4

#define PH_NUM 36
byte sinus[PH_NUM];

byte ampl = 0, period;
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

#define FREQ_MAX ((uint16_t)488) /* 18e6/1024/36 */

#define DISABLED 0x00

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
	while (i < len && (arr[i] >= '0' && arr[i] <= '9')) {
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
	if (rw == 0)
		rw = 255;
	rw = FREQ_MAX / rw;
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
	period = FREQ_MAX/eeread(PH_NUM + 1);
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
	pwm_u = 10;
	pwm_v = 40;
	pwm_w = 70;

	sei();
	while (1) {
		register byte i;
		if (go) {
			int8_t cnt_u, cnt_v, cnt_w;
			/* Disable all phases */
			PORTC &= ~((1 << EU) | (1 << EV) | (1 << EW));

			/* Wait to turn mosfets off */
			_delay_loop_2(500);

			/* Switch all bridges to HIGH */
			PORTC |= ((1<<UU) | (1<<UV) | (1<<UW));

			/* Enable all phases */
			PORTC |= ((1<<EU) | (1<<EV) | (1<<EW));
			//_delay_loop_2(50);

			cnt_u = cnt_v = cnt_w = -1;
			for (i = 0; i < 100; i++) {
				if (i == pwm_u) {
					CLRBIT(PORTC, EU); // DISABLE U
					cnt_u = 3;
				}
				if (i == pwm_v) {
					CLRBIT(PORTC, EV); // DISABLE V
					cnt_v = 3;
				}
				if (i == pwm_w) {
					CLRBIT(PORTC, EW); // DISABLE W
					cnt_w = 3;
				}
				_delay_loop_2(200);
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
