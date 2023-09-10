10 Sept 2023
------------
Big progress over the past few days:
* Built a basic Vivado hardware design that contains two block RAM controllers
  which will become the SRAM and EEPROM emulators that the SBC requires
* I was able to build the FSBL from the `xilinx_v2023.1` tag and my own Vivado
  exported hardware definition (which they call a Xilinx support archive or XSA
  file now).
* From the XSA file and their device tree generator Tcl scripts, I was able to
  create a pile of device tree source and include files and finally compile it
  into a flattened device tree blob. I dumped out the FDT afterwards and sure
  enough, the two block RAM controllers that are currently in the exported
  hardware design appear at the locations I specified in the address editor of
  Vivado.
* I also built U-boot from the Xilinx source code (I should have built from a
  tag here, but looks like I used 024eb37c1e38ab811abe5408d42069fbd7901824
  instead - little scary in retrospect, since it hasn't been touched since March
  of 2023) and used that device tree in the process. I built this using their
  standard Makefile approach and did not need to engage the Vitis tools except
  to use their cross-compiler.
* With the FSBL and U-Boot ELF files, the flattened device tree blob, and the
  bitstream from the XSA, I was able to use the XSCT, with the Microzed
  configured to boot over JTAG, to load and boot the FSBL, load the FDT to
  memory, program the FPGA, and finally boot U-boot and get to a U-boot console
  prompt. From the U-boot console, I could print out the FDT and saw my two BRAM
  controllers present right where they should be.

So this is a big win - it shows that I can reproduce the entire software stack
so far without having to create numerous SDK or PetaLinux projects to do
anything. If I decide to, I can actually write my own Makefile to build the FSBL
without creating Vitis projects and instead, just use their toolchain naturally
and grab the portions I need from the `standalone` BSP that ships with the Vitis
tool.

TBD - instructions on how I built the FSBL so I don't have to go through that
again

TBD - instructions on building the device tree blob

TBD - instructions on building U-boot for the Zynq-7000

For the remainder of my time this morning, I'm going to try to write up script
that will boot and program the FPGA (I'm going to just start using FPGA and SoC
interchangeably and leave it to context to indicate the more precise meaning)
from XSCT so that I do not have to rely on Vitis not falling on its face.

And as a note to anyone that happens to be reading this, I'm excited - because
a) I finally know what I'm going to build here and b) you get to read along as I
go. Let's get to scripting.

