# Docker-fpm rule-file is simply a wrapper containing routing-stack selection logic.

ifeq ($(SONIC_ROUTING_STACK), frr)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_FPM_FRR)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_FPM_FRR_DBG)
else
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_FPM_GOBGP)
endif
