# docker dpu image for load

DOCKER_DPU_BASE_STEM = docker-dpu-base

DOCKER_DPU_BASE = $(DOCKER_DPU_BASE_STEM).gz

$(DOCKER_DPU_BASE)_PATH = $(PLATFORM_PATH)/pensando-sonic-artifacts

COPY_DOCKER_IMAGES += $(DOCKER_DPU_BASE)

