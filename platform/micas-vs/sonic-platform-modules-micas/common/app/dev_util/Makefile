top_srcdir:=$(shell pwd)
#include $(top_srcdir)/Rules.mk
DIR=$(shell pwd)
BUILD_OUTPUT=$(DIR)/tmp
SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, $(BUILD_OUTPUT)/%.o, $(SRCS))
DEPS=$(patsubst %.o, %.d, $(OBJS))
CFLAGS+=-Wall -W -g -I$(DIR)/include
LDFLAGS=
PROGRAM=dfd_debug

.PHONY: all

all:$(OBJS)
	$(CC)  $(OBJS) $(LDFLAGS) -o $(BUILD_OUTPUT)/$(PROGRAM)
	@if [ ! -d ${common_out_put_dir} ]; then mkdir -p ${common_out_put_dir} ;fi
	cp -p $(BUILD_OUTPUT)/$(PROGRAM) $(common_out_put_dir)

$(OBJS):$(SRCS)
	@if [ ! -d ${BUILD_OUTPUT} ]; then mkdir -p ${BUILD_OUTPUT} ;fi
	$(CC)  -c $(CFLAGS) $(INCLUDE) $(*F).c -o $@

.PHONY: install
install:
	@mkdir -p $(common_out_put_dir)
	cp -p $(BUILD_OUTPUT)/$(PROGRAM) $(common_out_put_dir)

rebuild: clean all
clean:
	@rm -rf $(BUILD_OUTPUT)/*
