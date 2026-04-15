# libyang1-dev and libyang3-dev are not designed to coexist.  The way
# SONiC builds, however, it depends on libyang3-dev being installed first
# and anything that needs it being built then, and only then, should
# libyang1-dev be installed, and the remaining packages can then be
# built.
#
# Any package that relies directly on $(LIBYANG3_DEV) needs to be listed
# as a dependency for libyang3-dev-done.
#
# libyang1 won't even be built until everything needing libyang3-dev is
# done.

LIBYANG3-DEV-DONE = libyang3-dev-done
$(LIBYANG3-DEV-DONE)_DEPENDS = $(LIBYANG3_PY3)

# Not compiling FRR for Trixie
ifeq ($(BLDENV),bookworm)
$(LIBYANG3-DEV-DONE)_DEPENDS += $(FRR)
endif

SONIC_PHONIES += $(LIBYANG3-DEV-DONE)
