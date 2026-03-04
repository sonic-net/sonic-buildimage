.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(FRR)
DERIVED_TARGET = $(FRR_PYTHONTOOLS) $(FRR_DBG) $(FRR_SNMP) $(FRR_SNMP_DBG)
SUFFIX = $(shell date +%Y%m%d\.%H%M%S)
STG_BRANCH = stg_temp.$(SUFFIX)
DPLANE_FPM_SONIC_MODULE = dplane_fpm_sonic/dplane_fpm_sonic.c

# DEBEMAIL required by gpb dch
export DEBEMAIL := sonicproject@googlegroups.com

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Build the package
	pushd ./frr
	git fetch origin tag $(FRR_TAG)
	# Create a branch out of tag as stg needs a branch.
	# Prefix branch name with "build-" to avoid conflicts with tag name
	git checkout -B build-$(FRR_TAG) refs/tags/$(FRR_TAG)
ifeq ($(CROSS_BUILD_ENVIRON), y)
	git reset --hard
endif
	stg branch --create $(STG_BRANCH) build-$(FRR_TAG)
	stg import -s ../patch/series
	gbp dch --ignore-branch --new-version=$(FRR_VERSION)-sonic-$(FRR_SUBVERSION) --dch-opt="--force-bad-version" --commit --git-author
	cp ../$(DPLANE_FPM_SONIC_MODULE) zebra/

ifeq ($(CROSS_BUILD_ENVIRON), y)
	CFLAGS="-I $$CROSS_PERL_CORE_PATH" dpkg-buildpackage -rfakeroot -b -d -us -uc -Ppkg.frr.nortrlib -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -Ppkg.frr.nortrlib -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	stg undo || true
	git clean -xfdf
	git checkout build-$(FRR_TAG)
	stg branch --delete --force $(STG_BRANCH)
ifeq ($(CROSS_BUILD_ENVIRON), y)
	git reset --hard
endif
	popd
	mv $(DERIVED_TARGET) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
