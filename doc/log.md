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

Next step is to engage the block RAM that is in the FPGA and allegedly exposed
by the device tree.  I have no idea where to begin, aside from the BRAM access
page in the Xilinx wiki.  But, at this point, my next goal is:
- Write some Tcl commands to load a pattern to the BRAM from JTAG and read it
  back via JTAG
- Then write some code on the board to let me read the same pattern from Linux
- Then write some code to set a pattern and read the pattern back and check
- Then write some code to read / write sections of the two memories and hammer
  away on the BRAM and AXI interconnect logic
- At that point, the SBC ROM and RAM should be pretty solid.

The win criteria for this is that I do an overnight run of memory accesses
between the two block RAM from Linux without errors.  A summary of a few billion
memory accesses between the two memories is what I'm expecting.  An interesting
diversion would be to spawn two threads that are hammering away on the two
memories separately.

Once that's done, then we'd start looking at putting some hardware on the other
end to talk to, probably a control register interface for the SBC.

Couple questions - why does /proc/meminfo not show the complete amount of DDR
memory? What is the offset of DDR memory and why is it not in the device tree?
Are level shifters enabled? Can I discover this from userspace and alert the
user during startup?

User space driver for memory

02 October 2023
---------------
Another big success - have a kernel module loaded that triggers messgaes upon
removal and insertion (which I had earlier) but now it does it in response to
the platform subsystem matching it to my own entry in the device tree.

Really quickly, since I'm in a hurry, I needed an entry that matched my
compatibility string in my kernel module. So I created my own basic.dtsi and
then added that to the set of the stuff that I was compiling into my device
tree.  At some point, I might want to just put my entire device tree under
revision control and then include different files for different configurations,
but for now, i'll stick to what I've got which is tied to the Xilinx stuff.
Actually, that might be a better idea since it can change based upon the
exported hardware definition and I do not want to touch that stuff because it
can affect the FSBL (mismatches between the FSBL initiatalization code and my
device tree is not something i want to debug).

At any rate, to incoprorate what i have now, I'm just adding a single line to
the top level `system-top.dts` file to include a `basic.dtsi` which I'm going to
store in my source directory.  So, changes to my driver and device tree entry
will be tracked, but i dont have to put the entire device tree under revision
control yet. Also, at some point i'll want to make a makefile to rebuild the
device tree when the inputs change.  For now, I'll have to manually make the
changes.

Seriously - big win.  Also, some things to keep in mind that were absolutely
badass.  First, huge positive move to build my own FSBL and U-boot and kernel
and cut myself off from the Petalinux nonsense. Second, huge move to use
extlinux instead of boot.scr or FSBL programming, etc. Putting the kernel and
the device tree and the bootloaders in separate places and using distro boot
allowed me to very quickly rebuild a device tree, create an experimental boot
entry in the menu and now I dont have to worry about bricking the board if I
screw something up. I can easily switch between kernels, device trees, etc. and
eventually my own modules in `/lib/modules/<uname -r>`.

Not sure how what I'm doing is going to carry over to understanding the Xilinx
BRAM to Linux interface.  But i'll get there.

13 October 2023
---------------
I've been able to get data into and out of the block RAM from Linux via the UIO
subsystem interface.  Huge win.

Next things to do - first, need to write some memory verificaton software which
demonstrates that I can load, dump, purge, save, etc. data from both memories
from software.

From the hardware side, I need to make some decisions about clocking and resets.
I've written a bit about this - I really want to start working on some hardware,
but I really need to get the software side taken care of while it is clear in my
mind.

Operations to support:
- Peek and poke operations
- Purge entire memory
- Test memory with static pattern, address, walking ones, PRN
- Load file to memory
- Dump memory to file
- Load file to offset
- Dump file from offset

- As far as interface options and such go, I want to be able to pipe to standard
  out (so that I can pipe to xxd or hexdump) or save to a file. Or maybe even ASCII
  as well.
- As much fun as it would be to get hardware going now, the right thing to do is
  stick to the plan, get memory locked down, and then start working on clocks
  and resets.

21 October 2023
---------------
Haven't done anything this week - been working a lot (on somethign that largely
mirrors this little side project).  This morning, I'm trying to get udev to
set permissions for me when UIO devices are created. This [link](http://www.reactivated.net/writing_udev_rules.html)
is helpful and the only rule I needed to write was something like this:

`ACTION=="add", KERNEL=="uio[0-9]*", SUBSYSTEM=="uio", ATTR{name}=="axi_bram_ctrl", MODE="0666"`

This needs to get added to a rule in `/etc/udev/rules.d/` and it should make UIO
devices on this platform read and writable by a normal user.  There's lots of
extra complexity here, if you want it, but this is fine for what I need.  This
basically just makes BRAM controllers that show up as UIO devices accessible
after those nodes are created.

12 November 2023
----------------
Lot of thought the last day or so about how to clock the PL design - it wasn't
as simple as I thought it would be. I don't want to dismantle my set up in my
office, so I might do some prototyping with my BASYS3 board in the lab to fully
debug the clock and reset generator circuits, and then port them to the Microzed
here.  I believe that board has a 33.333MHz on the board, so I shouldn't have to
make many changes once I'm happy with it in the fabric.  It's a lot easier to
prototype PL stuff in an actual FPGA than it is in the Zynq.  Here's where we're
headed:

- Synthesize a 236.25MHz base clock using an MMCM
- Synthesize a master clock by dividing the base clock by 11 at the MMCM
- From the master clock, synthesize the following:
  - CPU clock enable by dividing by 12
  - PPU clock enable by dividing by 4
- The relationships of these two output clocks need to be deterministic and
  stable from route to route. Furthermore, there may need to be a third output
  to serve as the block RAM clock.  This one still has me a little confused, but
  it's not something I have to solve today.

Once these are done and working in hardware, I'll address reset.


