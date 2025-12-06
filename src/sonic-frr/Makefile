.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(FRR)
DERIVED_TARGET = $(FRR_PYTHONTOOLS) $(FRR_DBG) $(FRR_SNMP) $(FRR_SNMP_DBG)
SUFFIX = $(shell date +%Y%m%d\.%H%M%S)
STG_BRANCH = stg_temp.$(SUFFIX)
STG_DEVELOPER_BRANCH = $(FRR_BRANCH)-$(FRR_TAG)-patched
DPLANE_FPM_SONIC_MODULE = dplane_fpm_sonic/dplane_fpm_sonic.c

# DEBEMAIL required by gpb dch
export DEBEMAIL := sonicproject@googlegroups.com

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	# Prepare source tree.

	pushd ./frr
ifeq ($(DEVELOPER_MODE), y)
	set -x
	# Developer mode: apply patches only once and preserve source tree.
	if ! stg branch --list 2>/dev/null | grep -q "$(STG_DEVELOPER_BRANCH)"; then
		echo "Developer mode: creating branch $(STG_DEVELOPER_BRANCH) from $(FRR_TAG)"
		git checkout -b $(FRR_BRANCH) origin/$(FRR_BRANCH) || git checkout $(FRR_BRANCH)
		stg branch --create $(STG_DEVELOPER_BRANCH) $(FRR_TAG)
		stg import -s ../patch/series
		gbp dch --ignore-branch --new-version=$(FRR_VERSION)-sonic-$(FRR_SUBVERSION) --dch-opt="--force-bad-version" --commit --git-author
		cp ../$(DPLANE_FPM_SONIC_MODULE) zebra/
		echo "Developer mode: branch $(STG_DEVELOPER_BRANCH) initialized with SONiC patches"
	else
		echo "Developer mode: reusing branch $(STG_DEVELOPER_BRANCH)"
		# Clean artifacts in frr/debian directory from previous runs.
		fakeroot debian/rules clean
	fi
else
	# Standard mode: apply patches and clean up after build.
	git checkout -b $(FRR_BRANCH) origin/$(FRR_BRANCH) || git checkout $(FRR_BRANCH)
ifeq ($(CROSS_BUILD_ENVIRON), y)
	git reset --hard
endif
	stg branch --create $(STG_BRANCH) $(FRR_TAG)
	stg import -s ../patch/series
	gbp dch --ignore-branch --new-version=$(FRR_VERSION)-sonic-$(FRR_SUBVERSION) --dch-opt="--force-bad-version" --commit --git-author
	cp ../$(DPLANE_FPM_SONIC_MODULE) zebra/
endif # $(DEVELOPER_MODE)

	# Build the package.

ifeq ($(CROSS_BUILD_ENVIRON), y)
	CFLAGS="-I $$CROSS_PERL_CORE_PATH" dpkg-buildpackage -rfakeroot -b -d -us -uc -Ppkg.frr.nortrlib -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -Ppkg.frr.nortrlib -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	# Cleanup source tree after the build is done.

ifeq ($(DEVELOPER_MODE), y)
	echo "Developer mode: keeping patched tree $(STG_DEVELOPER_BRANCH) for follow-up work"
else
	# Standard mode: clean up and revert to pristine state.
	stg undo || true
	git clean -xfdf
	git checkout $(FRR_BRANCH)
	stg branch --delete --force $(STG_BRANCH)
	git rev-parse --short HEAD | xargs git checkout
ifeq ($(CROSS_BUILD_ENVIRON), y)
	git reset --hard
endif
	git branch -f master origin/$(FRR_BRANCH)
	git checkout master
	git branch -D $(FRR_BRANCH)
endif # $(DEVELOPER_MODE)
	popd
	mv $(DERIVED_TARGET) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
