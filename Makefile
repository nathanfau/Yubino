WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wstrict-prototypes -Wno-array-bounds

CFLAGS := -Os -DF_CPU=16000000UL -mmcu=atmega328p -Iutils $(WARNINGS)

upload: program.hex
	avrdude -v -patmega328p -carduino -P/dev/ttyACM0 -b115200 -D -Uflash:w:$^

program.hex: program.elf
	avr-objcopy -O ihex $^ $@

program.elf: main.c utils/uart.c utils/ringbuffer.c utils/random.c micro-ecc/uECC.c
	avr-gcc $(CFLAGS) -o $@ $^

clean:
	rm -f *.elf *.hex utils/*.o micro-ecc/*.o *.o
