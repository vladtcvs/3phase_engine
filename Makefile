MCU=atmega48pa
AVRDUDEMCU=m48p
CC=/usr/bin/avr-gcc
HOSTCC=/usr/bin/gcc
AS=/usr/bin/avra
CFLAGS=-Os -Wall -mcall-prologues -mmcu=$(MCU)
OBJ2HEX=/usr/bin/avr-objcopy
AVRDUDE=/usr/bin/avrdude
TARGET=code
EEPROM=eesin.bin
MKEE=./mkee

default: code-c.hex eesin.bin clean

program : code.hex eesin.bin
	$(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/parport0 -c dapa -U flash:w:code.hex -U eeprom:w:$(EEPROM):r

program-c : code-c.hex eesin.bin
	$(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/parport0 -c dapa -U flash:w:code-c.hex -U eeprom:w:$(EEPROM):r

eeprom: eesin.bin	
	$(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/parport0 -c dapa -U eeprom:w:$(EEPROM):r

code-c.hex: code-c.elf
	avr-objcopy -j .text -j .data -O ihex code-c.elf code-c.hex

code-c.elf: code-c.c
	$(CC) $(CFLAGS) -o code-c.elf code-c.c

code-c.S: code-c.c
	$(CC) $(CFLAGS) -S -o code-c.S code-c.c

code-c.bin: code-c.elf
	avr-objcopy -j .text -j .data -O binary code-c.elf code-c.bin

code.hex : code.asm
	$(AS) code.asm -o code.hex

eesin.bin : sin.txt ampl.txt freq.txt mkee
	$(MKEE) eesin.bin sin.txt ampl.txt freq.txt

mkee : mkee.c
	$(HOSTCC) mkee.c -o mkee

clean :
	rm -f *.obj *.o *.cof
