#!/bin/bash

# Directories
export UZED_SBC_EXTERN_DIR="../extern"
export UZED_SBC_BUILD_DIR="../build"

# Constants for building a Debian root filesystem
export DEBIAN_ARCH="armhf"
export DEBIAN_SUITE="bookworm"
export DEBIAN_MIRROR="http://deb.debian.org/debian"
export DEBIAN_INCLUDE=

# QEMU related constants
export QEMU_ARCH="arm"

