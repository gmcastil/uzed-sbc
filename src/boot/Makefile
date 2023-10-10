MKIMAGE		:= mkimage
ARCH		:= arm

RM		:= /bin/rm
SHELLCHECK	:= shellcheck

boot.scr: boot.cmd
	$(MKIMAGE) -T script -A $(ARCH) -d boot.cmd boot.scr

check:
	$(SHELLCHECK) boot.cmd

clean:
	$(RM) -f boot.scr

