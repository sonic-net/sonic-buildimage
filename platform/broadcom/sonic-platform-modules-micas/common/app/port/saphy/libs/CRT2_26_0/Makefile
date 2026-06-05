export
HAL = $(foreach d, $(wildcard $(CURDIR)/credo/chips/*), $(shell basename $(d)))


HAL_CHECK = $(HAL:%=%_check)

TARGETS = sdk $(HAL)

# Directory calculation
ROOT_DIR = $(CURDIR)
SRC_ROOT = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))credo
OBJ_ROOT := $(CURDIR)/build/obj
IMG_ROOT := $(CURDIR)/build

# Build option
ifeq ($(V),1)
Q =
else
Q = @
endif

# Toolchain
ifeq "$(origin CC)" "default"
	CC = gcc
endif
ifeq "$(origin AR)" "default"
	AR = ar
endif
ifeq "$(origin LD)" "undefined"
	LD = ld
endif
ifeq "$(origin OBJCOPY)" "undefined"
	OBJCOPY = $(CROSS_COMPILE)objcopy
endif

CFLAGS = -fPIC -ansi -std=c99 -pedantic -pedantic-errors -Wall -Werror -Wno-unused-result
INCDIR = $(SRC_ROOT)/include/ $(SRC_ROOT)/include/libs $(SRC_ROOT)/include/ip ./include
CPPFLAGS = $(foreach d, $(INCDIR), -I $(d)) -D_POSIX_C_SOURCE=200809L -DCREDO_DEBUG_API

# OS Specific flags
ifdef WIN_BUILD
ARGSHAL = DLLFLAGS='-L$(CURDIR)/build -lcredo'
SHLIBEXT = dll
else ifdef MAC_BUILD
ARGSHAL = DLLFLAGS='-L$(CURDIR)/build -lcredo'
SHLIBEXT = dylib
else
ARGSHAL =
SHLIBEXT = so
LDFLAGS = -lrt
endif

ifdef DEBUG
CPPFLAGS += -g
CFLAGS += -g
else
CPPFLAGS += -O2
CFLAGS += -O2
endif

ifdef HARDWARE_FLOAT
CPPFLAGS += -mfloat-abi=hard
CFLAGS += -mfloat-abi=hard
endif


all: sdk $(HAL) crcli crlua
	@echo "SDK Build Done!"

hal_check: $(HAL_CHECK)

# Check that the hal library has no undefined symbols
$(HAL_CHECK): %_check: %
	@echo Checking $*
	$(Q)main_temp=$$(mktemp --tmpdir=$(IMG_ROOT) --suffix=.c); \
	echo "int main() {return 0;}" > $$main_temp; \
	if $(CC) $(CPPFLAGS) $(CFLAGS) $$main_temp $(IMG_ROOT)/lib$*.$(SHLIBEXT) $(IMG_ROOT)/libcredo.$(SHLIBEXT) -lpthread -lm $(LDFLAGS) -o $(IMG_ROOT)/$*.out; then \
		$(RM) $$main_temp $(IMG_ROOT)/$*.out; \
	else \
		$(RM) $$main_temp; \
		exit 1; \
	fi

crlua: sdk
	$(MAKE) -C $(SRC_ROOT)/crlua all
	cp $(SRC_ROOT)/crlua/libcrlua.* $(IMG_ROOT)

sdk:
	$(MAKE) -C $(SRC_ROOT)/sdk

$(HAL): %: sdk crcli crlua
	$(MAKE) -C $(SRC_ROOT)/chips/$* $(ARGSHAL)

crcli:
ifndef WIN_BUILD
	$(MAKE) -C $(SRC_ROOT)/crcli all
else
	@echo "No crcli for windows builds"
endif


lddk:
ifndef WIN_BUILD
	$(MAKE) -C $(SRC_ROOT)/crlua/lddk all
else
	@echo "No lddk for windows builds"
endif


clean:
	$(MAKE) -C $(SRC_ROOT)/sdk clean
	$(MAKE) -C $(SRC_ROOT)/crlua clean
	$(MAKE) -C $(SRC_ROOT)/crcli clean
	for i in $(HAL); do $(MAKE) -C $(SRC_ROOT)/chips/$$i clean; done
	rm -rf build/obj
	-rm build/crcli
	
cleandep:
	find $(OBJ_ROOT) -name "*.dep" -delete

listobj:
	@$(MAKE) -s -C $(SRC_ROOT)/sdk listobj
	@for i in $(HAL); do $(MAKE) -s -C $(SRC_ROOT)/chips/$$i listobj; done
