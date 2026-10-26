# Docker base image (based on Debian Trixie)

DOCKER_BASE_TRIXIE = docker-base-trixie.gz
$(DOCKER_BASE_TRIXIE)_PATH = $(DOCKERS_PATH)/docker-base-trixie

$(DOCKER_BASE_TRIXIE)_DEPENDS += $(SOCAT)
$(DOCKER_BASE_TRIXIE)_DEPENDS += $(IPROUTE2)

GDB = gdb
GDBSERVER = gdbserver
VIM = vim
OPENSSH = openssh-client
SSHPASS = sshpass
STRACE = strace

ifeq ($(INCLUDE_FIPS), y)
$(DOCKER_BASE_TRIXIE)_DEPENDS += $(FIPS_OPENSSL_LIBSSL) $(FIPS_OPENSSL_LIBSSL_DEV) $(FIPS_OPENSSL) $(SYMCRYPT_OPENSSL) \
    $(FIPS_KRB5) $(FIPS_KRB5_SUPPORT0) $(FIPS_KRB5_3) $(FIPS_KRB5_LIBGSSAPI)
endif

$(DOCKER_BASE_TRIXIE)_DBG_IMAGE_PACKAGES += $(GDB) $(GDBSERVER) $(VIM) $(SSHPASS) $(STRACE)

ifeq ($(INCLUDE_FIPS), y)
$(DOCKER_BASE_TRIXIE)_DBG_DEPENDS += $(FIPS_OPENSSH_CLIENT)
else
$(DOCKER_BASE_TRIXIE)_DBG_IMAGE_PACKAGES += $(OPENSSH)
endif

SONIC_DOCKER_IMAGES += $(DOCKER_BASE_TRIXIE)
SONIC_TRIXIE_DOCKERS += $(DOCKER_BASE_TRIXIE)

# Source-backed OCI assembly uses the same shared Make selector and archive
# bridge as the service images. Keep legacy package metadata above for callers
# that still combine it into their own Dockerfiles.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_MANIFESTS += docker-base-trixie
docker-base-trixie_MANIFEST_IMAGE = $(DOCKER_BASE_TRIXIE)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_BASE_TRIXIE)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_BASE_TRIXIE)
$(DOCKER_BASE_TRIXIE)_BAZEL_TARGET = //dockers/docker-base-trixie:docker-base-trixie.gz
$(DOCKER_BASE_TRIXIE)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json
endif
endif
endif
