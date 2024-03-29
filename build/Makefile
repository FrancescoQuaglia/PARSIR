all: runtime
	echo "Runtime environemt of PARSIR has been compiled - object files are available in their corresponding directories"

runtime:
	cd ../scripts ; make get-HW-info
	gcc -c -O3 ../queue/queue.c -o ../lib/queue.o -I ../include
	gcc -c ../queue/barrier-synchronization.c -o ../lib/barrier-synchronization.o -I ../include
	gcc -c -O3 ../memory/memory.c -o ../lib/memory.o -I ../include
	gcc -c -O3 ../stubs/stubs.c -o ../lib/stubs.o -I../include 
	gcc -c -O3 ../random/random.c -o ../lib/random.o -I../include 
	gcc -c -O3 ../engine/engine.c -o ../lib/engine.o -I../include 
	ld  -r ../lib/queue.o ../lib/memory.o ../lib/barrier-synchronization.o ../lib/stubs.o ../lib/random.o ../lib/engine.o -I../include -o ../lib/PARSIR-runtime.o

phold: runtime
	cd ../models/phold ; make
	gcc -o ../bin/PARSIR-simulator ../lib/PARSIR-runtime.o -I../include -lpthread -lnuma ../models/phold/phold.o
