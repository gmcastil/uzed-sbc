#!/bin/sh

# Note that U-boot and the kernel do not share the same device enumeration, so
# the root device and partitions numbers are not expected to be the same in
# general
default_bootargs="earlycon console=ttyPS0,111520 clk_ignore_unused uio_pdrv_genirq.of_id=generic-uio root=/dev/mmcblk0p2 rw rootwait earlyprintk"

bitstream="uzed_sbc_top.bit.bin"
bitstream_size="0x001fcba0"

uimage="uImage"
fdt="system.dtb"

boot_intf="mmc"
boot_dev=0
boot_part=1

# Flag to check after loading everything to determine if it is safe to boot
safe_to_boot=0

# WIll use this location to store the FPGA image temporarily prior to
# programming and then load the kernel over it.
kernel_load_addr="0x02000000"
# The device tree is loaded 1MB below the start of the kernel uimage
fdt_load_addr="0x01f00000"

# Copy the bitstream to the same location the kernel will eventually go
if test -e "${boot_intf}" "${boot_dev}":"${boot_part}" "${bitstream}"; then
    echo "Loading bitstream to ${kernel_load_addr}"
    fatload "${boot_intf}" "${boot_dev}":"${boot_part}" "${kernel_load_addr}" "${bitstream}"
    # Load the FPGA image to device 0 (presumably there are PL configurations
    # where this is non-zero)
    fpga load 0 "${kernel_load_addr}" "${bitstream_size}"
else
    echo "ERROR: Could not find bitstream. Will continue booting"
fi

# shellcheck disable=SC2154
# Testing for bootargs to be empty allows us to override them from the shell or
# provide reasonable defaults here
if test "${bootargs}" = ""; then
    setenv bootargs "${default_bootargs}"
    echo "Applying default kernel boot arguments: ${bootargs}"
else
    echo "User provided alternate boot arguments: ${bootargs}"
fi

# Load the kernel to the same location as the FPGA image was
if test -e "${boot_intf}" "${boot_dev}":"${boot_part}" "${uimage}"; then
    echo "Loading ${uimage} to ${kernel_load_addr}"
    fatload "${boot_intf}" "${boot_dev}":"${boot_part}" "${kernel_load_addr}" "${uimage}"
else
    echo "ERROR: Could not find ${uimage}"
    safe_to_boot=1
fi

# Load the FDT blob to the desired location
if test -e "${boot_intf}" "${boot_dev}":"${boot_part}" "${fdt}"; then
    echo "Loading flattened device tree ${fdt} to ${fdt_load_addr}"
    fatload "${boot_intf}" "${boot_dev}":"${boot_part}" "${fdt_load_addr}" "${fdt}"
else
    echo "ERROR: Could not find flattened device tree ${fdt}"
    safe_to_boot=1
fi

# If kernel and FDT were found and loaded we can boot
if test "${safe_to_boot}" -eq 0; then
    echo "Booting application from memory"
    bootm "${kernel_load_addr}" "-" "${fdt_load_addr}"
fi

