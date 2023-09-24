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

24 September 2023
-----------------
I've made a ton of progress in the past week - I switched from buildroot to a
Debian 12 full operating system derived from debootstrap and that has already
proven to be a wise decision. This is a project with a lot of development and I
can't really anticipate the need.

At this point I've gotten the following things accomplished:
- FSBL built manually using Vitis SDK and the embeddedsw FSBL source code along
  with `ps7_init.*` files pulled from the exported hardware design
- U-boot ELF built manually from Xilinx source tree tag `xilinx-v2023.1`
- Kernel 6.1 built from the Xilinx source tree tag
  `xlnx_rebase_v6.1_LTS_2023.1_update` (a bit more on the kernel in a second)
- I have scripts built to gather these components together along with
  debootstrap and build a root filesystem with an SSH server, build tools, etc.
- Haven't automated SD card image generation (and I'm not sure that I need to,
  since I'm happy with what I have no and don't need to do much after booting -
  still some bugs to probably address later, but for now, I'm good with it).

Programming the FPGA
~~~~~~~~~~~~~~~~~~~~
So, this is where I've run into a snag. At some point, the old approach of `cat
static.bit > /dev/xdevcfg` was removed from the Linux kernel. A more accurate
way to put it is that the xdevcfg driver was superseded by the fpga_manager
subsystem instead and the previous driver provided functionality that Xilinx
apparently had relied on.  I read through the patch review in the LKML that
discussed this when the Zynq driver was added to the kernel.  There's a couple
of things that I gleaned from this discussion:
* Xilinx devices may have hardware limitations on the sizes of the bitstream.
  Per the LKML, the Zynq requires bitstreams by multiples of 32-bits. Unclear if
  this is true for other devices like the Ultrascale+
* I'm not the first one to hate on the Xilinx tools for not being able to
  consistently and easily generate the required programming files. One of the
kernel devs had this to say:
```
Probably one of the key reasons that the "bit" format is still popular is that 
getting the Vivado tools to create a proper "bin" that will actually work on 
the Zynq is about as easy as nailing jelly to a tree. We've been using a 
simple Python script to do the bit->bin conversion for that reason.
```
* First, here's the error message I'm getting in the kernel ring buffer when I
  try to program the FPGA using the fpga_manager subsystem:
```
[Sun Sep 24 01:33:49 2023] fpga_manager fpga0: Error while parsing FPGA image header
[Sun Sep 24 01:47:04 2023] fpga_manager fpga0: writing uzed_sbc_top.bin to Xilinx Zynq FPGA Manager
[Sun Sep 24 01:47:04 2023] fpga_manager fpga0: Invalid bitstream, could not find a sync word. Bitstream must be a byte swapped .bin file
```
The source code (from my own kernel tree that I built) that is yielding this
message is from the `drivers/fpga/zynq-fpga.c` source file, which I'm not going
to reproduce entirely here. The gist of it all is that the fpga manager
subsystem is refusing to load the bitstream because the .bin I am loading has
the incorrect byte ordering.  Specifically, the driver code is looking for two
things: 1) a bitstream that is a multiple of 4 bytes, a hardware limitation on
how it supports DMA transfers during programming I believe 2) the sync word
needs to be in the proper orientation, which is what this function is actually
checking. Aside: pretty cool to see the actual sync word in the blob on the
disk. Also, per a statement in the patch discussion for the zynq-fpga driver,
the only difference between the .bit and .bin formats aside from the header that
apparently matter are the locations of the sync word.
```c
/* Sanity check the proposed bitstream. It must start with the sync word in
 * the correct byte order, and be dword aligned. The input is a Xilinx .bin
 * file with every 32 bit quantity swapped.
 */
static bool zynq_fpga_has_sync(const u8 *buf, size_t count)
{
	for (; count >= 4; buf += 4, count -= 4)
		if (buf[0] == 0x66 && buf[1] == 0x55 && buf[2] == 0x99 &&
		    buf[3] == 0xaa)
			return true;
	return false;
}
```

I did a hexdump of my bin file that was naively created with
`write_bitstream -force  uzed_sbc_top.bin -bin_file` and you can indeed see the 0xAA995566 in the file.

It appears to me that the programming problem is that I have not gotten Vivado
to give me the correct .bin for a Zynq 7000 that will work with this version of
the Linux kernel zynq-fpga driver. The root cause seems to be that prior
versions of Linux supported the xdevcfg interface, which fixed the byte ordering
problem transparently when users programmed the bitstream by just `cat`ting it
to a file in /dev/xdevcfg and at some point that functionality was removed and
replaced with the fpga manager subsystem instead.  It is unclear to me what
specifically broke and I'm not sure I want to try to understand it - what *is*
clear is that Xilinx seemed to think that their tools generated the appropriate
images by default using a single Tcl command and others pointed out that in
fact, it generates a .bin file but does not perform the byte swapping.  On byte
swapping, when they use that term, what they are referring to is byte ordering
within individual 32-bit chunks.  And you can see that from the sync word check.
In reality, it's actually a good thing for me that they do this check - it isnt
checking that I did something wrong, but rather that their tools don't generate
the correct images by default.  If it had just tried to shove a misordered
stream of bytes into the FPGA, I'd have had no insight into it at all and I'm
not sure how I would have found it.

Upshot of this is that a) reading the kernel source code was very helpful, and
having access to the source code myself without having it hidden away under
layers of Yocto nonsense was invaluable b) the Xilinx people do some really
questionable things and I'm glad the kernel developers are not enslaved to those
guys. I get the impression that the kernel developers tolerate the FPGA vendor
developers because they have a valid use case, but aren't thrilled to support
it. The FPGA subsystem should be hardware agnostic and the drivers are hit or
miss it would appear and that's c) I should not generalize from my experience
with one family to another. The culprit here is the zynq-fpga driver and its
conflation with the tool version, tcl commands, output product, and device
family that I'm building for. If I do this for an Ultrascale+ I may have an
entirely different experience and reading the kernel source code then may very
well be beneficial.

For reference:
https://lore.kernel.org/all/1478732303-13718-1-git-send-email-jgunthorpe@obsidianresearch.com/

As is usually the case, I was not the first one to encounter this problem and
people have thrown some python at the problem.  It looks like the canonical
solution might be to take the bitstream (.bit) file and use bootgen to generate
the correct binary.  Obviously, one of the first pieces of software I should
write natively on my board is a tool to do the byte swapping correctly in C on
the platform, so that I can take the .bit file from the .xsa and process it into
the .bin that is required by the fpga driver.  Yikes, I can't believe this
actually all makes sense to me.

Ok, so it all works now, the FPGA gets programmed, the DONE light goes on, the
hardware manager connected to the board sees that it is programmed and that
there is an `hw_axi` core present on the board, which is accurate, since I have
a JTAG to AXI master in the current hardware build.  So, yeah, that's a big win.
The whole stack now belongs to me. To get the .bin file in the format I needed,
I used bootgen along with a .bif of the form:
```cpp
bitstream_convert:
{
	uzed_sbc_top.bit
}
```
And then ran
`bootgen -arch zynq -image uzed_sbc_top.bif -process_bitstream bin` which, when
I hex dumped it, had the proper ordering of the sync word (and diffing it with
what came out of Vivado looked to be entirely swapped. And by swapped, I mean
LSB first instead of MSB in each 32-bit word (not reversing the entire file).
What's odd is that the size that came out differed by one 32-bit value (the
Vivado version was smaller by 4 bytes).  Tough to know why - the four missing
bytes appeared to be at the end of the file but I didn't really dig any further.
For now, I've got an entire working stack from FSBL all the way to programming
the FPGA manually from Linux that supports my use case (that's a key detail, it
might not support someone else's).

