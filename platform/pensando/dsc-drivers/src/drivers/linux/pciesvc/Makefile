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

INCLUDES = -I/sonic/platform/pensando/dsc-drivers/src/drivers/linux/pciesvc \
	   -I/sonic/platform/pensando/dsc-drivers/src/drivers/linux/pciesvc/pciesvc/include \
	   -I/sonic/platform/pensando/dsc-drivers/src/drivers/linux/pciesvc/pciesvc/src \
	   -I/usr/src/linux-headers-$(shell echo $(KVERSION) | sed 's/-arm64//')-common/include/linux
	   #-I/usr/include \
	   #-I$(PWD)
$(MODNAME)-y := $(kpci) kpci_get_entry.o kpcimgr_module.o kpcinterface.o \
	     kpci_entry.o kpci_kexec.o kpci_test.o pciesvc_end.o

KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)
UTS := X$(shell grep UTS_RELEASE $(KDIR)/include/generated/utsrelease.h)
REL := $(shell echo $(UTS) | awk '{ print $$3 }' | sed -e 's/"//g')

# find CC like the kernel build does
ifneq ($(LLVM),)
CC = clang
else
CC = $(CROSS_COMPILE)gcc
endif

# Check for cc flag support, if yes emit the flag else alternate (or null)
# Usage: CFLAGS += $(call cc-option,-fno-store-merging,)
cc-option = $(shell set -e;		\
	TMPO=/tmp/cc-option-$$$$-tmp.o;	\
	trap "rm -rf $$TMPO" EXIT;	\
	if ($(CC) -Werror $(1) -c -x c /dev/null -o $$TMPO) >/dev/null 2>&1; \
	then echo "$(1)";		\
	else echo "$(2)";		\
	fi)

KCFLAGS = -fno-jump-tables -fno-stack-protector -fno-function-sections -fno-dse -ffreestanding -fno-builtin
KCFLAGS += -fno-data-sections
KCFLAGS += $(call cc-option,-fno-store-merging,)
KCFLAGS += $(INCLUDES) -DASIC_ELBA -DPCIESVC_SYSTEM_EXTERN
KOPT = KCFLAGS="$(KCFLAGS)"

all:
	make pciesvc.ko
	make pciesvc_upg.ko

buildmod:
	$(info Compiler version: $(shell $(CC) --version | head -n 1))
	$(MAKE) -C $(KDIR) M=$(PWD) $(KOPT) modules
	@mkdir -p $(REL)
	@mv $(patsubst %.o,%.ko,$(obj-m)) $(REL)
	@echo Checking for illegal relocations...
	tools/reloc_check $(REL)/$(MODNAME).ko

pciesvc.ko:
	$(MAKE) buildmod MODNAME=pciesvc

pciesvc_upg.ko:
	$(MAKE) buildmod MODNAME=pciesvc_upg

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) -r $(REL)
