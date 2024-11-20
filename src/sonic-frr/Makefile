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
	git fetch --all

	oldbranch=$$(git describe --always)
	if ! git checkout $(FRR_BRANCH)-sonic; then
		git checkout -b $(FRR_BRANCH)-sonic origin/$(FRR_BRANCH)
	fi
	git clean -xdf
	git reset --hard origin/$(FRR_BRANCH)
	git branch --list 'stg_temp.*' | sed -e '/^[*] /d' | xargs --no-run-if-empty git branch -D

	stg branch --create $(STG_BRANCH)-sonic $(FRR_TAG)
	stg import -s ../patch/series

	cp ../$(DPLANE_FPM_SONIC_MODULE) zebra/
	git add -A zebra
	git commit -m 'Add dplan netlink module'

	gbp dch --ignore-branch --new-version=$(FRR_VERSION)-sonic-$(FRR_SUBVERSION) --dch-opt="--force-bad-version" --commit --git-author

ifeq ($(CROSS_BUILD_ENVIRON), y)
	CFLAGS="-I $$CROSS_PERL_CORE_PATH" dpkg-buildpackage -rfakeroot -b -d -us -uc -Ppkg.frr.nortrlib -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -Ppkg.frr.nortrlib -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	git checkout "$$oldbranch"
	popd
	mv $(DERIVED_TARGET) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
