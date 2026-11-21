# Re-enter the slave build for a directly requested archive, even when a file
# from the other producer already exists. The slave checks the producer stamp.
SONIC_DIRECT_BAZEL_IMAGES := docker-database docker-fpm-frr docker-gbsyncd-vs docker-lldp docker-mux docker-platform-monitor docker-router-advertiser docker-sonic-otel docker-teamd

sonic_direct_image_goals := $(foreach image,$(SONIC_DIRECT_BAZEL_IMAGES),target/$(image).gz target/$(image)-dbg.gz)
sonic_direct_image_goals += target/docker-dash-engine.gz
ifneq ($(filter $(sonic_direct_image_goals),$(MAKECMDGOALS)),)
.PHONY: sonic-direct-image-forward
$(filter $(sonic_direct_image_goals),$(MAKECMDGOALS)): sonic-direct-image-forward
endif
