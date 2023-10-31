RM	= /usr/bin/rm

GCC	= /usr/bin/gcc
DTC	= /usr/bin/dtc

PPFLAGS	= -I . -E -nostdinc -undef -D__DTS__ -x assembler-with-cpp

DTSIFILES = pcw.dtsi \
	    pl.dtsi \
	    zynq-7000.dtsi 

system.dtb: system.dts
	# Use the device tree compiler as a formatting tool
	$(DTC) -I dts -O dts -o system.dts system.dts
	$(DTC) -I dts -O dtb -o system.dtb system.dts

system.dts: system-top.dts $(DTSIFILES)
	$(GCC) $(PPFLAGS) -o system.dts system-top.dts

clean:
	$(RM) -fv system.dtb
	$(RM) -fv system.dts

