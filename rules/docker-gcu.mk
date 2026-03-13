# docker image for GCU (Generic Config Updater) container
#
# Design note on sonic-gcu.mk / sonic-gcu.dep:
# -----------------------------------------------
# The sonic-gcu wheel (sonic_gcu-*.whl) is produced by the sonic-utilities
# submodule build (src/sonic-utilities/Makefile) via setup_gcu.py and is
# referenced in slave.mk as SONIC_GCU_WHEEL / SONIC_GCU_PY3.
#
# - rules/sonic-gcu.mk: NOT required here because the wheel is already
#   declared as a build artifact via SONIC_GCU_PY3 in slave.mk and the
#   corresponding rules/sonic-gcu.mk that tracks the submodule source.
#
# - sonic-gcu.dep: NOT included in this Docker .mk because the wheel is
#   installed into a venv with --system-site-packages, so C-extension
#   packages (swsscommon, libyang Python bindings, etc.) from the system are
#   visible. Pure-Python deps (jsonpatch, jsondiff, natsort, etc.) must be
#   declared in the wheel's own install_requires; pip resolves them at image
#   build time from the wheel's embedded metadata.

DOCKER_GCU_STEM = docker-gcu
DOCKER_GCU = $(DOCKER_GCU_STEM).gz
DOCKER_GCU_DBG = $(DOCKER_GCU_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_GCU)_PATH = $(DOCKERS_PATH)/$(DOCKER_GCU_STEM)

# Pull in the sonic-gcu wheel produced by src/sonic-utilities submodule build.
# SONIC_GCU_PY3 is declared in rules/sonic-gcu.mk and added to
# SONIC_PYTHON_WHEELS by that file, which means slave.mk builds it and
# makes it available as a dependency and as a file to install into the image.
# _PYTHON_WHEELS is the attribute read by slave.mk to populate the Jinja2
# variable docker_gcu_whls used in dockers/docker-gcu/Dockerfile.j2.
$(DOCKER_GCU)_PYTHON_WHEELS += $(SONIC_GCU_PY3)

$(DOCKER_GCU)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BOOKWORM)

$(DOCKER_GCU)_VERSION = 1.0.0
$(DOCKER_GCU)_PACKAGE_NAME = gcu

$(DOCKER_GCU)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_BOOKWORM)_DBG_DEPENDS)
$(DOCKER_GCU)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_BOOKWORM)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_GCU)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_GCU)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_GCU_DBG)
SONIC_BOOKWORM_DBG_DOCKERS += $(DOCKER_GCU_DBG)

ifeq ($(INCLUDE_GCU), y)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
SONIC_PACKAGES_LOCAL += $(DOCKER_GCU_DBG)
else
SONIC_PACKAGES_LOCAL += $(DOCKER_GCU)
endif
endif

$(DOCKER_GCU)_CONTAINER_NAME = gcu
$(DOCKER_GCU)_RUN_OPT += -t
$(DOCKER_GCU)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_GCU)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
# Bind-mount /opt/sonic/gcu from the host so the venv and the gcu-standalone
# symlink are visible on the host filesystem. sonic-utilities references
# GCU_STANDALONE_BIN = "/opt/sonic/gcu/current/bin/gcu-standalone" and needs
# to execute it directly from the host. docker-gcu-watchdog mounts the same
# path to monitor the venv.
$(DOCKER_GCU)_RUN_OPT += -v /opt/sonic/gcu:/opt/sonic/gcu
# Redis socket for CONFIG_DB access (config apply-patch connects via Unix socket)
$(DOCKER_GCU)_RUN_OPT += -v /var/run/redis:/var/run/redis:rw
# Share the host network namespace so the container can reach Redis without
# needing to expose individual Unix socket paths via complex volume mappings.
$(DOCKER_GCU)_RUN_OPT += --net=host


