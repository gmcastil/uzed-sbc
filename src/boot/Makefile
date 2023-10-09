MKIMAGE		:= mkimage
ARCH		:= arm

boot.scr: boot.cmd
	$(MKIMAGE) -T script -A $(ARCH) -d boot.cmd boot.scr

