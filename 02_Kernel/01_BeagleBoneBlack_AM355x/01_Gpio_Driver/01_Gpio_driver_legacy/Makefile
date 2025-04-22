BBB_KERNEL := /lib/modules/$(shell uname -r)/build
TOOLCHAIN  := 

obj-m := gpio.o

all:
	make -C $(BBB_KERNEL) M=$(shell pwd) modules
	
clean:
	make -C $(BBB_KERNEL) M=$(shell pwd) clean
