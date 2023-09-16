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

13 Sept 2023
------------
Building the kernel was straightforward:

First, have to source the Vitis setup script to bring the cross compiler
toolchain into the path. It's entirely possible that the compiler from the
distro package manager that gets installed would work to, but I'm trying to
follow the instructions from Xilinx as closely as I can.
```bash
$ source /tools/Xilinx/Vitis/2023.1/settings64.sh
$ export CROSS_COMPILE='arm-linux-gnueabihf-'
```
Then the kernel build process is quite normal
```
$ make mrproper
$ make ARCH=arm xilinx_zynq_defconfig
$ make ARCH=arm menuconfig
$ make ARCH=arm UIMAGE_LOADADDR=0x8000 uImage
```
One can (and should) pipe the output of the build command to a log file - I
commonly use something like `>build_$(date +%H%M%S-%m%d%Y).log 2>&1`,
background the process, and then just `tail -f` the log file.  I'm not entirely
sure that I understand the `UIMAGE_LOADADDR` yet.  Anyway, once the build has
finished, there will be a number of output products, including a compressed
kernel image with U-boot header (the zImage is self-extracting and then gets a
U-Boot wrapper added by `mkimage` so something like `file arch/arm/boot/uImage`
will indicate it is uncompressed when it actually is).  It's this image that
we're going to try to boot with the `bootm` command. Apparently, there are newer
versions of U-Boot that are capable of booting the zImage directly and don't
need the header stuck on there.

To load this to the board, from the U-Boot prompt, I ran the following:
```tcl
xsct% stop
xsct% dow -data "arch/arm/boot/uImage" 0x10000000
xsct% con
```
The terminal spat out the following:
```console
Zynq> bootm 0x10000000
## Booting kernel from Legacy Image at 10000000 ...
   Image Name:   Linux-6.1.40-xilinx
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    4672584 Bytes = 4.5 MiB
   Load Address: 00008000
   Entry Point:  00008000
   Verifying Checksum ... OK
Working FDT set to 0
   Loading Kernel Image
FDT and ATAGS support not compiled in

resetting ...
```
So, not exactly what I was hoping for - I would have like to see a bunch of
kernel messages followed by a panic where it failed to find a root filesystem,
but I'm not exactly sure what I'm missing yet.  I should be able to figure it
out soon.


15 September 2023
-----------------
It's alive.  I got all the way through to the kernel loading, finding the root
filesystem, running busybox, the whole nine yards. I needed to modify the U-boot
environment variables to include these
```tcsh
Zynq> setenv rootfstype ext4
Zynq> setenv bootargs "earlycon console=ttyPS0,115200 root=/dev/mmcblk0p2 rootwait rw earlyprintk"
```
In hindsight, that `rootfstype` variable might not be necessary, but the others
definitely were. The kernel serial port output was vanishing at some point
during the boot because I didn't have the `console=ttyPS0,115200` there and
without the `rootwait` directive, it would completely miss the SD card.


