# docker base image

DOCKER_BASE = docker-base.gz
$(DOCKER_BASE)_PATH = $(DOCKERS_PATH)/docker-base
$(DOCKER_BASE)_DEPENDS += $(BASH)
$(DOCKER_BASE)_DEPENDS += $(SOCAT)

ifeq ($(INSTALL_DEBUG_TOOLS),y)
GDB = gdb
GDBSERVER = gdbserver
VIM = vim
OPENSSH = openssh-client
SSHPASS = sshpass
STRACE = strace
$(DOCKER_BASE)_DBG_PACKAGES += $(GDB) $(GDBSERVER) $(VIM) $(OPENSSH) $(SSHPASS) $(STRACE)
endif

SONIC_DOCKER_IMAGES += $(DOCKER_BASE)
