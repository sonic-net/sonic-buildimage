# docker dpu image for load

DOCKER_DPU_BASE_STEM = docker-dpu-base

DOCKER_DPU_BASE = $(DOCKER_DPU_BASE_STEM).gz

$(DOCKER_DPU_BASE)_URL = https://github.com/pensando/dsc-artifacts/blob/main/docker-dpu-base.gz?raw=true

DOWNLOADED_DOCKER_IMAGES += $(DOCKER_DPU_BASE)

