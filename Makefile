TOOLCHAIN = $(pwd)../toolchain/bin
CROSS_COMPILE = $(TOOLCHAIN)/mips-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar cr
STRIP = $(CROSS_COMPILE)strip
CFLAGS = $(INCLUDES) -O2 -Wall -march=mips32r2
CFLAGS += -muclibc
LDFLAG += -muclibc




LDFLAG += -Wl,-gc-sections

all: motor_app api_motor_mq

motor_app: motor_app.o
	$(CC) $(LDFLAG) -o $@ $^ $(LIBS) -lm -lrt -std=gnu99
	$(STRIP) $@

api_motor_mq: api_motor_mq.o
	$(CC) $(LDFLAG) -o $@ $^ $(LIBS) -lm -lrt -L. -ljust_test

clean:
	rm -f *.o *~

distclean: clean
	rm -f $(SAMPLES)
