MCU=atmega8
AVRDUDEMCU=m48p
CC=/usr/bin/avr-gcc
HOSTCC=/usr/bin/gcc
AS=/usr/bin/avra
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=$(MCU)
OBJ2HEX=/usr/bin/avr-objcopy
AVRDUDE=/usr/bin/avrdude
TARGET=code
EEPROM=eesin.bin
MKEE=./mkee

program : code.hex eesin.bin
	$(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/parport0 -c dapa -U flash:w:$(TARGET).hex -U eeprom:w:$(EEPROM):r

eeprom: eesin.bin	
	$(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/parport0 -c dapa -U eeprom:w:$(EEPROM):r

code.hex : code.asm
	$(AS) code.asm -o code.hex

eesin.bin : sin.txt ampl.txt freq.txt mkee
	$(MKEE) eesin.bin sin.txt ampl.txt freq.txt

mkee : mkee.c
	$(HOSTCC) mkee.c -o mkee

clean :
	rm -f *.hex *.o
