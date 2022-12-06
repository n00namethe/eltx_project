TOOLCHAIN = $(pwd)../toolchain/bin
CROSS_COMPILE = $(TOOLCHAIN)/mips-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar rc
STRIP = $(CROSS_COMPILE)strip
CFLAGS = $(INCLUDES) -O2 -Wall -march=mips32r2
CFLAGS += -muclibc
LDFLAG += -muclibc

.PHONY: all clean

all: api_motor_mq daemon_motor 

clean:
	rm -f bin/* obj/*.o lib/*.a

libcontrol_motor.o: libcontrol_motor.c
	$(CC) $(LDFLAG) -c -o obj/libcontrol_motor.o src/libcontrol_motor.c 

libmotor_daemon_api.a: libcontrol_motor.o
	$(AR) lib/libmotor_daemon_api.a obj/libcontrol_motor.o

api_motor_mq.o: api_motor_mq.c
	$(CC) $(LDFLAG) -c -o obj/api_motor_mq.o src/api_motor_mq.c -lm -lrt -Llib/ -lmotor_daemon_api

api_motor_mq: libmotor_daemon_api.a api_motor_mq.o
	$(CC) $(LDFLAG) -o bin/api_motor_mq obj/api_motor_mq.o -lm -lrt -Llib/ -lmotor_daemon_api 

daemon_motor.o: daemon_motor.c
	$(CC) $(LDFLAG) -c -o obj/daemon_motor.o src/daemon_motor.c -lrt -lm

daemon_motor: daemon_motor.o
	$(CC) $(LDFLAG) -o bin/daemon_motor obj/daemon_motor.o -lrt -lm


