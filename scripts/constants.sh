#!/bin/bash

# Directories
export UZED_SBC_EXTERN_DIR="../extern"
export UZED_SBC_BUILD_DIR="../build"

export UZED_SBC_LINUX_DIR="${UZED_SBC_EXTERN_DIR}/linux-xlnx"
export UZED_SBC_FSBL_DIR=
export UZED_SBC_UBOOT_DIR="${UZED_SBC_EXTERN_DIR}/u-boot-xlnx"
export UZED_SBC_DTB_DIR="${UZED_SBC_BUILD_DIR}/device-tree"

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
DEBIAN_INCLUDE+=",vim,rsync,haveged"
export DEBIAN_INCLUDE

# This is a case sensitive string and needs to match the Linux kernel entries
# in /proc for registered executable formats
export QEMU_BINFMT="qemu-arm"

# Second stage related constants
export DEBIAN_USER="debian"
export DEBIAN_PASS="none"
export DEBIAN_HOSTNAME="uzed-sbc"
export DEBIAN_SERIAL="ttyPS0"

