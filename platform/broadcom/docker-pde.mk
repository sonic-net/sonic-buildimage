# Docker image for SONiC Platform Development Environment (PDE)

ifeq ($(INCLUDE_PDE), y)
DOCKER_PDE_STEM = docker-pde
DOCKER_PDE = $(DOCKER_PDE_STEM).gz
DOCKER_PDE_DBG = $(DOCKER_PDE_STEM)-$(DBG_IMAGE_MARK).gz
$(DOCKER_PDE)_PATH = $(DOCKERS_PATH)/$(DOCKER_PDE_STEM)
$(DOCKER_PDE)_DEPENDS += $(PYTHON_NETIFACES)
$(DOCKER_PDE)_DEPENDS += $(SONIC_PLATFORM_PDE) $(BRCM_XGS_SAI)

$(DOCKER_PDE_RDEPENDS += $(PYTHON_NETIFACES)
$(DOCKER_PDE)_PYTHON_DEBS += $(SONIC_UTILS)
$(DOCKER_PDE)_PYTHON_WHEELS += $(SONIC_PLATFORM_COMMON_PY3)

ifeq ($(PDDF_SUPPORT), y)
$(DOCKER_PDE)_PYTHON_WHEELS += $(PDDF_PLATFORM_API_BASE_PY3)
endif
$(DOCKER_PDE)_PYTHON_WHEELS += $(SONIC_DAEMON_BASE_PY3)
$(DOCKER_PDE)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_BOOKWORM)_DBG_DEPENDS)
$(DOCKER_PDE)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_BOOKWORM)_DBG_IMAGE_PACKAGES)
$(DOCKER_PDE)_LOAD_DOCKERS = $(DOCKER_CONFIG_ENGINE_BOOKWORM)

SONIC_DOCKER_IMAGES += $(DOCKER_PDE)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_PDE)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_PDE)
SONIC_BOOKWORM_DBG_DOCKERS += $(DOCKER_PDE_DBG)
SONIC_DOCKER_DBG_IMAGES += $(DOCKER_PDE_DBG)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_PDE_DBG)

$(DOCKER_PDE)_VERSION = 1.0.0
$(DOCKER_PDE)_PACKAGE_NAME = pde

$(DOCKER_PDE)_CONTAINER_NAME = pde
$(DOCKER_PDE)_RUN_OPT += --privileged -t
$(DOCKER_PDE)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_PDE)_RUN_OPT += -v /host/machine.conf:/host/machine.conf:ro
$(DOCKER_PDE)_RUN_OPT += -v /usr/lib/python2.7/dist-packages:/usr/share/sonic/classes:ro

$(DOCKER_PDE)_RUN_OPT += -v /usr/local/lib/python3.7/dist-packages/utilities_common:/usr/local/lib/python3.7/dist-packages/utilities_common:ro
$(DOCKER_PDE)_RUN_OPT += -v /var/log/syslog:/var/log/syslog:ro
$(DOCKER_PDE)_RUN_OPT += -v /var/log/ramfs:/var/log/ramfs:ro
$(DOCKER_PDE)_RUN_OPT += -v /lib/modules:/lib/modules:ro
$(DOCKER_PDE)_RUN_OPT += -v /boot:/boot:ro
$(DOCKER_PDE)_RUN_OPT += -v /var/log/ramfs:/var/log/ramfs:ro
$(DOCKER_PDE)_RUN_OPT += -v /usr/share/sonic/device/x86_64-broadcom_common:/usr/share/sonic/device/x86_64-broadcom_common:ro
$(DOCKER_PDE)_RUN_OPT += -v /usr/share/sonic/device/pddf:/usr/share/sonic/device/pddf:ro
$(DOCKER_PDE)_RUN_OPT += -v /usr/share/sonic/device/x86_64-broadcom_common:/usr/share/sonic/device/x86_64-broadcom_common:ro
$(DOCKER_PDE)_BASE_IMAGE_FILES += pde-test:/usr/local/bin/pde-test
$(DOCKER_PDE)_BASE_IMAGE_FILES += pde-bench:/usr/local/bin/pde-bench
$(DOCKER_PDE)_BASE_IMAGE_FILES += pde-stress:/usr/local/bin/pde-stress
$(DOCKER_PDE)_BASE_IMAGE_FILES += pde-bench-knet:/usr/local/bin/pde-bench-knet

endif
