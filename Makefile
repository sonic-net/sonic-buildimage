# SONiC make file

NOJESSIE ?= 1
NOSTRETCH ?= 1
NOBUSTER ?= 1
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

ifeq ($(NOJESSIE),0)
BUILD_JESSIE=1
endif

ifeq ($(NOSTRETCH),0)
BUILD_STRETCH=1
endif

ifeq ($(NOBUSTER),0)
BUILD_BUSTER=1
endif

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


# When BUILD_WITH_BAZEL_WHEN_AVAILABLE is true, dockers listed in
# BAZEL_COMPATIBLE_DOCKERS are built with Bazel instead of the legacy Make/slave
# flow. RUN_BAZEL_IN_SLAVE_CONTAINER selects whether Bazel runs inside the
# sonic-slave container (default) or directly on the host.
BUILD_WITH_BAZEL_WHEN_AVAILABLE ?= false
RUN_BAZEL_IN_SLAVE_CONTAINER ?= true

ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE), true)
TARGET_PATH := target/
BAZEL_COMPATIBLE_DOCKERS := \
	docker-sysmgr

# These explicit targets take precedence over the catch-all `%::` rule below, so
# requesting one builds with Bazel and bypasses the normal slave.mk flow.
$(addprefix $(TARGET_PATH), $(addsuffix .gz, $(BAZEL_COMPATIBLE_DOCKERS))): $(TARGET_PATH)%.gz:
	@echo "BL: Bazel build $* (RUN_BAZEL_IN_SLAVE_CONTAINER=$(RUN_BAZEL_IN_SLAVE_CONTAINER))"
ifeq ($(RUN_BAZEL_IN_SLAVE_CONTAINER), true)
	$(MAKE) -f Makefile.work BLDENV=bookworm sonic-slave-run \
		SONIC_RUN_CMDS='cd /sonic && bazel build //dockers/$*:$*.tar && gzip -c bazel-bin/dockers/$*/load/tarball.tar > $@'
else
	bazel build //dockers/$*:$*.tar
	gzip -c "$$(bazel cquery --output=files //dockers/$*:$*.tar)" > $@
endif

endif

%::
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
	$(MAKE_WITH_RETRY) EXTRA_DOCKER_TARGETS=$(notdir $@) BLDENV=bookworm -f Makefile.work bookworm
endif
ifeq ($(NOTRIXIE), 0)
	$(MAKE_WITH_RETRY) BLDENV=trixie -f Makefile.work $@
endif

	BLDENV=bookworm $(MAKE) -f Makefile.work docker-cleanup

jessie:
	@echo "+++ Making $@ +++"
ifeq ($(NOJESSIE), 0)
	$(MAKE) -f Makefile.work jessie
endif

stretch:
	@echo "+++ Making $@ +++"
ifeq ($(NOSTRETCH), 0)
	$(MAKE) -f Makefile.work stretch
endif

buster:
	@echo "+++ Making $@ +++"
ifeq ($(NOBUSTER), 0)
	$(MAKE) -f Makefile.work buster
endif

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
	$(if $(BUILD_JESSIE),$(MAKE) -f Makefile.work $@,)
	$(if $(BUILD_STRETCH),BLDENV=stretch $(MAKE) -f Makefile.work $@,)
	$(if $(BUILD_BUSTER),BLDENV=buster $(MAKE) -f Makefile.work $@,)
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
