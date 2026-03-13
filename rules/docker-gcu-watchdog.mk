# docker image for GCU watchdog

DOCKER_GCU_WATCHDOG_STEM = docker-gcu-watchdog
DOCKER_GCU_WATCHDOG = $(DOCKER_GCU_WATCHDOG_STEM).gz
DOCKER_GCU_WATCHDOG_DBG = $(DOCKER_GCU_WATCHDOG_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_GCU_WATCHDOG)_LOAD_DOCKERS = $(DOCKER_CONFIG_ENGINE_BOOKWORM)
$(DOCKER_GCU_WATCHDOG)_PATH = $(DOCKERS_PATH)/$(DOCKER_GCU_WATCHDOG_STEM)

$(DOCKER_GCU_WATCHDOG)_VERSION = 1.0.0

SONIC_DOCKER_IMAGES += $(DOCKER_GCU_WATCHDOG)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_GCU_WATCHDOG)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_GCU_WATCHDOG_DBG)
SONIC_BOOKWORM_DBG_DOCKERS += $(DOCKER_GCU_WATCHDOG_DBG)

ifeq ($(INCLUDE_GCU_WATCHDOG), y)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
SONIC_PACKAGES_LOCAL += $(DOCKER_GCU_WATCHDOG_DBG)
else
SONIC_PACKAGES_LOCAL += $(DOCKER_GCU_WATCHDOG)
endif
endif

# Container name uses hyphen (kebab-case) to match SONiC naming convention;
# the Rust binary uses underscore (snake_case) internally.
$(DOCKER_GCU_WATCHDOG)_CONTAINER_NAME = gcu-watchdog
$(DOCKER_GCU_WATCHDOG)_PACKAGE_NAME   = gcu-watchdog
$(DOCKER_GCU_WATCHDOG)_RUN_OPT += -t --pid=host
$(DOCKER_GCU_WATCHDOG)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
# Mount /opt/sonic/gcu from the host read-only — same bind-mount used by
# docker-gcu so the watchdog can see the venv at /opt/sonic/gcu/venv.
$(DOCKER_GCU_WATCHDOG)_RUN_OPT += -v /opt/sonic/gcu:/opt/sonic/gcu:ro
