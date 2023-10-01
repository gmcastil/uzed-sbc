PWD	:= $(shell pwd)

obj-m	:= basic.o

all:
	make -C $(KDIR) M=$(PWD)

clean:
	$(RM) basic.ko
	$(RM) .basic.ko.cmd
	$(RM) basic.mod
	$(RM) basic.mod.c
	$(RM) .basic.mod.cmd
	$(RM) basic.mod.o
	$(RM) .basic.mod.o.cmd
	$(RM) basic.o
	$(RM) .basic.o.cmd
	$(RM) modules.order
	$(RM) .modules.order.cmd
	$(RM) Module.symvers
	$(RM) .Module.symvers.cmd

