SDK_HOME =/home/ninja07/Downloads/i586/sysroots

PATH := $(PATH):$(SDK_HOME)/pokysdk-linux/usr/bin/i586-poky-linux

CC = /home/ninja07/Downloads/i586/sysroots/i586-pokysdk-linux/usr/bin/i586-poky-linux-uclibc/i586-poky-linux-uclibc-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SROOT=$(SDK_HOME)/i586-poky-linux-uclibc/


all: main.c
	$(CC) -std=c99 -g -o main.o main.c -pthread -Wall -lrt --sysroot=$(SROOT)

run:
	sudo ./main.o input.txt
clean:
	rm -f *.o
