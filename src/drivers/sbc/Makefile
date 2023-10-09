# Cross compile for ARM - better make sure these are in your PATH
export ARCH		:= arm
export CROSS_COMPILE	:= arm-linux-gnueabihf-
# The Linux kernel source code here needs to have already been built with the
# appropriate compiler
export KDIR		:= ../../../extern/linux-xlnx

PWD			:= $(shell pwd)

obj-m			:= sbc-bram.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

# Do not attempt to build an object file but generate preprocessed source instead
pp:
	$(MAKE) -C $(KDIR) M=$(PWD) sbc-bram.i

clean:
	$(RM) sbc-bram.ko
	$(RM) .sbc-bram.ko.cmd
	$(RM) sbc-bram.mod
	$(RM) sbc-bram.mod.c
	$(RM) .sbc-bram.mod.cmd
	$(RM) sbc-bram.mod.o
	$(RM) .sbc-bram.mod.o.cmd
	$(RM) sbc-bram.i
	$(RM) .sbc-bram.i.cmd
	$(RM) sbc-bram.o
	$(RM) .sbc-bram.o.cmd
	$(RM) modules.order
	$(RM) .modules.order.cmd
	$(RM) Module.symvers
	$(RM) .Module.symvers.cmd

