#define F_CPU 18000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay_basic.h>

typedef unsigned char byte;
typedef unsigned short word;

#define SET_AMPLITUDE_FREQ

#define SETBIT(x, n) ((x) |= (1 << n))
#define CLRBIT(x, n) ((x) &= (0xFF - (1 << n)))

#define EU 1
#define EV 3
#define EW 5

#define UU 0
#define UV 2
#define UW 4

#define PH_OFF 0
#define PH_NUM 36

#define AMPL_START PH_NUM
#define FREQ_START (PH_NUM + 1)

#define AMPL_OFF (PH_NUM + 2)
#define AMPL_NUM 30

#define DELAY_STEP 100
#define DELAY_NUM 5

byte sinus[PH_NUM];

byte ampl = 0, period;
byte ph_u, ph_v, ph_w;
register byte pwm_u asm("r10"), pwm_v asm("r11"), pwm_w asm("r12");

#define RXS 16
byte rs232_len = 0;
byte rs232_buf[RXS];

#define TXS 16
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

static void tx_byte(byte c);

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
#define eewrite(addr, val) eeprom_write_byte((uint8_t*)((int)(addr)), val)
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

static const byte hexdig[] = "0123456789ABCDEF";

static void print_int_dec(byte val)
{
	if (val >= 100)
		tx_byte(hexdig[val/100]);
	if (val >= 10)
		tx_byte(hexdig[(val%100)/10]);
	tx_byte(hexdig[val%10]);
}

static void print_int_hex(byte val)
{
	tx_byte(hexdig[val >> 4]);
	tx_byte(hexdig[val & 0x0F]);
}

static void set_amplitude(void)
{
	if (rs232_len > 1) {
		uint16_t rw = read_uint(&rs232_buf[1], rs232_len - 1);
		ampl = rw < 256 ? rw : 255;
	} else {
		tx_byte('\n');
		tx_byte('\r');
		print_int_dec(ampl);
	}
}

static void set_freq(void)
{
	if (rs232_len > 1) {
		uint16_t rw = read_uint(&rs232_buf[1], rs232_len - 1);
		if (rw == 0)
			rw = 15;
#ifdef SET_AMPLITUDE_FREQ
		byte aid = rw/5;
		if (aid >= AMPL_NUM)
			aid = AMPL_NUM - 1;
		ampl = eeread(AMPL_OFF + aid);
#endif
		rw = FREQ_MAX / rw;
		period = rw < 256 ? rw : 255;
	} else {
		uint16_t fr;
		if (period < 2)
			fr = 255;
		else 
			fr = FREQ_MAX / period;
		tx_byte('\n');
		tx_byte('\r');
		print_int_dec(fr);
	}
}

static void read_eeprom(void)
{
	if (rs232_len < 3)
		return;
	uint16_t addr = read_uint(&rs232_buf[2], rs232_len - 2);
	if (addr > 256)
		return;
	tx_byte('\n');
	tx_byte('\r');
	print_int_dec(eeread(addr));
}

static void write_eeprom(void)
{
	uint16_t addr = read_uint(&rs232_buf[2], rs232_len - 2), val;
	byte i;
	if (addr > 256)
		return;
	for (i = 2; i < rs232_len; i++)
		if (rs232_buf[i] == '=')
			break;
	if (i >= rs232_len - 1) {
		val = 0;
	} else {
		i++;
		val = read_uint(&rs232_buf[i], rs232_len - i);
	}
	if (val >= 256)
		return;
	eewrite(addr, val);
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
	case 'E':
		if (rs232_len == 1)
			break;
		if (rs232_buf[1] == 'R')
			read_eeprom();
		else if (rs232_buf[1] == 'W')
			write_eeprom();
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
	} else {
		rs232_len_tx = 0;
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
		if (rs232_len < RXS - 1) {
			rs232_buf[rs232_len++] = rcv;
			tx_byte(rcv);
		} else {
			tx_byte('^');
		}
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
