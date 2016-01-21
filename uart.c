#include "defines.h"

#include "uart.h"

#define RXS 16
static byte rs232_len = 0;
static byte rs232_buf[RXS];

#define TXS 16
static byte rs232_buf_tx[TXS];
static byte rs232_len_tx = 0;

#define STOP 'B'
#define START 'G'
#define CW 'R'
#define CCW 'L'
#define REV 'I'
#define PER 'T'
#define AMPL 'A'

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

static const byte hexdig[] = "0123456789ABCDEF";

void print_int_dec(byte val)
{
	if (val >= 100)
		tx_byte(hexdig[val/100]);
	if (val >= 10)
		tx_byte(hexdig[(val%100)/10]);
	tx_byte(hexdig[val%10]);
}

void print_int_hex(byte val)
{
	tx_byte(hexdig[val >> 4]);
	tx_byte(hexdig[val & 0x0F]);
}

void tx_byte(byte c)
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
