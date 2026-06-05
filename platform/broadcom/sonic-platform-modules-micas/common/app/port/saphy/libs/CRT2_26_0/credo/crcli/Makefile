
include $(SRC_ROOT)/target.mk

all: crcli

crcli:
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) crcli.c linenoise.c fort.c argparse.c libunixsocket.c -o $(IMGDIR)/crcli
