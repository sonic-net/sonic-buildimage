# docker image for docker-restapi-sidecar

DOCKER_RESTAPI_SIDECAR_STEM = docker-restapi-sidecar
DOCKER_RESTAPI_SIDECAR = $(DOCKER_RESTAPI_SIDECAR_STEM).gz
DOCKER_RESTAPI_SIDECAR_DBG = $(DOCKER_RESTAPI_SIDECAR_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_RESTAPI_SIDECAR)_LOAD_DOCKERS = $(DOCKER_CONFIG_ENGINE_BOOKWORM)

$(DOCKER_RESTAPI_SIDECAR)_PATH = $(DOCKERS_PATH)/$(DOCKER_RESTAPI_SIDECAR_STEM)

$(DOCKER_RESTAPI_SIDECAR)_VERSION = 1.0.0
$(DOCKER_RESTAPI_SIDECAR)_PACKAGE_NAME = restapi-sidecar

SONIC_DOCKER_IMAGES += $(DOCKER_RESTAPI_SIDECAR)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_RESTAPI_SIDECAR)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_RESTAPI_SIDECAR)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_RESTAPI_SIDECAR_DBG)
SONIC_BOOKWORM_DBG_DOCKERS += $(DOCKER_RESTAPI_SIDECAR_DBG)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_RESTAPI_SIDECAR_DBG)


$(DOCKER_RESTAPI_SIDECAR)_DEPENDS += $(LIBSWSSCOMMON)
$(DOCKER_RESTAPI_SIDECAR)_INSTALL_DEBS = $(LIBSWSSCOMMON) \
                                         $(PYTHON3_SWSSCOMMON) \
                                         $(LIBYANG_PY3)

$(DOCKER_RESTAPI_SIDECAR)_CONTAINER_NAME = restapi-sidecar
# NOTE: This container must run in privileged mode with host PID namespace so that
# nsenter can access host namespaces and systemd-related scripts/files can be
# synchronized with the host system. This grants broad host access and should be
# restricted to environments where this level of privilege is acceptable.
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -t --privileged --pid=host
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -v /lib/systemd/system:/lib/systemd/system:rw
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro

$(DOCKER_RESTAPI_SIDECAR)_FILES += $(CONTAINER_CHECKER)
$(DOCKER_RESTAPI_SIDECAR)_FILES += $(RESTAPI_SYSTEMD)
$(DOCKER_RESTAPI_SIDECAR)_FILES += $(K8S_POD_CONTROL)

.PHONY: docker-restapi-sidecar-ut
docker-restapi-sidecar-ut: $(PYTHON_WHEELS_PATH)/sonic_py_common-1.0-py3-none-any.whl-install
	@echo "Running unit tests for systemd_stub.py..."
	@PYTHONPATH=dockers/docker-restapi-sidecar \
		python3 -m pytest -q dockers/docker-restapi-sidecar/cli-plugin-tests

target/docker-restapi-sidecar.gz: docker-restapi-sidecar-ut
