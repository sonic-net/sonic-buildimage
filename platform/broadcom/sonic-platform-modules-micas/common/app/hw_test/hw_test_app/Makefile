top_srcdir:=$(shell pwd)
#include $(top_srcdir)/Rules.mk
DIR=$(shell pwd)
BUILD_OUTPUT=$(DIR)/tmp
SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, $(BUILD_OUTPUT)/%.o, $(SRCS))
DEPS=$(patsubst %.o, %.d, $(OBJS))
CFLAGS+=-Wall -W -g -I$(DIR)/include
LDFLAGS=
PROGRAM=hw_test.bin
TEST_PROGRAM=ddr_bounds_test

.PHONY: all check

all:$(OBJS)
	$(CC)  $(OBJS) $(LDFLAGS) -o $(BUILD_OUTPUT)/$(PROGRAM)
	@if [ ! -d ${common_out_put_dir} ]; then mkdir -p ${common_out_put_dir} ;fi
	cp -p $(BUILD_OUTPUT)/$(PROGRAM) $(common_out_put_dir)	

check: $(BUILD_OUTPUT)/$(TEST_PROGRAM)
	$<

$(BUILD_OUTPUT)/$(TEST_PROGRAM): tests/ddr_bounds_test.c ft_ddr_test.c include/hw_dram/ft_ddr_test.h include/hw_dram/fac_common.h
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(CFLAGS) $(INCLUDE) $< $(LDFLAGS) -o $@

$(OBJS):$(SRCS)
	@if [ ! -d ${BUILD_OUTPUT} ]; then mkdir -p ${BUILD_OUTPUT} ;fi
	$(CC)  -c $(CFLAGS) $(INCLUDE) $(*F).c -o $@
	
.PHONY: install
install:
	@mkdir -p $(install_sbin_dir)
	cp -p $(BUILD_OUTPUT)/$(PROGRAM) $(common_out_put_dir)	

	
rebuild: clean all
clean:
	@rm -rf $(BUILD_OUTPUT)/*