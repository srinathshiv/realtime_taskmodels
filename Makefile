
CC =gcc

all: main.c
	$(CC) -g -o main.o main.c -pthread -Wall -lrt

run:
	sudo taskset -c 0 ./main.o ./input.txt

clean:
	rm -f *.o
