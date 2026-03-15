.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(FRR)
DERIVED_TARGET = $(FRR_PYTHONTOOLS) $(FRR_DBG) $(FRR_SNMP) $(FRR_SNMP_DBG)
STG_PATCHED_BRANCH = $(FRR_BRANCH)-$(FRR_TAG)-patched
PATCHSET_SHA_FILE = ../.sonic-frr-patch-$(subst /,_,$(STG_PATCHED_BRANCH)).sha1
DPLANE_FPM_SONIC_MODULE = dplane_fpm_sonic/dplane_fpm_sonic.c

# DEBEMAIL required by gpb dch
export DEBEMAIL := sonicproject@googlegroups.com

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	# Prepare source tree.

	pushd ./frr

	# Rebuild the patched branch only when it is missing or patch inputs changed.
	current_patch_hash=$$(cat ../patch/series ../patch/*.patch 2>/dev/null | sha1sum | awk '{print $$1}')
	recorded_patch_hash="$$(cat $(PATCHSET_SHA_FILE) 2>/dev/null || true)"
	patched_branch_exists=no
	if git rev-parse --verify --quiet "refs/heads/$(STG_PATCHED_BRANCH)" >/dev/null; then
		patched_branch_exists=yes
	fi
	if [[ "$$patched_branch_exists" == "no" || "$$current_patch_hash" != "$$recorded_patch_hash" ]]; then
		git fetch origin $(FRR_BRANCH)
		git checkout -B $(FRR_BRANCH) origin/$(FRR_BRANCH)
		if [[ "$$patched_branch_exists" == "yes" ]]; then
			stg branch --delete --force $(STG_PATCHED_BRANCH)
		fi
		stg branch --create $(STG_PATCHED_BRANCH) $(FRR_TAG)
		stg import -s ../patch/series
		gbp dch --ignore-branch --new-version=$(FRR_VERSION)-sonic-$(FRR_SUBVERSION) --dch-opt="--force-bad-version" --commit --git-author
		echo "$$current_patch_hash" > $(PATCHSET_SHA_FILE)
	else
		# Reuse existing patched branch.
		git checkout $(STG_PATCHED_BRANCH)
	fi

	# Copy the sonic dplane module source file.
	cp ../$(DPLANE_FPM_SONIC_MODULE) zebra/

	# Clean artifacts in frr/debian directory from previous run.
	fakeroot debian/rules clean

	# Build the package.

ifeq ($(CROSS_BUILD_ENVIRON), y)
	CFLAGS="-I $$CROSS_PERL_CORE_PATH" dpkg-buildpackage -rfakeroot -b -d -us -uc -Ppkg.frr.nortrlib -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -Ppkg.frr.nortrlib -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd
	mv $(DERIVED_TARGET) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
