set xtrace yes

boot_intf="mmc"
boot_dev=0
boot_part=1

root_dev=0
root_part=2

# Note that U-boot and the kernel do not share the same device enumeration, so
# the root device and partitions numbers are not expected to be the same in
# general
default_bootargs="earlycon console=ttyPS0,11520 root=/dev/mmcblk0p2 rw rootwait earlyprintk"

uimage_load_addr="0x00200000"
fdt_load_addr="0x"

# Does my kernel have a device tree attached? It shouldnt, becuase i didn't
# attach it during the build phase of the kernel



bitstream="system.bit"
bitstream_size="0x0076fc2d"

# Copy the FPGA to the same location the kernel will eventually go
fatload "mmc" "${boot_dev}:${boot_part}" "${kernel_load_addr}" "${bitstream}"
fpga load 0 "${kernel_load_addr}" "${bitstream_size}"

# Testing for bootargs to be empty allows us to override them from the shell
if test "${bootargs}" = ""; then
    echo "Applying default kernel boot arguments"
    setenv bootargs "${default_bootargs}"
fi

# Test for the 

fatload ${devtype} ${devnum}:${distro_bootpart} 0x00200000 ${kernel_name};



# Load the FPGA image to the same location that the kernel is going to go
fatload mmc 1:1 0x00200000 system.bit
# Now instruct U-Boot to load the PL image - the size had better now change
# and per `stat system.bit` is 7797805 bytes
fpga load 0 0x00200000 0x76FC2D

# This setting of root= assumes that the device tree disables the MMC
# device on the SOM (Xilinx ID = 0) and enables the SD card (Xilinx ID = 1)
# and that the /etc/fstab on the rootfs partition matches this
if test "${bootargs}" = ""; then
	echo "Applying default kernel boot arguments"
	setenv bootargs "earlycon console=ttyPS1,115200 clk_ignore_unused root=/dev/mmcblk1p2 rw rootwait"
fi

fitimage_name=image.ub
kernel_name=Image
ramdisk_name=ramdisk.cpio.gz.u-boot
rootfs_name=rootfs.cpio.gz.u-boot

echo "#### Begin U-Boot environment variables ####"
printenv
echo "#### End U-Boot environment variables   ####"

for boot_target in ${boot_targets};
do
	echo "Trying to load boot images from ${boot_target}"
	if test "${boot_target}" = "jtag" ; then
		booti 0x00200000 0x04000000 0x00100000
	fi
	if test "${boot_target}" = "mmc0" || test "${boot_target}" = "mmc1" || test "${boot_target}" = "usb0" || test "${boot_target}" = "usb1"; then
		if test -e ${devtype} ${devnum}:${distro_bootpart} /uEnv.txt; then
			fatload ${devtype} ${devnum}:${distro_bootpart} 0x00200000 uEnv.txt;
			echo "Importing environment(uEnv.txt) from ${boot_target}..."
			env import -t 0x00200000 $filesize
			if test -n $uenvcmd; then
				echo "Running uenvcmd ...";
				run uenvcmd;
			fi
		fi
		if test -e ${devtype} ${devnum}:${distro_bootpart} /${fitimage_name}; then
			fatload ${devtype} ${devnum}:${distro_bootpart} 0x10000000 ${fitimage_name};
			bootm 0x10000000;
                fi
		if test -e ${devtype} ${devnum}:${distro_bootpart} /${kernel_name}; then
			fatload ${devtype} ${devnum}:${distro_bootpart} 0x00200000 ${kernel_name};
		fi
		if test -e ${devtype} ${devnum}:${distro_bootpart} /system.dtb; then
			fatload ${devtype} ${devnum}:${distro_bootpart} 0x00100000 system.dtb;
		fi
		if test -e ${devtype} ${devnum}:${distro_bootpart} /${ramdisk_name} && test "${skip_tinyramdisk}" != "yes"; then
			fatload ${devtype} ${devnum}:${distro_bootpart} 0x04000000 ${ramdisk_name};
			booti 0x00200000 0x04000000 0x00100000
		fi
		if test -e ${devtype} ${devnum}:${distro_bootpart} /${rootfs_name} && test "${skip_ramdisk}" != "yes"; then
			fatload ${devtype} ${devnum}:${distro_bootpart} 0x04000000 ${rootfs_name};
			booti 0x00200000 0x04000000 0x00100000
		fi
		booti 0x00200000 - 0x00100000
	fi
	if test "${boot_target}" = "xspi0" || test "${boot_target}" = "qspi" || test "${boot_target}" = "qspi0"; then
		sf probe 0 0 0;
		sf read 0x10000000 0xF40000 0x6400000
		bootm 0x10000000;
		echo "Booting using Fit image failed"

		sf read 0x00200000 0xF00000 0x1D00000
		sf read 0x04000000 0x4000000 0x4000000
		booti 0x00200000 0x04000000 0x00100000;
		echo "Booting using Separate images failed"
	fi
	if test "${boot_target}" = "nand" || test "${boot_target}" = "nand0"; then
		nand info;
		nand read 0x10000000 0x4180000 0x6400000
		bootm 0x10000000;
		echo "Booting using Fit image failed"

		nand read 0x00200000 0x4100000 0x3200000
		nand read 0x04000000 0x7800000 0x3200000
		booti 0x00200000 0x04000000 0x00100000;
		echo "Booting using Separate images failed"
	fi
done
