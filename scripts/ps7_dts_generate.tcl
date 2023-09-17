# Generate device tree source code from a Vivado exported hardware definition

set build_dir "../build"
set dts_dir "${build_dir}/dts"

set common "common.tcl"
set xlnx_dtg_repo "../extern/device-tree-xlnx"

proc usage {prog_name} {
    puts stdout "Usage: ${prog_name} \[filename\]"
}

# Import common Tcl functions
if { [file exists "${common}"] } {
    source "${common}"
} else {
    puts stderr "ERROR: Could not import ${common}"
    exit 1
}

# Need to provide a location for the exported hardware design as the first
# argument
if { "${argc}" != 1 } {
    usage "${argv0}"
    exit 1
} else {
    set xsa_file [lindex "${argv}" 0]
    if { [file exist "${xsa_file}"] } {
        set xsa_file [file normalize "${xsa_file}"]
    } else {
        err "XSA file not found"
    }
}

# Need to check that the Xilinx device tree generator source code has been
# checked out in the appropriate place or we go no further.
if { ! [file isdirectory "${xlnx_dtg_repo}"] } {
    err "Device tree generator source code not found at ${xlnx_dtg_repo}"
    exit 1
}

# Open the exported hardware design - note that some of these commands do not
# have return values so we have to do the best we can, usually by supressing any
# error messages that might occur and then checking afterwards for success
hsi::open_hw_design -quiet "${xsa_file}"
if { [hsi::current_hw_design -quiet] == "" } {
    err "Could not open hardware design"
    exit 1 
}

# Sets the software repository path to the Xilinx device tree generator
# repository - unclear if there is a way to verify this actually happens, but
# presumably a failure to identify it here will cause a later step to fail
hsi::set_repo_path -quiet "${xlnx_dtg_repo}"
set procs [hsi::get_cells -hier -filter {(IP_TYPE == PROCESSOR) && (NAME =~ "ps7*")}]
if { "${procs}" == "" } {
    err "Could not find Zynq processor IP in design"
    exit 1
}

# Create a device tree software design - note that this is not covered in the
# online documentation. See UG1400 for details.
hsi::create_sw_design device-tree -os device_tree -proc [lindex "${procs}" 0]
if { [hsi::get_sw_designs] == "" } {
    err "Could not create device tree software design"
    exit 1
}

status "Generating device tree in [file normalize ${dts_dir}]"
hsi::generate_target -dir "${dts_dir}"

hsi::close_hw_design -quiet [hsi::current_hw_design]
if { [hsi::current_hw_design -quiet] != "" } {
    err "Could not close hardware design"
    exit 1
} else {
    exit 0
}

