MCU=attiny202
F_CPU=400000
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -B "C:\dev\avr8-gnu-toolchain-win32_x86_64\avr\gcc\dev\attiny202" -DF_CPU=${F_CPU} -I.
TARGET=main
SRCS=main.c

all:
		${CC} ${CFLAGS} -o bin/${TARGET}.bin ${SRCS}
		${OBJCOPY} -j .text -j .data -O ihex bin/${TARGET}.bin bin/${TARGET}.hex
		avrdude -c serialupdi -p t202 -P COM7 -e -U flash:w:"bin\main.hex":a 