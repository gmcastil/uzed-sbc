#!/bin/bash

# Project constants
export UZED_SBC_ARCH="arm"
export UZED_SBC_PLATFORM="zynq"
export UZED_SBC_BOARD="uzed"
export UZED_SBC_LINUX_IMG="uImage"
export UZED_SBC_CROSS_COMPILE="arm-linux-gnueabihf-"

# Directories
export UZED_SBC_EXTERN_DIR="../extern"
export UZED_SBC_BUILD_DIR="../build"
export UZED_SBC_IMAGE_DIR="../images"

export UZED_SBC_LINUX_DIR="${UZED_SBC_EXTERN_DIR}/linux-xlnx"
# Placeholder for now until I can build my own FSBL without having to engage
# with Vitis
export UZED_SBC_FSBL_DIR="/home/castillo/vitis-workspaces/2023.1/uzed_sbc_fsbl/Debug"
export UZED_SBC_UBOOT_DIR="${UZED_SBC_EXTERN_DIR}/u-boot-xlnx"
# TODO More consistent to expect users to build it in the source tree itself
# (where I can do things like git clean -dfx and so forth) but leave for now so
# we can get done today.
export UZED_SBC_DTB_DIR="${UZED_SBC_BUILD_DIR}/dts"

# Constants for building a Debian root filesystem
export DEBIAN_ARCH="armhf"
export DEBIAN_SUITE="bookworm"
export DEBIAN_MIRROR="http://deb.debian.org/debian"

# There are a ton of packages that either do not get pulled in by default and
# need to be or are extremely useful and it is far quicker to do it now rather
# than from an SD card
DEBIAN_INCLUDE="sudo,device-tree-compiler,i2c-tools,build-essential,screen"
DEBIAN_INCLUDE+=",pkg-config,libglib2.0-dev,texinfo,dh-autoreconf"
DEBIAN_INCLUDE+=",autotools-dev,git,debhelper,fakeroot,devscripts,bc"
# Some of these packages do not get pulled in by default and need to be
# explicitly specified or the first stage will fail
DEBIAN_INCLUDE+=",libi2c-dev,cloud-guest-utils,dbus,perl-openssl-defaults"
# Useful packages, including the HAVEGE userspace entropy daemon, which makes
# SSHD start faster
DEBIAN_INCLUDE+=",vim,rsync,haveged,tree"
# More development stuff
DEBIAN_INCLUDE+=",manpages-dev,apt-file"
export DEBIAN_INCLUDE

# This is a case sensitive string and needs to match the Linux kernel entries
# in /proc for registered executable formats
export QEMU_BINFMT="qemu-arm"

# Second stage related constants
export DEBIAN_USER="castillo"
export DEBIAN_PASS="none"
export DEBIAN_HOSTNAME="uzed-sbc"
export DEBIAN_SERIAL="ttyPS0"

# Kernel boot arguments to be supplied via extlinux (note that the chosen node
# in the device tree may have boot arguments as well)
export UBOOT_BOOTARGS="earlycon console=${DEBIAN_SERIAL},115200 clk_ignore_unused root=/dev/mmcblk0p2 rootwait rw earlyprintk"

# Constants for the BOOT.BIN that will be assembled while building a boot
# filesystem
export BOOT_DTB_OFFSET="0x02A00000"

# Constants for generating SD card images
export UZED_SBC_SD_IMG_SIZE=4
export UZED_SBC_SD_SFDISK="mmcblk0.sfdisk"
export UZED_SBC_SD_PART_BOOT="p1"
export UZED_SBC_SD_PART_ROOT="p2"

