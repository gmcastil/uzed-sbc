#!/bin/bash

function err () {
    local msg
    msg="${1}"
    printf 'ERROR: [%s] %s\n' "$(date +%H%M%S-%m%d%Y)" "${msg}" >&2
    return 0
}

function status () {
    local msg
    msg="${1}"
    printf 'STATUS: [%s] %s\n' "$(date +%H%M%S-%m%d%Y)" "${msg}" >&1
    return 0
}

function is_xsa_file () {
    unzip -t "${1}" >/dev/null 2>&1
    return "${?}"
}

