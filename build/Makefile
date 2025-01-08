
FLAGS = #-DNUMA_BALANCING -DNUMA_UBIQUITOUS
AFLAGS = -DSYSCALL_NUMBER=134 
HWSETUP = -DMEM_NODES=2 


all: runtime
	@ echo "The runtime environment of PARSIR has been compiled - object files are available in the lib directory"

runtime:
	cd ../scripts ; make get-HW-info
	gcc -c -O3 ../queue/queue.c -o ../lib/queue.o -I ../include $(FLAGS) $(HWSETUP)
	gcc -c ../queue/barrier-synchronization.c -o ../lib/barrier-synchronization.o -I ../include $(FLAGS) $(HWSETUP)
	gcc -c -O3 ../memory/memory.c -o ../lib/memory.o -I ../include $(FLAGS) $(HWSETUP)
	gcc -c -O3 ../stubs/stubs.c -o ../lib/stubs.o -I../include  $(FLAGS) $(HWSETUP)
	gcc -c -O3 ../random/random.c -o ../lib/random.o -I../include  $(FLAGS) $(HWSETUP)
	gcc -c -O3 ../engine/engine.c -o ../lib/engine.o -I../include  $(FLAGS) $(HWSETUP)
	gcc -c -O3 ../syscalls/syscalls.c -o ../lib/syscalls.o -I../include  $(FLAGS) $(AFLAGS) $(HWSETUP)
	ld  -r ../lib/queue.o ../lib/memory.o ../lib/barrier-synchronization.o ../lib/stubs.o ../lib/random.o ../lib/engine.o ../lib/syscalls.o -I../include -o ../lib/PARSIR-runtime.o

LKM:
	cd ../PTE-entry-switcher ; make ; make mount ; dmesg | grep acquiring | grep SCTH | tail -1 

rm-LKM:
	rmmod the_pte-entry-switcher

pcs: runtime
	make -C ../models/pcs _FLAGS="$(FLAGS)" _HWSETUP="$(HWSETUP)"
	gcc -o ../bin/PARSIR-simulator ../lib/PARSIR-runtime.o -I../include -lpthread -lnuma ../models/pcs/pcs.o -lm

highway: runtime
	make -C ../models/highway _FLAGS="$(FLAGS)" _HWSETUP="$(HWSETUP)"
	gcc -g -o ../bin/PARSIR-simulator ../lib/PARSIR-runtime.o -I../include -lpthread -lnuma ../models/highway/highway.o
