#
# usage: make KDIR=/path/to/kernel/build/area DRIVER=driver_name
#

DRIVER=pciesvc_loader

$(shell echo '#define PCIESVC_VERSION "'`date`'"' >version.h)

obj-m := $(DRIVER).o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
UTS := X$(shell grep UTS_RELEASE $(KDIR)/include/generated/utsrelease.h)
REL := $(shell echo $(UTS) | awk '{ print $$3 }' | sed -e 's/"//g')
INCLUDES = -I$(PWD) -I$(PWD)/..
KCFLAGS = $(INCLUDES)
KOPT = KCFLAGS="$(KCFLAGS)"

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(KOPT) modules
	@mkdir -p $(REL)
	@mv $(PWD)/$(patsubst %.o,%.ko,$(obj-m)) $(REL)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

