.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(FRR)
DERIVED_TARGET = $(FRR_PYTHONTOOLS) $(FRR_DBG) $(FRR_SNMP) $(FRR_SNMP_DBG)
STG_PATCHED_BRANCH = $(FRR_BRANCH)-$(FRR_TAG)-patched
DPLANE_FPM_SONIC_MODULE = dplane_fpm_sonic/dplane_fpm_sonic.c

# DEBEMAIL required by gpb dch
export DEBEMAIL := sonicproject@googlegroups.com

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	# Prepare source tree.

	pushd ./frr

	# Check if this is the first time checkout.
	if ! stg branch --list 2>/dev/null | grep -q "$(STG_PATCHED_BRANCH)"; then
		# First time: create branch and apply patches.
		git fetch origin $(FRR_BRANCH)
		git checkout -b $(FRR_BRANCH) origin/$(FRR_BRANCH) || git checkout $(FRR_BRANCH)
		stg branch --create $(STG_PATCHED_BRANCH) $(FRR_TAG)
		stg import -s ../patch/series
		gbp dch --ignore-branch --new-version=$(FRR_VERSION)-sonic-$(FRR_SUBVERSION) --dch-opt="--force-bad-version" --commit --git-author
		cp ../$(DPLANE_FPM_SONIC_MODULE) zebra/
	else
		# Reuse existing patched branch.
		# Clean artifacts in frr/debian directory from previous run.
		fakeroot debian/rules clean
	fi

	# Build the package.

ifeq ($(CROSS_BUILD_ENVIRON), y)
	CFLAGS="-I $$CROSS_PERL_CORE_PATH" dpkg-buildpackage -rfakeroot -b -d -us -uc -Ppkg.frr.nortrlib -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -Ppkg.frr.nortrlib -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd
	mv $(DERIVED_TARGET) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
