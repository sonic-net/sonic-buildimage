pes_parent_dir:=$(shell pwd)/$(lastword $(MAKEFILE_LIST))
pes_parent_dir:=$(shell dirname $(pes_parent_dir))

COMMON_DIR = common

INC = -I./inc

COMMON_OUT_PUT := $(shell pwd)/build
common_out_put_dir := $(COMMON_OUT_PUT)/app
common_module_dir := $(COMMON_OUT_PUT)/module/
export common_out_put_dir common_module_dir

all :
	make -C $(COMMON_DIR)

.PHONY : clean
clean :
	-rm  -rf $(COMMON_OUT_PUT)
