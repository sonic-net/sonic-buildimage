###############################################################################
# Bazel-based build of docker-orchagent (opt-in via BAZEL_ORCHAGENT=y)
#
# This file is included from slave.mk *after* the generic $(TARGET_PATH)/%.gz
# pattern rule is defined (slave.mk:~1146), so any explicit rule we declare
# here wins the GNU make "later recipe overrides earlier" tie-break.
#
# When BAZEL_ORCHAGENT=y, target/docker-orchagent.gz is produced by Bazel
# (//dockers/docker-orchagent:load) instead of the standard `docker build`
# of Dockerfile.j2. All other images keep the traditional build path.
#
# Architecture note (slave dockerd vs host dockerd):
#   The sonic-slave-bookworm container runs its OWN isolated dockerd on
#   /var/run/docker.sock. Images built/loaded inside the slave are not
#   visible on the host, and vice versa. Bazel-driven image builds, however,
#   live in the host dockerd (so the persistent bazel disk cache and the
#   sonic-bazel-builder:bookworm image stay on the host).
#
#   To bridge the two, Makefile.work also bind-mounts the host docker.sock
#   to /var/run/host-docker.sock inside the slave when BAZEL_ORCHAGENT=y.
#   The recipe below does:
#     1. on the host dockerd:  bazel run :load   -> docker-orchagent:latest
#     2. on the host dockerd:  docker save       -> /tmp/orchagent.tar
#     3. on the slave dockerd: docker load       <- /tmp/orchagent.tar
#     4. on the slave dockerd: docker tag + docker-image-save -> .gz
#
# Override variables:
#   BAZEL_ORCHAGENT=y                    -- enable Bazel path
#   BAZEL_ORCHAGENT_BUILDER_IMAGE=<img>  -- builder image tag (on host)
#   BAZEL_ORCHAGENT_WORKSPACE=<path>     -- absolute path to this repo
#                                          (must match on host and container)
#   BAZEL_ORCHAGENT_TARGET=<label>       -- Bazel target to run
###############################################################################
BAZEL_ORCHAGENT ?= n
BAZEL_ORCHAGENT_BUILDER_IMAGE ?= sonic-bazel-builder:bookworm
# HOST_SONIC_DIR is exported by Makefile.work into the slave container. When
# running outside the slave (e.g. directly on host), fall back to $(abspath .).
BAZEL_ORCHAGENT_WORKSPACE ?= $(if $(HOST_SONIC_DIR),$(HOST_SONIC_DIR),$(abspath .))
# The parent of the workspace is bind-mounted into the builder container so
# that Bazel `local_path_override` entries pointing at sibling repos (e.g.
# ../p4runtime/proto, ../p4-constraints, ../p4c, ../gutil) are resolvable.
BAZEL_ORCHAGENT_WORKSPACE_PARENT := $(patsubst %/,%,$(dir $(BAZEL_ORCHAGENT_WORKSPACE)))
# HOST_HOME is also exported by Makefile.work; it's the host-side path to the
# user's home directory, where bazelisk and bazel keep their disk caches.
# Inside the slave this is needed because the slave's own $HOME (/var/lunyue)
# does not exist on the host and would cause `docker run -v $$HOME:$$HOME`
# to bind-mount an empty directory.
BAZEL_ORCHAGENT_HOST_HOME ?= $(if $(HOST_HOME),$(HOST_HOME),$(HOME))
BAZEL_ORCHAGENT_TARGET ?= //dockers/docker-orchagent:load
BAZEL_ORCHAGENT_IMAGE ?= docker-orchagent:latest
# Path inside the slave container to reach the host dockerd. Makefile.work
# bind-mounts /var/run/docker.sock to this path when BAZEL_ORCHAGENT=y.
BAZEL_ORCHAGENT_HOST_DOCKER_SOCK ?= /var/run/host-docker.sock

ifeq ($(BAZEL_ORCHAGENT),y)

# When Bazel produces the docker image, the traditional build-time deps for
# this image are unnecessary -- Bazel resolves everything itself from the
# @bookworm registry and from its own //src/sonic-swss tree.
#
# IMPORTANT: GNU make merges prereqs from explicit and pattern rules (only
# the recipe is overridden). The generic %.gz pattern in slave.mk has 8
# prereq slots driven by $$*.gz_* variables -- every one of them must be
# cleared here, or make will queue the original deb/wheel/layer graph in
# addition to invoking our Bazel recipe. See slave.mk:1146.
#
# Variables that affect runtime (CONTAINER_NAME, RUN_OPT) are intentionally
# NOT cleared.
$(DOCKER_ORCHAGENT)_DEPENDS :=
$(DOCKER_ORCHAGENT)_DBG_DEPENDS :=
$(DOCKER_ORCHAGENT)_AFTER :=
$(DOCKER_ORCHAGENT)_FILES :=
$(DOCKER_ORCHAGENT)_PYTHON_DEBS :=
$(DOCKER_ORCHAGENT)_PYTHON_WHEELS :=
$(DOCKER_ORCHAGENT)_LOAD_DOCKERS :=
$(DOCKER_ORCHAGENT)_INSTALL_PYTHON_WHEELS :=
$(DOCKER_ORCHAGENT)_INSTALL_DEBS :=
$(DOCKER_ORCHAGENT)_BASE_IMAGE_FILES :=
$(DOCKER_ORCHAGENT_DBG)_DEPENDS :=
$(DOCKER_ORCHAGENT_DBG)_PYTHON_WHEELS :=

# Explicit target rule. Defined *after* slave.mk's %.gz pattern rule, so the
# "later recipe wins" rule in GNU make makes this one fire for the orchagent
# image. The pattern rule's prereqs would still apply, but the variables that
# drove them are cleared above, leaving only `.platform docker-start`.
$(TARGET_PATH)/$(DOCKER_ORCHAGENT): .platform docker-start
	$(HEADER)
	@echo "==> [BAZEL] Building $(DOCKER_ORCHAGENT) via $(BAZEL_ORCHAGENT_BUILDER_IMAGE)" $(LOG)
	@# All `docker` invocations in this recipe go to the *host* daemon via
	@# the bind-mounted socket, NOT to the slave's isolated dockerd, except
	@# the final `docker load` + `docker-image-save` which must run on the
	@# slave's dockerd (default DOCKER_HOST is unix:///var/run/docker.sock).
	@if ! DOCKER_HOST=unix://$(BAZEL_ORCHAGENT_HOST_DOCKER_SOCK) docker image inspect $(BAZEL_ORCHAGENT_BUILDER_IMAGE) >/dev/null 2>&1; then \
	  echo "ERROR: builder image '$(BAZEL_ORCHAGENT_BUILDER_IMAGE)' not found on host dockerd." >&2; \
	  echo "       Build it first: docker build -t $(BAZEL_ORCHAGENT_BUILDER_IMAGE) tools/bazel-builder/" >&2; \
	  exit 1; \
	fi
	@# Step 1: ask the host dockerd to run the builder container which runs
	@# `bazel run :load`. The image lands in the host dockerd.
	@# If the host's $HOME/.cache resolves to a path outside $HOME (e.g. a
	@# symlink to /data/lunyue/home-overflow/.cache), bind-mount that real
	@# path too so bazelisk's mkdir on ~/.cache/bazelisk succeeds. The extra
	@# mount is conveyed via HOST_HOME_CACHE_REAL set in Makefile.work.
	DOCKER_HOST=unix://$(BAZEL_ORCHAGENT_HOST_DOCKER_SOCK) docker run --rm \
	  -v $(BAZEL_ORCHAGENT_WORKSPACE_PARENT):$(BAZEL_ORCHAGENT_WORKSPACE_PARENT) \
	  -v $(BAZEL_ORCHAGENT_HOST_HOME):$(BAZEL_ORCHAGENT_HOST_HOME) \
	  $${HOST_HOME_CACHE_REAL:+-v "$${HOST_HOME_CACHE_REAL}:$${HOST_HOME_CACHE_REAL}"} \
	  -v /var/run/docker.sock:/var/run/docker.sock \
	  --group-add $$(stat -c '%g' $(BAZEL_ORCHAGENT_HOST_DOCKER_SOCK)) \
	  -u $$(id -u):$$(id -g) \
	  -e HOME=$(BAZEL_ORCHAGENT_HOST_HOME) \
	  -w $(BAZEL_ORCHAGENT_WORKSPACE) \
	  $(BAZEL_ORCHAGENT_BUILDER_IMAGE) \
	  bazel run $(BAZEL_ORCHAGENT_TARGET) $(LOG)
	@# Step 2: save the image from the host dockerd to a temp tar.
	@mkdir -p $(TARGET_PATH) $(LOG)
	DOCKER_HOST=unix://$(BAZEL_ORCHAGENT_HOST_DOCKER_SOCK) docker save $(BAZEL_ORCHAGENT_IMAGE) -o $(TARGET_PATH)/$(DOCKER_ORCHAGENT_STEM)-bazel.tar $(LOG)
	@# Step 3: load the tar into the slave's isolated dockerd so the
	@# subsequent retag + docker-image-save (which use the default
	@# DOCKER_HOST = unix:///var/run/docker.sock) can find it.
	docker load -i $(TARGET_PATH)/$(DOCKER_ORCHAGENT_STEM)-bazel.tar $(LOG)
	@rm -f $(TARGET_PATH)/$(DOCKER_ORCHAGENT_STEM)-bazel.tar $(LOG)
	@# Step 4: retag and let the standard SONiC pipeline finalize the .gz.
	docker tag $(BAZEL_ORCHAGENT_IMAGE) $(DOCKER_ORCHAGENT_STEM)-$(DOCKER_USERNAME):$(DOCKER_USERTAG) $(LOG)
	$(call docker-image-save,$(DOCKER_ORCHAGENT_STEM),$@)
	$(FOOTER)

endif
