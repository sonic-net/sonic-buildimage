# SONiC make file

NOJESSIE ?= 1
NOSTRETCH ?= 1
NOBUSTER ?= 0
NOBULLSEYE ?= 0
NOBOOKWORM ?= 0

override Q := @
ifeq ($(QUIET),n)
  override Q := 
endif
override SONIC_OVERRIDE_BUILD_VARS += $(SONIC_BUILD_VARS)
override SONIC_OVERRIDE_BUILD_VARS += Q=$(Q)
export Q SONIC_OVERRIDE_BUILD_VARS

PLATFORM_PATH := platform/$(if $(PLATFORM),$(PLATFORM),$(CONFIGURED_PLATFORM))
PLATFORM_CHECKOUT := platform/checkout
PLATFORM_CHECKOUT_FILE := $(PLATFORM_CHECKOUT)/$(PLATFORM).ini
PLATFORM_CHECKOUT_CMD := $(shell if [ -f $(PLATFORM_CHECKOUT_FILE) ]; then PLATFORM_PATH=$(PLATFORM_PATH) j2 $(PLATFORM_CHECKOUT)/template.j2 $(PLATFORM_CHECKOUT_FILE); fi)
MAKE_WITH_RETRY := ./scripts/run_with_retry $(MAKE)

target/%::
	@echo "+++ --- Making $@ --- +++"
ifeq ($(NOJESSIE), 0)
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) -f Makefile.work jessie
endif
ifeq ($(NOSTRETCH), 0)
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) BLDENV=stretch -f Makefile.work stretch
endif
ifeq ($(NOBUSTER), 0)
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) BLDENV=buster -f Makefile.work buster
endif
ifeq ($(NOBULLSEYE), 0)
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) BLDENV=bullseye -f Makefile.work bullseye
endif
ifeq ($(NOBOOKWORM), 0)
	$(MAKE_WITH_RETRY) BLDENV=bookworm -f Makefile.work $@
endif
	BLDENV=bookworm $(MAKE) -f Makefile.work docker-cleanup

jessie:
ifeq ($(NOJESSIE), 0)
	@echo "+++ Making $@ +++"
	$(MAKE_WITH_RETRY) BLDENV=$@ -f Makefile.work $@ 2>&1 | tee target/$@.log
endif

stretch:
ifeq ($(NOSTRETCH), 0)
	@echo "+++ Making $@ +++"
	$(MAKE_WITH_RETRY) BLDENV=$@ -f Makefile.work $@ 2>&1 | tee target/$@.log
endif

buster:
ifeq ($(NOBUSTER), 0)
	@echo "+++ Making $@ +++"
	$(MAKE_WITH_RETRY) BLDENV=$@ -f Makefile.work $@ 2>&1 | tee target/$@.log
endif


bullseye:
ifeq ($(NOBULLSEYE), 0)
	@echo "+++ Making $@ +++"
	$(MAKE_WITH_RETRY) BLDENV=$@ -f Makefile.work $@ 2>&1 | tee target/$@.log
endif

bookworm:
ifeq ($(NOBOOKWORM), 0)
	@echo "+++ Making $@ +++"
	$(MAKE_WITH_RETRY) BLDENV=$@ -f Makefile.work $@ 2>&1 | tee target/$@.log
endif

init:
	@echo "+++ Making $@ +++"
	$(MAKE) -f Makefile.work $@

#
# Function to invoke target $@ in Makefile.work with proper BLDENV
#
define make_work
	@echo "+++ Making $@ +++"
	$(if $(filter 0, $(NOJESSIE)),$(MAKE) -f Makefile.work $@,)
	$(if $(filter 0, $(NOSTETCH)),BLDENV=stretch $(MAKE) -f Makefile.work $@,)
	$(if $(filter 0, $(NOBUSTER)),BLDENV=buster $(MAKE) -f Makefile.work $@,)
	$(if $(filter 0, $(NOBULLSEYE)),BLDENV=bullseye $(MAKE) -f Makefile.work $@,)
	$(if $(filter 0, $(NOBOOKWORM)),BLDENV=bookworm $(MAKE) -f Makefile.work $@,)
endef

.PHONY: $(PLATFORM_PATH)

dists: jessie stretch buster bullseye bookworm
	@echo +++ Making finished: $^ +++

$(PLATFORM_PATH):
	@echo "+++ Cheking $@ +++"
	$(PLATFORM_CHECKOUT_CMD)

configure : $(PLATFORM_PATH)
	$(MAKE) -f Makefile.work $@

clean reset:
	$(MAKE) -f Makefile.work $@

showtag docker-cleanup sonic-slave-build sonic-slave-bash :
	$(call make_work, $@)

# Freeze the versions, see more detail options: scripts/versions_manager.py freeze -h
freeze:
	@scripts/versions_manager.py freeze $(FREEZE_VERSION_OPTIONS)
