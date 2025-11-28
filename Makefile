WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wstrict-prototypes -Wno-array-bounds

# Mode bypass (0 = normal, 1 = bypass)
BYPASS ?= 0

CFLAGS := -Os -DF_CPU=16000000UL -mmcu=atmega328p -Iutils $(WARNINGS)
CFLAGS += -DBYPASS_USER_CONSENT=$(BYPASS)

upload: program.hex
	avrdude -v -patmega328p -carduino -P/dev/ttyACM0 -b115200 -D -Uflash:w:$^

bypass:
	$(MAKE) BYPASS=1

program.hex: program.elf
	avr-objcopy -O ihex $^ $@

program.elf: main.c utils/uart.c utils/ringbuffer.c utils/random.c micro-ecc/uECC.c utils/consent.c utils/storage.c utils/handlers.c
	avr-gcc $(CFLAGS) -o $@ $^

clean:
	rm -f *.elf *.hex utils/*.o micro-ecc/*.o *.o
