include tools/bazel/manifests.mk

# Owners register archive names in SONIC_BAZEL_DOCKER_IMAGES or
# SONIC_BAZEL_DBG_DOCKER_IMAGES and declare each archive's explicit Bazel target,
# complete Make prerequisite paths, and source path for its SBOM fragment.
# An optional <archive>_BAZEL_ARGS selects that image's source-build features.
# Owners whose complete option list can be shared with the selected images
# declare it as <archive>_BAZEL_SHARED_ARGS. Other options keep the original
# per-owner build so that shared OCI base layers always have canonical paths.
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

# The VS installer chooses the shipped variants. Its legacy Docker recipes
# can also load Bazel-built base archives (including through other images).
# Stop following legacy metadata as soon as we reach a Bazel producer.
bazel_docker_archives := $(sort $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES))
vs_legacy_image_inputs = $(filter %.gz,$(patsubst %-load,%,$($(1)_AFTER) \
    $(if $(filter %-$(DBG_IMAGE_MARK).gz,$(1)), \
        $(1:-$(DBG_IMAGE_MARK).gz=.gz) $($(1:-$(DBG_IMAGE_MARK).gz=.gz)_AFTER), \
        $($(1)_LOAD_DOCKERS))))
vs_required_bazel_images = $(foreach image,$(filter-out $(2),$(sort $(1))), \
    $(if $(filter $(image),$(bazel_docker_archives)),$(image), \
        $(call vs_required_bazel_images,$(call vs_legacy_image_inputs,$(image)),$(2) $(image))))

vs_installer_goals := $(filter $(addprefix $(TARGET_PATH)/,sonic-vs.bin sonic-vs.img.gz),$(SONIC_BUILD_TARGET) $(MAKECMDGOALS))
SONIC_BAZEL_VS_BATCH_ARCHIVES :=
ifeq ($(CONFIGURED_PLATFORM):$(BLDENV),vs:trixie)
ifneq ($(and $(vs_installer_goals),$(bazel_docker_archives)),)
vs_requested_images := $(foreach installer,$(notdir $(vs_installer_goals)),$($(installer)_DOCKERS)) \
    $(SONIC_PACKAGES_LOCAL) $(notdir $(filter $(TARGET_PATH)/docker-%.gz,$(MAKECMDGOALS)))
vs_required_images := $(sort $(call vs_required_bazel_images,$(vs_requested_images)))
# Preserve the existing VS runtime/debug pair handoff, including when only one
# variant goes into the installer. Singleton producers export just their archive.
vs_required_runtimes := $(filter $(SONIC_BAZEL_DOCKER_IMAGES),$(vs_required_images:%-$(DBG_IMAGE_MARK).gz=%.gz))
vs_paired_debug := $(filter $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(vs_required_runtimes:%.gz=%-$(DBG_IMAGE_MARK).gz))
ifeq ($(SONIC_ENABLE_DEBUG_LAYER):$(sort $(notdir $(vs_installer_goals))),n:sonic-vs.bin)
ifeq ($(INSTALL_DEBUG_TOOLS),y)
$(error SONIC_ENABLE_DEBUG_LAYER=n requires INSTALL_DEBUG_TOOLS to be disabled for sonic-vs.bin)
endif
# A runtime installer needs no companion debug archives. Keep any debug
# archive explicitly required by another goal or by a legacy image dependency.
vs_paired_debug := $(filter $(vs_required_images),$(vs_paired_debug))
endif
vs_pairs := $(vs_paired_debug:%-$(DBG_IMAGE_MARK).gz=%.gz)

# A private flag may change how a dependency is built. Keep that owner's
# original invocation rather than applying its flags to unrelated images.
define vs_classify_image
ifneq ($(2),)
ifneq ($$(strip $$($(1)_BAZEL_ARGS)),$$(strip $$($(2)_BAZEL_ARGS)))
$$(error VS runtime/debug pair $(1) $(2) requires identical Bazel arguments)
endif
endif
ifeq ($$(strip $$($(1)_BAZEL_ARGS)),$$(strip $$($(1)_BAZEL_SHARED_ARGS)))
SONIC_BAZEL_VS_BATCH_ARCHIVES += $(1) $(2)
else ifneq ($(2),)
vs_private_pairs += $(1)
endif
endef
$(foreach image,$(vs_pairs),$(eval $(call vs_classify_image,$(image),$(image:.gz=-$(DBG_IMAGE_MARK).gz))))
$(foreach image,$(filter-out $(vs_pairs) $(vs_paired_debug),$(vs_required_images)),$(eval $(call vs_classify_image,$(image))))
SONIC_BAZEL_VS_BATCH_ARCHIVES := $(sort $(SONIC_BAZEL_VS_BATCH_ARCHIVES))
vs_private_pair_archives := $(vs_private_pairs) $(vs_private_pairs:%.gz=%-$(DBG_IMAGE_MARK).gz)
endif
endif

# One directory per recipe execution keeps parallel and repeated invocations separate.
ifeq ($(SONIC_VS_IMAGE_TIMINGS),1)
define vs_start_image_timing
	timing_directory="$$(python3 -B tools/bazel/ci/time_vs_image.py image-recipe $(1))"
	trap 'if [[ -f "$@.log" && ! -L "$@.log" ]]; then cp -- "$@.log" "$$timing_directory/make.log"; fi' EXIT
endef
vs_time_phase = python3 -B tools/bazel/ci/time_vs_image.py "$$timing_directory" $(1)
endif

# Use one release for every image label, including shared base and debug images.
# Quote the entire option for the shell and for the batch's shlex option parser.
bazel_quote_argument = '$(subst ','"'"',$(1))'
bazel_make_image_options = --//tools/bazel/image/vs/metadata:use_make_metadata=true $(call bazel_quote_argument,--//tools/bazel/oci:image_version=$(SONIC_IMAGE_VERSION))

# An individual archive goal keeps its own Bazel settings and export.
define build_bazel_docker_images
	$(HEADER)
	$(call vs_start_image_timing,$(1))
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
		bazel_options+=($(bazel_make_image_options)
			"--platforms=@sonic_build_infra//platforms:$$bazel_platform"
			"--host_platform=@sonic_build_infra//platforms:$$bazel_platform")
		boot_layouts=()
		$(if $(filter y,$(DOCKER_LOAD_ON_BOOT)),boot_targets="$$(python3 tools/bazel/build_helpers.py boot-layouts --bazel="$${BAZEL:-bazel}" $(foreach image,$(1),--label="$($(image)_BAZEL_TARGET)"))"
		if [[ -n "$$boot_targets" ]]; then mapfile -t boot_layouts <<< "$$boot_targets"; fi)
		$(call vs_time_phase,build) "$${BAZEL:-bazel}" build "$${bazel_options[@]}" $(foreach image,$(1),"$($(image)_BAZEL_TARGET)") "$${boot_layouts[@]}"
		$(foreach image,$(1),
		bazel_output="$$($(call vs_time_phase,cquery-$(image)) "$${BAZEL:-bazel}" cquery "$${bazel_options[@]}" --output=files "$($(image)_BAZEL_TARGET)")"
		layout_option=()
		$(if $(filter y,$(DOCKER_LOAD_ON_BOOT)),if grep -Fxq -- "$($(image)_BAZEL_TARGET).boot-oci" <<< "$$boot_targets"; then
			layout="$$("$${BAZEL:-bazel}" cquery "$${bazel_options[@]}" --output=files "$($(image)_BAZEL_TARGET).boot-oci")"
			[[ -d "$$layout" ]] || { echo "Missing native boot OCI layout for $(image)" >&2; exit 1; }
			layout_option=(--layout-query-output="$$layout")
		fi)
		$(call vs_time_phase,export-$(image)) python3 tools/bazel/build_helpers.py export --query-output="$$bazel_output" --output="$(TARGET_PATH)/$(image)" "$${layout_option[@]}"
		)
	} $(LOG)
	$(foreach image,$(1),$(call sbom_emit_fragment,$(TARGET_PATH)/$(image),DOCKER_IMAGE,$($(image)_PATH),,,,)
	)
	$(FOOTER)
endef

$(addprefix $(TARGET_PATH)/,$(filter-out $(SONIC_BAZEL_VS_BATCH_ARCHIVES) $(vs_private_pair_archives),$(bazel_docker_archives))) : $(TARGET_PATH)/%.gz : .platform bazel-docker-force \
        $$($$*.gz_BAZEL_DEPENDS)
	$(call build_bazel_docker_images,$(@F))

define bazel_vs_private_pair_rule
$(addprefix $(TARGET_PATH)/,$(1) $(2)) &: .platform bazel-docker-force \
        $$$$($(1)_BAZEL_DEPENDS) $$$$($(2)_BAZEL_DEPENDS) \
        $(addprefix $(TARGET_PATH)/.container-build-method/,$(filter $(SONIC_BAZEL_SWITCHABLE_IMAGES),$(1) $(2)))
	$$(call build_bazel_docker_images,$(1) $(2))
endef
$(foreach image,$(vs_private_pairs),$(eval $(call bazel_vs_private_pair_rule,$(image),$(image:.gz=-$(DBG_IMAGE_MARK).gz))))

ifneq ($(SONIC_BAZEL_VS_BATCH_ARCHIVES),)
# All consumers wait for the same Make target, even under -j and when an
# archive already exists. The conductor publishes every variant before return.
$(addprefix $(TARGET_PATH)/,$(SONIC_BAZEL_VS_BATCH_ARCHIVES)): $(TARGET_PATH)/vs-container-images ;

.PHONY: $(TARGET_PATH)/vs-container-images
$(TARGET_PATH)/vs-container-images: .platform \
        $(foreach image,$(SONIC_BAZEL_VS_BATCH_ARCHIVES),$$($(image)_BAZEL_DEPENDS)) \
        $(addprefix $(TARGET_PATH)/.container-build-method/,$(filter $(SONIC_BAZEL_SWITCHABLE_IMAGES),$(SONIC_BAZEL_VS_BATCH_ARCHIVES)))
	$(HEADER)
ifeq ($(SONIC_VS_IMAGE_TIMINGS),1)
	SONIC_VS_CONTAINER_BATCH_TIMING_DIR="$$(python3 -B tools/bazel/ci/time_vs_image.py start-recipe . vs-container-images --bazel build --profiles build --exports)"
	export SONIC_VS_CONTAINER_BATCH_TIMING_DIR
	trap 'if [[ -f "$@.log" && ! -L "$@.log" ]]; then cp -- "$@.log" "$$SONIC_VS_CONTAINER_BATCH_TIMING_DIR/make.log"; fi' EXIT
endif
	python3 -B tools/bazel/build_container_batch.py --target-path="$(TARGET_PATH)" --architecture="$(CONFIGURED_ARCH)" \
		--debug-image-mark="$(DBG_IMAGE_MARK)" $(if $(filter y,$(DOCKER_LOAD_ON_BOOT)),--boot-layouts) \
		--shared-args=$(call bazel_quote_argument,$(bazel_make_image_options)) \
		$(foreach image,$(SONIC_BAZEL_VS_BATCH_ARCHIVES),$(if $($(image)_BAZEL_SHARED_ARGS),--shared-args=$(call bazel_quote_argument,$($(image)_BAZEL_SHARED_ARGS)))) \
		$(foreach image,$(SONIC_BAZEL_VS_BATCH_ARCHIVES),--archive="$(image)" --label="$($(image)_BAZEL_TARGET)" --owner-args=$(call bazel_quote_argument,$($(image)_BAZEL_ARGS))) $(LOG)
	$(foreach image,$(SONIC_BAZEL_VS_BATCH_ARCHIVES),$(call sbom_emit_fragment,$(TARGET_PATH)/$(image),DOCKER_IMAGE,$($(image)_PATH),,,,)
	)
	$(FOOTER)

endif

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES))
SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(sort $(SONIC_BAZEL_OCI_BASES)))
