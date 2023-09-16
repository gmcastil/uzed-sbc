Building
========
This document describes a process for building a Linux system for a Microzed
Zynq-7000 based SoC board entirely from source without having to engage the
Xilinx Petalinux tools.  It is by no means the only way and was to some extent,
borne out of my own frustrations with being unable to find a reasonable
description of how to build a minimal Linux system for this board without having
to engage the giant Xilinx ecosystem (e.g., Petalinux). While likely
generalizable to other boards, it is quite specifi to this one and to the
project at hand, namely my 6502 SBC that I'm implementing in the PL of the
board.  I've encountered a number of folks asking how to do this, but I've yet
to find any detailed instructions on how to get from a Vivado exported hardware
design to a bootable Linux system for this board without having to resort to
prebuilt images, downloading massive BSPs, or generating huge projects in Vitis
or Petalinux.

That said, you'll need Vitis to have access to the standalone Xilinx source
libraries and the cross-compiler with the target triplet `arm-linux-gnueabihf`.
These are largely required in order to build the first-stage bootloader (FSBL),
but after that, we use the Xilinx tools as little as possible and always from a
context that is simple to script.

I'm using the following tools, code, and versions:
* Xilinx 2023.1 Vivado and Vitis SDK on Ubuntu 20.04.3 LTS in a VirtualBox VM on
  a Windows 10 host (there are undoubtedly other platforms that will work, but I
  use this one because it is one of the officially supported releases).
* The tools are actually installed on another machine that is NFS mounted
  at /tools/Xilinx and prior to using them, need to be setup by sourcing the
  appropriate settings script
  (e.g., `source /tools/Xilinx/Vivado/2023.1/settings64.sh`
* To build the root filesystem, I'm using a tag of
  [buildroot](https://git.busybox.net/buildroot) (2023.02.4) which you will need
  to download. There are most certainly alternates here, including things like
  [debootstrap](https://wiki.debian.org/Debootstrap) which I have used on other
  projects which work well. For this application, I really wanted access to the
  cross compiler and do not expect I will have a need or desire for the more
  advanced package management that Debian offers.  I may revisit this decision
  rapidly if it becomes apparent that I do.
* The exported hardware design I'm using contains a couple of block RAM that are
  in hardware and are connected to the PS via an AXI interconnect as well as
  JTAG-to-AXI master that allows me to bypass the operating system layer and
  interact directly with the hardware, so long as the clock is running. This is
  not as much of a slam dunk as one might expect, since the kernel can and does
  shut off clocks that it deems unnecessary based on the device tree, so beware
  if you ever try something of the sort and get errors in the hardware manager
  that you need a free-running clock to keep the JTAG interface running. In that
  event, the `clk-ignore-unused` boot argument may be particularly relevant.
* For source code, I've pulled the Xilinx forks of U-boot and the Linux kernel
  from their Github page as well as their FSBL source code. Where possible, I
  would recommend using tags that correspond to the version of the tools you are
  using.

U-Boot
------
To build U-boot, follow these steps:

1. Clone the Xilinx U-Boot source code
[repository](http://github.com/Xilinx/u-boot-xlnx.git) and then check out one of
the tags (e.g., xilinx-v2023.1)
```bash
$ git clone git@github.com:Xilinx/u-boot-xlnx.git
$ cd u-boot-xlnx.git
$ git checkout xilinx-v2023.1
```
2. Make sure that the ARM cross compiler has been added to your `PATH`. I'm
using the toolchain that ships with Vitis, which I would recommend, but you
might be able to use the ARM cross compiler tools that can be installed through
your Linux distribution's package manager
```bash
$ source /tools/Xilinx/Vitis/2023.1/settings64.sh
```
3. Since we will be cross compiling, we'll need to set the following two
   environment variables
```bash
$ export CROSS_COMPILE='arm-linux-gnueabihf-'
$ export ARCH=arm
```
Note that the target triplet still has the trailing hyphen `-` in the string.
4. From the U-boot source code directory, you'll want to run the following
`make` targets
```bash
$ make distclean
$ make xilinx_zynq_virt_defconfig
$ make
```
If you wish to make customizations, run the `make menuconfig` target prior to
running `make`.  When the build is finished, you should see a `u-boot.elf` in
the top of the source directory. This will get used eventually when we build the
boot image.

Device Tree
-----------
The goal of this section is to simply capture the steps required to generate
complete device trees suitable for booting the Linux kernel and identifying PL
elements within in. The tools required are the XSCT command line tools as well
as the Xilinx device tree generator source repository, which is essentially a
giant set of Tcl scripts that are used to create various device tree source
include files from the contents of the exported hardware design from Vivado.
There are undoubtedly other methods for doing this involving higher degrees of
abstraction via Vitis, which apparently is the design flow now for boards like
the Kria KV260 and things like that. For newer boards, which I do not have an
entirely solid understanding of yet, the intent seems to be that users program
the FPGA using device tree overlays from Linux rather than via U-boot or the
FSBL.

1. Clone the Xilinx device tree generator source code
[repository](http:github.com/Xilinx/device-tree-xlnx) and check out the desired
tag. It is almost certain that you should check out a tag that corresponds to
the version of Vitis that you are going to use in subsequent steps.
```bash
$ git clone git@github.com:Xilinx/device-tree-xlnx.git
$ cd device-tree-xlnx
$ git checkout xilinx_v2023.1
```
2. Source the Vitis settings script, which will, among other things, place the
XSCT tools in our path
```bash
$ xsct
```
3. Now, from the Tcl console that the XSCT present, run the following commands
to open the exported hardware and generate the device tree source files
```tcl
% hsi open_hw_design <path/to/exported.xsa>
# For this example, I will assume you have invoked XSCT from the device-tree-xlnx
# source repository
% hsi set_repo_path <path/to/device-tree-xlnx>
```
Then create a software design for the targeted processor - these instructions
are lifted from the Xilinx wii and I do not claim to entirely understand what is
being done here
```tcl
% set procs [hsi get_cells -hier -filter {IP_TYPE=PROCESSOR}]
% puts "Processors extracted from XSA are ${procs}"
% hsi create_sw_design device-tree -os device_tree -proc ps7_cortexa9_0
```
For more information on these, one can try running the commands with the `-help`
argument, but it should be noted that some of these arguments, including 
`-os device_tree` are lacking documentation of any kind. To some extent, these
are simply the process one needs to proceed with in order to get this version of
the tools to generate the device tree. To do this, run the `generate_target`
command and direct it to save the device tree source and include files somewhere
intelligent
```tcl
% hsi generate_target -dir <path/to/generated/dts>
% hsi close_hw_design [hsi current_hw_design]
% exit
```
After this process is completed, there should be a directory, which I've called
`generated-dts` that contains the following:
```bash
$ tree
.
├── device-tree.mss
├── include
│   └── dt-bindings
│       ├── clock
│       │   ├── xlnx-versal-clk.h
│       │   ├── xlnx-versal-net-clk.h
│       │   └── xlnx-zynqmp-clk.h
│       ├── dma
│       │   └── xlnx-zynqmp-dpdma.h
│       ├── gpio
│       │   └── gpio.h
│       ├── input
│       │   └── input.h
│       ├── interrupt-controller
│       │   └── irq.h
│       ├── net
│       │   ├── mscc-phy-vsc8531.h
│       │   └── ti-dp83867.h
│       ├── phy
│       │   └── phy.h
│       ├── pinctrl
│       │   └── pinctrl-zynqmp.h
│       ├── power
│       │   ├── xlnx-versal-net-power.h
│       │   ├── xlnx-versal-power.h
│       │   ├── xlnx-versal-regnode.h
│       │   └── xlnx-zynqmp-power.h
│       └── reset
│           ├── xlnx-versal-net-resets.h
│           ├── xlnx-versal-resets.h
│           └── xlnx-zynqmp-resets.h
├── pcw.dtsi
├── pl.dtsi
├── skeleton.dtsi
├── system.dts
├── system-top.dts
└── zynq-7000.dtsi

12 directories, 25 files
```
Now that the device tree source files have been created from the metadata
contained in the XSA, we can put it all together to create a source file that
can then be compiled into the flattened device tree blob needed to actually boot
the kernel.

(TBD: not sure if the device tree is needed by U-Boot, at least in the way I'm using it).

4. Now, we will use the GCC preprocessor to consolidate all of the source files
into a device tree source file that can be fed into the device tree compiler.
```bash
$ gcc \
	-I ./ \
	-E -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
	-o system.dts system-top.dts
```
Now, we can use the DTC to compile the sources into a device tree blob
```bash
$ dtc -I dts -O dtb -o system.dtb system.dts
```
I would suggest dumping a copy of the flattened device tree on the boot medium
later so it can be compared against a running device tree. Also, note that if
desired, one can manually edit the device tree souce files at this point prior
to compiling and deploying it to the board (e.g., modifying default kernel
command line arguments via the `chosen` node in the device tree).  Also, at this
point, it is wise to inspect the compiled device tree and make sure that
whatever IP was desired actually exist.  For example , in my case I have two
block RAM that are exposed to the PS at 0x40000000 and 0x42000000 and if I dump
out the compiled device tree, I get the following section
```c
amba_pl {
	#address-cells = <0x01>;
	#size-cells = <0x01>;
	compatible = "simple-bus";
	ranges;

	axi_bram_ctrl@40000000 {
		clock-names = "s_axi_aclk";
		clocks = <0x01 0x0f>;
		compatible = "xlnx,axi-bram-ctrl-4.1";
		reg = <0x40000000 0x8000>;
		xlnx,bram-addr-width = <0x0d>;
		xlnx,bram-inst-mode = "EXTERNAL";
		xlnx,ecc = <0x00>;
		xlnx,ecc-onoff-reset-value = <0x00>;
		xlnx,ecc-type = <0x00>;
		xlnx,fault-inject = <0x00>;
		xlnx,memory-depth = <0x2000>;
		xlnx,rd-cmd-optimization = <0x00>;
		xlnx,read-latency = <0x01>;
		xlnx,s-axi-ctrl-addr-width = <0x20>;
		xlnx,s-axi-ctrl-data-width = <0x20>;
		xlnx,s-axi-id-width = <0x0d>;
		xlnx,s-axi-supports-narrow-burst = <0x00>;
		xlnx,single-port-bram = <0x01>;
	};

	axi_bram_ctrl@42000000 {
		clock-names = "s_axi_aclk";
		clocks = <0x01 0x0f>;
		compatible = "xlnx,axi-bram-ctrl-4.1";
		reg = <0x42000000 0x4000>;
		xlnx,bram-addr-width = <0x0c>;
		xlnx,bram-inst-mode = "EXTERNAL";
		xlnx,ecc = <0x00>;
		xlnx,ecc-onoff-reset-value = <0x00>;
		xlnx,ecc-type = <0x00>;
		xlnx,fault-inject = <0x00>;
		xlnx,memory-depth = <0x1000>;
		xlnx,rd-cmd-optimization = <0x00>;
		xlnx,read-latency = <0x01>;
		xlnx,s-axi-ctrl-addr-width = <0x20>;
		xlnx,s-axi-ctrl-data-width = <0x20>;
		xlnx,s-axi-id-width = <0x0d>;
		xlnx,s-axi-supports-narrow-burst = <0x00>;
		xlnx,single-port-bram = <0x01>;
	};
};
```
Note that we can see the clocks and offsets are all what would be expected. It's
worth keeping in mind that creating device trees is probably the place where the
most variability is to be expected, since that's where all of the board specific
details are more or less constrained to (by design). Hence, for that reason,
your mileage may very much vary.

Root Filesystem
---------------
Building the root filesystem is somewhat up to the user and I'll let you decide
how to do that.  The end result will need to be that you have a rootfs in a
tarball or image file that you can put on an SD card.  You will likely need to
modify the `/etc/fstab` file that it creates to make more specific to your
installation. Buildroot for example creates virtually nothing in `/etc` and you
will need to build the system up as you see fit.

Linux Kernel
------------
Building the Linux kernel follows a similar procedure:

1. Clone the Xilinx kernel source code
[repository](http:github.com/Xilinx/linux-xlnx) and check out one of the tags.
```bash
$ git clone git@github.com:Xilinx/linux-xlnx.git
$ cd linux-xlnx
$ git checkout xlnx_rebase_v6.1_LTS_2023.1_update
```
2. Again, make sure that you system includes your toolchain of choice in your
`PATH`:
```bash
$ source /tools/Xilinx/Vitis/2023.1/settings64.sh
```
3. Since we will be cross compiling, we'll need to indicate the target triplet
```bash
$ export CROSS_COMPILE='arm-linux-gnueabihf-'
```
4. Now run the following `make` targets
```bash
$ make distclean
$ make ARCH=arm xilinx_zynq_defconfig
$ make ARCH=arm UIMAGE_LOADADDR=0x8000 menuconfig
$ make ARCH=arm UIMAGE_LOADADDR=0x8000 uImage >build_$(date +%H%M%S-%m%d%Y).log 2>&1
```
I would recommend capturing the build log with something like the last line,
which allows you to use the command history to run the same command, but
preserve all of the build logs sequentially.
5. To build the kernel modules, you will need to run that target as well and
then install them somewhere convenient so that you can transfer them to
`/lib/modules` on the SD card image or rootfs that is going to get used.
```bash
$ make ARCH=arm UIMAGE_LOADADDR=0x8000 modules
$ make ARCH=arm UIMAGE_LOADADDR=0x8000 INSTALL_MOD_PATH=</path/to/modules> modules_install
```
Unclear as to whether additional kernel modules will need to be built to support
Xilinx IP. Time will tell.

