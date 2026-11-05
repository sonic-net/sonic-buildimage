SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

MAIN_TARGET = bash_$(BASH_VERSION_FULL)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = $(BASH_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Quilt store applied patches info in .pc folder, if this folder not clean, quilt can't apply patches correctly.
	rm -rf .pc
	rm -rf bash-$(BASH_VERSION_MAJOR)

	dget -u https://deb.debian.org/debian/pool/main/b/bash/bash_$(BASH_VERSION_FULL).dsc

	# Apply optional downstream-only patches listed in patches/series.local
	# (absent upstream -> no-op). Each non-empty, non-comment line naming a patch
	# is appended to the effective quilt series before "quilt push -a". The
	# pristine patches/series is saved and restored via a trap so the tracked
	# file is never left mutated for later builds.
	if [ -f patches/series.local ]; then \
		cp patches/series patches/.series.orig; \
		trap 'mv -f patches/.series.orig patches/series' EXIT; \
		while IFS= read -r patch || [ -n "$$patch" ]; do \
			patch="$$(printf '%s' "$$patch" | sed 's/^[[:space:]]*//;s/[[:space:]]*$$//')"; \
			[ -z "$$patch" ] && continue; \
			case "$$patch" in '#'*) continue ;; esac; \
			if ! grep -qxF -- "$$patch" patches/series; then \
				printf '%s\n' "$$patch" >> patches/series; \
			fi; \
		done < patches/series.local; \
	fi

	# Apply plugin suport patches
	quilt push -a

	pushd bash-$(BASH_VERSION_MAJOR)

	# build package
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	# copy UT code.
	cp -a ../Files/. ./

	# generate config.h which need by UT
	./configure

	# generate 'pathnames.h' by make bash again, which is needed by UT.
	make

	# run UT after config.h ready.
	make -C unittest && make test -C unittest
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
