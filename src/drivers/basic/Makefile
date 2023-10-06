# Cross compile for ARM - better make sure these are in your PATH
export ARCH		:= arm
export CROSS_COMPILE	:= arm-linux-gnueabihf-
# The Linux kernel source code here needs to have already been built with the
# appropriate compiler
export KDIR		:= ../../../extern/linux-xlnx

PWD			:= $(shell pwd)

obj-m			:= basic.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

# Do not attempt to build an object file but generate preprocessed source instead
pp:
	$(MAKE) -C $(KDIR) M=$(PWD) basic.i

clean:
	$(RM) basic.ko
	$(RM) .basic.ko.cmd
	$(RM) basic.mod
	$(RM) basic.mod.c
	$(RM) .basic.mod.cmd
	$(RM) basic.mod.o
	$(RM) .basic.mod.o.cmd
	$(RM) basic.i
	$(RM) .basic.i.cmd
	$(RM) basic.o
	$(RM) .basic.o.cmd
	$(RM) modules.order
	$(RM) .modules.order.cmd
	$(RM) Module.symvers
	$(RM) .Module.symvers.cmd

