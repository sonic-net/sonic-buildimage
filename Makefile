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

PLATFORM_PATH := platform/$(if $(PLATFORM),$(PLATFORM),$(CONFIGURED_PLATFORM))
PLATFORM_CHECKOUT := platform/checkout
PLATFORM_CHECKOUT_FILE := $(PLATFORM_CHECKOUT)/$(PLATFORM).ini
PLATFORM_CHECKOUT_CMD := $(shell if [ -f $(PLATFORM_CHECKOUT_FILE) ]; then PLATFORM_REPO=$(PLATFORM_REPO) PLATFORM_REF=$(PLATFORM_REF) PLATFORM_PATH=$(PLATFORM_PATH) j2 $(PLATFORM_CHECKOUT)/template.j2 $(PLATFORM_CHECKOUT_FILE); fi)
MAKE_WITH_RETRY := ./scripts/run_with_retry $(MAKE)

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

	BLDENV=bookworm $(MAKE) -f Makefile.work docker-cleanup

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
