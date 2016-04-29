CONFIG_MODULE_SIG=n 
CONFIG_MODULE_SIG_ALL=n
 obj-m += test.o
# obj-m += tasklet_exp.o
#obj-m += sched1.o
#obj-m += scheduler.o
#obj-m += trial.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
