obj-m += pwm_driver.o
 
#KDIR = /lib/modules/$(shell uname -r)/build
KDIR = /home/balibrea/Documentos/fpga-linux/linux-xlnx-master
 
 
all:
	make -C $(KDIR)  M=$(shell pwd) modules
 
clean:
	make -C $(KDIR)  M=$(shell pwd) clean
