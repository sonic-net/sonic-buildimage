include tools/bazel/manifests.mk

# Owners register archive names in SONIC_BAZEL_DOCKER_IMAGES or
# SONIC_BAZEL_DBG_DOCKER_IMAGES and declare each archive's explicit Bazel target,
# complete Make prerequisite paths, and source path for its SBOM fragment.
# An optional <archive>_BAZEL_ARGS selects that image's source-build features.
$(foreach IMAGE,$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),\
    $(if $(strip $($(IMAGE)_BAZEL_TARGET)),,$(error $(IMAGE)_BAZEL_TARGET is required for a Bazel container))\
    $(if $(strip $($(IMAGE)_PATH)),,$(error $(IMAGE)_PATH is required for a Bazel container SBOM)))

ifneq ($(filter $(SONIC_PACKAGES_LOCAL),$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES)),)
$(error Bazel container archives use the latest tag and cannot be in SONIC_PACKAGES_LOCAL: $(filter $(SONIC_PACKAGES_LOCAL),$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES)))
endif

# Let Bazel check declared inputs on every request. Export preserves timestamps
# when bytes are unchanged and publishes changed archives atomically.
.PHONY: bazel-docker-force
bazel-docker-force:

# Owners register switchable archives even when Bazel is disabled. Track each
# builder independently so returning to Make rebuilds the archive, including
# when package caching is off. A new stamp also invalidates preexisting outputs.
$(addprefix $(TARGET_PATH)/.container-build-method/, $(sort $(SONIC_BAZEL_SWITCHABLE_IMAGES))) : $(TARGET_PATH)/.container-build-method/% : bazel-docker-force
	@mkdir -p "$(@D)"
	@printf '%s\n' '$(if $(filter $*,$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES)),bazel,make)' | cmp -s - "$@" || \
		printf '%s\n' '$(if $(filter $*,$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES)),bazel,make)' > "$@"

$(addprefix $(TARGET_PATH)/, $(sort $(SONIC_BAZEL_SWITCHABLE_IMAGES))) : $(TARGET_PATH)/%.gz : $(TARGET_PATH)/.container-build-method/%.gz

# A legacy cache hit normally keeps an existing output. On a builder change it
# must replace the Bazel archive with the cached Make archive instead. Expand
# this optional cache hook in the archive recipe, after its stamp is updated.
$(foreach IMAGE,$(filter-out $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(sort $(SONIC_BAZEL_SWITCHABLE_IMAGES))),\
    $(eval $(IMAGE)_CACHE_FORCE_RESTORE = $$(filter $(TARGET_PATH)/.container-build-method/$(IMAGE),$$?)))

# Owners register native OCI layouts relative to TARGET_PATH. Multiple consumers
# can share one base; prepare it after its Make archive, including cache hits.
# Delay metadata checks until rule execution so archive paths can use variables
# declared after this include. The helper preserves unchanged layouts atomically.
$(addprefix $(TARGET_PATH)/, $(sort $(SONIC_BAZEL_OCI_BASES))) : $(TARGET_PATH)/%.oci : \
		$$($$*.oci_OCI_ARCHIVE) tools/bazel/oci/prepare_oci_base.py tools/bazel/oci/oci_layout.py bazel-docker-force
	$(if $(strip $($*.oci_OCI_ARCHIVE)),,$(error $*.oci_OCI_ARCHIVE is required for a Bazel OCI base))
	$(if $(strip $($*.oci_OCI_PLATFORM)),,$(error $*.oci_OCI_PLATFORM is required for a Bazel OCI base))
	mkdir -p "$(@D)"
	python3 tools/bazel/oci/prepare_oci_base.py --archive "$($*.oci_OCI_ARCHIVE)" \
		--output "$@" --expected-platform "$($*.oci_OCI_PLATFORM)" $(LOG)

$(addprefix $(TARGET_PATH)/, $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform bazel-docker-force \
		$$($$*.gz_BAZEL_DEPENDS)
	$(HEADER)
	{
		# These image builds are native. Keep both target and execution tools on
		# Make's configured architecture instead of the root RC's AMD64 default.
		case "$(CONFIGURED_ARCH)" in
			amd64) bazel_platform=x86_64_trixie ;;
			arm64) bazel_platform=aarch64_trixie ;;
			*) printf 'Unsupported native Bazel container architecture: %s (supported: amd64, arm64)\n' "$(CONFIGURED_ARCH)" >&2; exit 1 ;;
		esac
		bazel_option_lines="$$(python3 tools/bazel/build_helpers.py options \
			--cache-directory="$${BAZEL_CONTAINER_CACHE_DIR-$${BAZEL_SWSS_CACHE_DIR:-}}" \
			--arguments="$${BAZEL_CONTAINER_ARGS-$${BAZEL_SWSS_ARGS:-}} $($(@F)_BAZEL_ARGS)")"
		bazel_options=()
		if [ -n "$$bazel_option_lines" ]; then mapfile -t bazel_options <<< "$$bazel_option_lines"; fi
		bazel_options+=("--platforms=@sonic_build_infra//platforms:$$bazel_platform"
			"--host_platform=@sonic_build_infra//platforms:$$bazel_platform")
		"$${BAZEL:-bazel}" build "$${bazel_options[@]}" "$($(@F)_BAZEL_TARGET)"
		bazel_output="$$("$${BAZEL:-bazel}" cquery "$${bazel_options[@]}" --output=files "$($(@F)_BAZEL_TARGET)")"
		python3 tools/bazel/build_helpers.py export --query-output="$$bazel_output" --output="$@"
	} $(LOG)
	$(call sbom_emit_fragment,$@,DOCKER_IMAGE,$($(@F)_PATH),,,,)
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES))
SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(sort $(SONIC_BAZEL_OCI_BASES)))
