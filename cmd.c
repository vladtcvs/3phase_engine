#include "defines.h"

#include "pwm.h"
#include "uart.h"

static uint16_t read_uint(const char *arr, byte len)
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

static void set_cw(void)
{
	ph_v = (ph_u + 12)%PHASES;
	ph_w = (ph_u + 24)%PHASES;
}

static void set_ccw(void)
{
	ph_w = (ph_u + 12)%PHASES;
	ph_v = (ph_u + 24)%PHASES;
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


static void set_amplitude(const char *buf, int8_t len)
{
	if (len > 1) {
		uint16_t rw = read_uint(&buf[1], len - 1);
		ampl = rw < 256 ? rw : 255;
	} else {
		tx_byte('\n');
		tx_byte('\r');
		print_int_dec(ampl);
	}
}

static void set_freq(const char *buf, int8_t len)
{
	if (len > 1) {
		uint16_t rw = read_uint(&buf[1], len - 1);
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

static void read_eeprom(const char *buf, int8_t len)
{
	if (len < 3)
		return;
	uint16_t addr = read_uint(&buf[2], len - 2);
	if (addr > 256)
		return;
	tx_byte('\n');
	tx_byte('\r');
	print_int_dec(eeread(addr));
}

static void write_eeprom(const char *buf, int8_t len)
{
	uint16_t addr = read_uint(&buf[2], len - 2), val;
	byte i;
	if (addr > 256)
		return;
	for (i = 2; i < len; i++)
		if (buf[i] == '=')
			break;
	if (i >= len - 1) {
		val = 0;
	} else {
		i++;
		val = read_uint(&buf[i], len - i);
	}
	if (val >= 256)
		return;
	eewrite(addr, val);
}

void shell_handle(const char *buf, int8_t len)
{
	switch (buf[0]) {
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
		set_amplitude(buf, len);
		break;
	case 'F':
		set_freq(buf, len);
		break;
	case 'E':
		if (len == 1)
			break;
		if (buf[1] == 'R')
			read_eeprom(buf, len);
		else if (buf[1] == 'W')
			write_eeprom(buf, len);
		break;
	}
}
