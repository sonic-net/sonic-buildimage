# MDIO access library for gearbox PHYs behind the Nexthop FPGA.

LIB := libgbsyncdaccess.so

CC ?= gcc
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Werror -fPIC -fstack-protector-strong -Wformat \
	  -Werror=format-security -D_FORTIFY_SOURCE=2
LDFLAGS += -shared -Wl,-z,relro,-z,now


OBJS := nh_mdio_access.o

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c nh_mdio_access.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB)
