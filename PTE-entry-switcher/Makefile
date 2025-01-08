
obj-m += the_pte-entry-switcher.o
the_pte-entry-switcher-objs += pte-entry-switcher.o  lib/scth.o

A = $(shell cat /sys/module/the_usctm/parameters/sys_call_table_address)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	insmod the_pte-entry-switcher.ko the_syscall_table=$(A)
