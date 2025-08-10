pes_parent_dir:=$(shell pwd)/$(lastword $(MAKEFILE_LIST))
pes_parent_dir:=$(shell dirname $(pes_parent_dir))

SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "build") print $$9}')
INC = -I./inc

COMMON_OUT_PUT := $(shell pwd)/build
common_out_put_dir := $(COMMON_OUT_PUT)/app
common_module_dir := $(COMMON_OUT_PUT)/module/
export common_out_put_dir common_module_dir

all : CHECK $(SUBDIRS)
CHECK :
	@echo $(pes_parent_dir)

$(SUBDIRS):ECHO
	#@echo $@
	make -C $@

ECHO:
	@echo $(SUBDIRS)

.PHONY : clean
clean :
	-rm  -rf $(COMMON_OUT_PUT)
