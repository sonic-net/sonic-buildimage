#
# usage: make KDIR=/path/to/kernel/build/area
#
MODNAME = pciesvc

obj-m := $(MODNAME).o

$(shell echo '#define PCIESVC_VERSION "'`date`'"' >version.h)

kpci :=

pciesvc-src := $(shell cd $(PWD) && ls pciesvc/src/*.c)
pciesvc-obj := $(patsubst %.c,%.o,$(pciesvc-src))
kpci += $(pciesvc-obj)

INCLUDES = -I$(PWD) \
	   -I$(PWD)/pciesvc/include \
	   -I$(PWD)/pciesvc/src

$(MODNAME)-y := $(kpci) kpcimgr_module.o kpcinterface.o kpci_entry.o \
		kpci_kexec.o kpci_test.o pciesvc_end.o


KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
UTS := X$(shell grep UTS_RELEASE $(KDIR)/include/generated/utsrelease.h)
REL := $(shell echo $(UTS) | awk '{ print $$3 }' | sed -e 's/"//g')

KCFLAGS = -fno-jump-tables -fno-stack-protector -fno-function-sections
KCFLAGS += -fno-data-sections -fno-store-merging -mstrict-align
KCFLAGS += $(INCLUDES) -DASIC_ELBA -DPCIESVC_SYSTEM_EXTERN
KOPT = KCFLAGS="$(KCFLAGS)"

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(KOPT) modules
	@mkdir -p $(REL)
	@mv $(patsubst %.o,%.ko,$(obj-m)) $(REL)
	@echo Checking for illegal relocations...
	tools/reloc_check $(REL)/$(MODNAME).ko

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
