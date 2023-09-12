# All ELF and FDT are assumed to be in this directory
set IMAGE_DIR "../images"

# These can be files or symlinks to somewhere else on the filesystem (e.g.,
# a build directory somewhere)
set FSBL_ELF "fsbl.elf"
set UBOOT_ELF "u-boot.elf"
set FDT "system.dtb"

# Offsets
set FDT_OFFSET "0x00100000"

proc err {msg} {
  puts stderr "ERROR: ${msg}"
  return 0
}

proc status {msg} {
  puts stdout "STATUS: ${msg}"
  return 0
}

proc check_for_images {image_dir} {
  set retval 0
  if {! [file exists "${image_dir}/${::FSBL_ELF}"]} {
    err "Could not find FSBL ELF at ${image_dir}/${::FSBL_ELF}"
    set retval 1
  }
  if {! [file exists "${image_dir}/${::FDT}"]} {
    err "Could not find flattened device tree blob at ${image_dir}/${::FDT}"
    set retval 1
  }
  if {! [file exists "${image_dir}/${::UBOOT_ELF}"]} {
    err "Could not find U-Boot ELF at ${image_dir}/${::UBOOT_ELF}"
    set retval 1
  }
  return "${retval}"
}

if {[check_for_images "${IMAGE_DIR}"]} {
  err "Could not find boot images at ${IMAGE_DIR}"
} else {
  status "Found boot images at ${IMAGE_DIR}"
}

# Initiate connection to hardware server
if {[ catch {set chan_id [connect -host localhost -port 3121]} ]} {
  err "Could not connect to hardware server"
  exit 1
} else {
  status "Connected to hardware server on ${chan_id}"
}

# Need to target the APU cores directly in the event another processor
# is present like a MicroBlaze or something
if {[ catch {targets -set -filter {name =~ "APU"}} ]} {
  err "Could not set target to APU"
  exit 1
}
stop

# Issue a power on reset to an active connection
if {[ catch {rst -srst -clear-registers} ]} {
  err "Reset unsupported"
} else {
  status "Reset successful"
}

# Load the FSBL and resume execution
if {[ catch {targets -set -filter {name =~ "ARM*#0"}} ]} {
  err "Could not set target to ARM #0"
  exit 1
}

# There is no way to check that this is successful, so we just wait
# a bit and then hope for the best
status "Loaded ${IMAGE_DIR}/${FSBL_ELF}"
dow "${IMAGE_DIR}/${FSBL_ELF}"
con
after 3000
stop

# Load the device tree and U-boot ELF
# TODO: expand these so that FSBl, U-boot, FDT are all absolute paths
# to the console
dow -data "${IMAGE_DIR}/${FDT}" "${FDT_OFFSET}"
status "Loaded FDT blob at ${IMAGE_DIR}/${FDT} to ${FDT_OFFSET}"
dow "${IMAGE_DIR}/${UBOOT_ELF}"
status "Loaded ${IMAGE_DIR}/${UBOOT_ELF}"
con

# Don't leave stale connections
if { [catch {disconnect "${chan_id}"} ] } {
  err "Invalid channel ID specified"
  exit 1
} else {
  status "Disconnected from hardware server on ${chan_id}"
}

