pes_parent_dir:=$(shell pwd)/$(lastword $(MAKEFILE_LIST))
pes_parent_dir:=$(shell dirname $(pes_parent_dir))

SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "build") print $$9}')
INC = -I./inc

all : CHECK $(SUBDIRS)
CHECK :
	@echo $(pes_parent_dir)

# dev_sysfs's modpost step reads dev_cfg/Module.symvers (see
# dev_sysfs/Makefile's KBUILD_EXTRA_SYMBOLS). Without an explicit
# ordering dependency, make -j (SONIC_BUILD_JOBS) can launch dev_sysfs
# in parallel with dev_cfg and the modpost step fails with:
#   dev_cfg/Module.symvers: No such file or directory
dev_sysfs: dev_cfg

$(SUBDIRS):ECHO
	#@echo $@
	make -C $@

ECHO:
	@echo $(SUBDIRS)

.PHONY : clean
clean :
	-rm  -rf $(SYSFS_OUT_PUT)
