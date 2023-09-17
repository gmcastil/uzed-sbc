#!/bin/bash

function err () {
    msg="${1}"
    printf 'ERROR: [%s] %s\n' "$(date +%H%M%S-%m%d%Y)" "${msg}" >&2
    return 0
}

function status () {
    msg="${1}"
    printf 'STATUS: [%s] %s\n' "$(date +%H%M%S-%m%d%Y)" "${msg}" >&1
    return 0
}

