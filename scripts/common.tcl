proc err {msg} {
    puts stderr "ERROR: ${msg}"
	return 0
}

proc status {msg} {
	puts stdout "STATUS: ${msg}"
	return 0
}

