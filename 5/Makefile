
obj-m := assignment5.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(shell pwd)
	
app: 
	gcc -o app_dl1   -pthread app_dl1.c
	gcc -o app_dl2   -pthread app_dl2.c


clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
