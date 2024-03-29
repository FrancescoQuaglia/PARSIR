all:
	gcc stubs.c -c -o ../lib/stubs.o -I../include 

test:	library
	gcc -c ../source/queue.c -o ../lib/queue.o -I ../include
	gcc -c ../source/barrier-synchronization.c -o ../lib/barrier-synchronization.o -I ../include
	gcc stubs.c -DSTUB_TEST ../lib/queue.o ../lib/barrier-synchronization.o -o stubs -I../include -lpthread

library:
	gcc -c ../source/queue.c -o ../lib/queue.o -I ../include
	gcc -c ../source/barrier-synchronization.c -o ../lib/barrier-synchronization.o -I ../include
