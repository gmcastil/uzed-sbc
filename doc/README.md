Building
========

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


