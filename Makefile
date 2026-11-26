# SONiC make file

NOBULLSEYE ?= 1
NOBOOKWORM ?= 0
NOTRIXIE ?= 0

override Q := @
ifeq ($(QUIET),n)
  override Q := 
endif
override SONIC_OVERRIDE_BUILD_VARS += $(SONIC_BUILD_VARS)
override SONIC_OVERRIDE_BUILD_VARS += Q=$(Q)
export Q SONIC_OVERRIDE_BUILD_VARS

ifeq ($(NOBULLSEYE),0)
BUILD_BULLSEYE=1
endif

ifeq ($(NOBOOKWORM),0)
BUILD_BOOKWORM=1
endif

ifeq ($(NOTRIXIE),0)
BUILD_TRIXIE=1
endif

# Bazel components only support a subset of configurations.
# Fail fast if we're trying to build with Bazel for anything other than trixie.
ifneq ($(filter-out bazel_disabled,$(BAZEL_MIN_READINESS)),)
BAZEL_NON_TRIXIE_BUILDS := $(strip $(BUILD_BULLSEYE) $(BUILD_BOOKWORM))
ifneq ($(BAZEL_NON_TRIXIE_BUILDS),)
$(error BAZEL_MIN_READINESS=$(BAZEL_MIN_READINESS) only supports trixie builds: Bazel dockers require the trixie base. Re-run with trixie only, e.g. NOBULLSEYE=1 NOBOOKWORM=1 NOTRIXIE=0, or set BAZEL_MIN_READINESS=bazel_disabled.)
endif
endif

PLATFORM_PATH := platform/$(if $(PLATFORM),$(PLATFORM),$(CONFIGURED_PLATFORM))
PLATFORM_CHECKOUT := platform/checkout
PLATFORM_CHECKOUT_FILE := $(PLATFORM_CHECKOUT)/$(PLATFORM).ini
PLATFORM_CHECKOUT_CMD := $(shell if [ -f $(PLATFORM_CHECKOUT_FILE) ]; then PLATFORM_REPO=$(PLATFORM_REPO) PLATFORM_REF=$(PLATFORM_REF) PLATFORM_PATH=$(PLATFORM_PATH) j2 $(PLATFORM_CHECKOUT)/template.j2 $(PLATFORM_CHECKOUT_FILE); fi)
MAKE_WITH_RETRY := ./scripts/run_with_retry $(MAKE)
vs_bookworm_cleanup_marker = $(if $(filter 1:0:target/sonic-vs.bin,$(SONIC_VS_IMAGE_TIMINGS):$(NOTRIXIE):$@),echo '[ vs timing ] [ bookworm-cleanup ] [ $(1) ]')

%::
	@echo "+++ --- Making $@ --- +++"
ifeq ($(NOBULLSEYE), 0)
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) BLDENV=bullseye -f Makefile.work bullseye
endif
ifeq ($(NOBOOKWORM), 0)
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) BLDENV=bookworm -f Makefile.work bookworm
endif
ifeq ($(NOTRIXIE), 0)
	$(MAKE_WITH_RETRY) BLDENV=trixie -f Makefile.work $@
endif

	@$(call vs_bookworm_cleanup_marker,start)
	BLDENV=bookworm $(MAKE) -f Makefile.work docker-cleanup
	@$(call vs_bookworm_cleanup_marker,end)

bullseye:
	@echo "+++ Making $@ +++"
ifeq ($(NOBULLSEYE), 0)
	$(MAKE) -f Makefile.work bullseye
endif

trixie:
	@echo "+++ Making $@ +++"
ifeq ($(NOTRIXIE), 0)
	$(MAKE) -f Makefile.work trixie
endif

init reset:
	@echo "+++ Making $@ +++"
	$(MAKE) -f Makefile.work $@

#
# Function to invoke target $@ in Makefile.work with proper BLDENV
#
define make_work
	@echo "+++ Making $@ +++"
	$(if $(BUILD_BULLSEYE),BLDENV=bullseye $(MAKE) -f Makefile.work $@,)
	$(if $(BUILD_BOOKWORM),BLDENV=bookworm $(MAKE) -f Makefile.work $@,)
	$(if $(BUILD_TRIXIE),BLDENV=trixie $(MAKE) -f Makefile.work $@,)
endef

.PHONY: $(PLATFORM_PATH)

$(PLATFORM_PATH):
	@echo "+++ Checking $@ +++"
	$(PLATFORM_CHECKOUT_CMD)

configure : $(PLATFORM_PATH)
	$(call make_work, $@)

clean showtag docker-cleanup clean-docker sonic-slave-build sonic-slave-bash :
	$(call make_work, $@)

# Freeze the versions, see more detail options: scripts/versions_manager.py freeze -h
freeze:
	@scripts/versions_manager.py freeze $(FREEZE_VERSION_OPTIONS)

include tools/bazel/direct_image_goals.mk

# Let the inner build check changed sources and Make/Bazel builder switches even
# when these outputs already exist. Keep the outputs non-phony so they retain
# the implicit forwarding recipe above, in both builder modes.
.PHONY: sonic-swss-forward
sonic-swss-forward:

target/docker-orchagent.gz target/docker-orchagent-dbg.gz \
target/sonic-vs.bin target/sonic-vs.img.gz target/sonic-vs.raw: sonic-swss-forward

# Existing kernel/module outputs still need the inner Make dependency checks.
# Filter only requested goals, so this does not enumerate or build unrelated
# outputs. Keep forwarding in both modes to detect a Bazel/Make builder change.
$(filter target/debs/trixie/linux-%.deb target/debs/trixie/linux-%.deb-install \
    target/debs/trixie/genl-packet-module_%.deb target/debs/trixie/genl-packet-module_%.deb-install \
    target/files/trixie/ixgbe.ko target/%rfs.squashfs,$(MAKECMDGOALS)): sonic-swss-forward
