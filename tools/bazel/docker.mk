include tools/bazel/manifests.mk

# Owners register archive names in SONIC_BAZEL_DOCKER_IMAGES or
# SONIC_BAZEL_DBG_DOCKER_IMAGES and declare each archive's explicit Bazel target,
# complete Make prerequisite paths, and source path for its SBOM fragment.
# An optional <archive>_BAZEL_ARGS selects that image's source-build features.
$(foreach IMAGE,$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),\
    $(if $(strip $($(IMAGE)_BAZEL_TARGET)),,$(error $(IMAGE)_BAZEL_TARGET is required for a Bazel container))\
    $(if $(strip $($(IMAGE)_PATH)),,$(error $(IMAGE)_PATH is required for a Bazel container SBOM)))

# Local packages also accept these latest-tagged archives: the installer's
# sonic-package-manager path adds SONIC_IMAGE_VERSION after loading them.

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

# Build each selected VS runtime/debug pair together while the installer builder
# still owns its warm Bazel state. This is a production dependency only: the
# installer continues to select its payload through <installer>_DOCKERS.
# Standalone archive requests and unsupported owner profiles keep their rules.
SONIC_BAZEL_VS_PAIRED_IMAGES :=
ifeq ($(CONFIGURED_PLATFORM):$(BLDENV),vs:trixie)
ifneq ($(filter $(TARGET_PATH)/sonic-vs.bin $(TARGET_PATH)/sonic-vs.img.gz,$(SONIC_BUILD_TARGET) $(MAKECMDGOALS)),)
SONIC_BAZEL_VS_PAIRED_IMAGES := $(foreach image,$(sort $(SONIC_BAZEL_DOCKER_IMAGES)),\
    $(if $(filter $(image:.gz=-$(DBG_IMAGE_MARK).gz),$(SONIC_BAZEL_DBG_DOCKER_IMAGES)),$(image)))
endif
endif
SONIC_BAZEL_VS_PAIR_OUTPUTS := $(foreach image,$(SONIC_BAZEL_VS_PAIRED_IMAGES),$(image) $(image:.gz=-$(DBG_IMAGE_MARK).gz))

# One directory per recipe execution keeps parallel and repeated invocations separate.
ifeq ($(SONIC_VS_IMAGE_TIMINGS),1)
define vs_start_image_timing
	timing_directory="$$(python3 -B tools/bazel/ci/time_vs_image.py image-recipe $(1))"
	trap 'if [[ -f "$@.log" && ! -L "$@.log" ]]; then cp -- "$@.log" "$$timing_directory/make.log"; fi' EXIT
endef
vs_time_phase = python3 -B tools/bazel/ci/time_vs_image.py "$$timing_directory" $(1)
endif

# One invocation per owner keeps its runtime/debug configuration identical.
# Query and export both standalone archives; unchanged exports retain timestamps.
define build_bazel_docker_images
	$(HEADER)
	$(call vs_start_image_timing,$(1))
	$(if $(word 2,$(1)),printf '%s\n' '[ bazel image pair ] [ $@ ] [ $(TARGET_PATH)/$(firstword $(1)) ]')
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
			--arguments="$${BAZEL_CONTAINER_ARGS-$${BAZEL_SWSS_ARGS:-}} $($(firstword $(1))_BAZEL_ARGS)")"
		bazel_options=()
		if [ -n "$$bazel_option_lines" ]; then mapfile -t bazel_options <<< "$$bazel_option_lines"; fi
		bazel_options+=("--//tools/bazel/image/vs/metadata:use_make_metadata=true"
			"--platforms=@sonic_build_infra//platforms:$$bazel_platform"
			"--host_platform=@sonic_build_infra//platforms:$$bazel_platform")
		$(call vs_time_phase,build) "$${BAZEL:-bazel}" build "$${bazel_options[@]}" $(foreach image,$(1),"$($(image)_BAZEL_TARGET)")
		$(foreach image,$(1),
		bazel_output="$$($(call vs_time_phase,cquery-$(image)) "$${BAZEL:-bazel}" cquery "$${bazel_options[@]}" --output=files "$($(image)_BAZEL_TARGET)")"
		$(call vs_time_phase,export-$(image)) python3 tools/bazel/build_helpers.py export --query-output="$$bazel_output" --output="$(TARGET_PATH)/$(image)"
		)
	} $(LOG)
	$(foreach image,$(1),$(call sbom_emit_fragment,$(TARGET_PATH)/$(image),DOCKER_IMAGE,$($(image)_PATH),,,,)
	)
	$(FOOTER)
endef

$(addprefix $(TARGET_PATH)/, $(filter-out $(SONIC_BAZEL_VS_PAIR_OUTPUTS),$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES))) : $(TARGET_PATH)/%.gz : .platform bazel-docker-force \
        $$($$*.gz_BAZEL_DEPENDS)
	$(call build_bazel_docker_images,$(@F))

# Grouped targets run once even when both variants are requested in parallel,
# and rebuild the pair if either archive is missing. Each variant's manifest,
# declared inputs and producer-transition stamp must exist before Bazel starts.
define bazel_vs_pair_rule
ifneq ($$(strip $$($(1)_BAZEL_ARGS)),$$(strip $$($(2)_BAZEL_ARGS)))
$$(error VS runtime/debug pair $(1) $(2) requires identical Bazel arguments)
endif
$(addprefix $(TARGET_PATH)/,$(1) $(2)) &: .platform bazel-docker-force \
        $$$$($(1)_BAZEL_DEPENDS) $$$$($(2)_BAZEL_DEPENDS) \
        $(addprefix $(TARGET_PATH)/.container-build-method/,$(filter $(SONIC_BAZEL_SWITCHABLE_IMAGES),$(1) $(2)))
	$$(call build_bazel_docker_images,$(1) $(2))
endef
$(foreach image,$(SONIC_BAZEL_VS_PAIRED_IMAGES),$(eval $(call bazel_vs_pair_rule,$(image),$(image:.gz=-$(DBG_IMAGE_MARK).gz))))

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES))
SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(sort $(SONIC_BAZEL_OCI_BASES)))
