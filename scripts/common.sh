#!/bin/bash

function timestamp () {
    printf '%s' "$(date +%H%M%S-%m%d%Y)"
    return 0
}

function err () {
    local msg
    msg="${1}"
    printf 'ERROR:  [%s] %s\n' "$(timestamp)" "${msg}" >&2
    return 0
}

function status () {
    local msg
    msg="${1}"
    printf 'STATUS: [%s] %s\n' "$(timestamp)" "${msg}" >&1
    return 0
}

function depends_on () {
    local prog
    prog="${1}"
    err "Missing a required dependancy: ${prog}"
    return 0
}

# Returns true if file that it is called with is a valid Xilinx exported
# hardware definition
function is_xsa_file () {
    unzip -t "${1}" >/dev/null 2>&1
    return "${?}"
}

