#!/bin/bash

# APT 3.0.3 can retain an unbounded delayed-retry warning queue (Debian
# https://bugs.debian.org/1117797). Only the native Docker CI job opts out of
# internal retries; its existing whole-build retry wrapper remains responsible
# for recovery. Never change the APT policy shipped in the resulting image.
case "${SONIC_DOCKER_APT_RETRIES:-}" in
    ""|0) ;;
    *) echo "SONIC_DOCKER_APT_RETRIES must be unset, empty, or 0" >&2; exit 1 ;;
esac
case "${SONIC_DOCKER_APT_PROXY:-}" in
    ""|0|1) ;;
    *) echo "SONIC_DOCKER_APT_PROXY must be unset, empty, 0, or 1" >&2; exit 1 ;;
esac

configure_apt_mounts() {
    if [ "${SONIC_DOCKER_APT_PROXY:-}" = 1 ] || grep -qx '# SONIC_DOCKER_APT_PROXY=1' "$1"; then
        python3 "$(dirname "$0")/docker_apt_secret_mounts.py" "$1" "${SONIC_DOCKER_APT_PROXY:-0}"
    fi
}

if grep -q "^# SKIP_HOOK" "$2"; then
    if [ -n "${SONIC_DOCKER_APT_RETRIES:-}" ]; then
        echo "SONIC_DOCKER_APT_RETRIES requires buildinfo finalization" >&2
        exit 1
    fi
    if [ "${SONIC_DOCKER_APT_PROXY:-}" = 1 ] || [ -f "${4:-$2}" ]; then
        if [ "${SONIC_DOCKER_APT_PROXY:-}" = 1 ] && [ "$2" != "${4:-$2}" ]; then
            cp -- "$2" "$4" || exit 1
        fi
        configure_apt_mounts "${4:-$2}" || exit 1
    fi
    exit 0
fi

[[ ! -z "${DBGOPT}" && $0 =~ ${DBGOPT} ]] && set -x

BUILDINFO_BASE=/usr/local/share/buildinfo

SCRIPT_SRC_PATH=src/sonic-build-hooks
if [ -e ${SCRIPT_SRC_PATH} ]; then
	. ${SCRIPT_SRC_PATH}/scripts/utils.sh
else
	. ${BUILDINFO_BASE}/scripts/utils.sh
fi

IMAGENAME=$1
DOCKERFILE=$2
ARCH=$3
DOCKERFILE_TARGET=$4
DISTRO=$5


[ -z "$BUILD_SLAVE" ] && BUILD_SLAVE=n
[ -z "$DOCKERFILE_TARGET" ] && DOCKERFILE_TARGET=$DOCKERFILE
DOCKERFILE_PATH=$(dirname "$DOCKERFILE_TARGET")
BUILDINFO_PATH="${DOCKERFILE_PATH}/buildinfo"
BUILDINFO_VERSION_PATH="${BUILDINFO_PATH}/versions"
DOCKER_PATH=$(dirname $DOCKERFILE)

# Reusing generated output under a different policy would silently ignore the
# caller's requested behavior. Regenerate from the original Dockerfile instead.
if [ -f "$DOCKERFILE_TARGET" ] && grep -q "Auto-Generated for buildinfo" "$DOCKERFILE_TARGET"; then
    if grep -q '^# SONIC_DOCKER_APT_RETRIES=0$' "$DOCKERFILE_TARGET"; then
        [ "${SONIC_DOCKER_APT_RETRIES:-}" = 0 ] || {
            echo "Regenerate the Dockerfile before changing SONIC_DOCKER_APT_RETRIES" >&2; exit 1;
        }
    elif [ -n "${SONIC_DOCKER_APT_RETRIES:-}" ]; then
        echo "Regenerate the Dockerfile before changing SONIC_DOCKER_APT_RETRIES" >&2
        exit 1
    fi
fi

[ -d $BUILDINFO_PATH ] && rm -rf $BUILDINFO_PATH
mkdir -p $BUILDINFO_VERSION_PATH

# Get the debian distribution from the docker base image
if [ -z "$DISTRO" ]; then
    DOCKER_BASE_IMAGE=$(grep "^ARG BASE=" $DOCKERFILE | head -n 1 | awk '{print $2}' | cut -d'=' -f 2)
    if [ -z "$DOCKER_BASE_IMAGE" ]; then
        DOCKER_BASE_IMAGE=$(grep "^FROM" $DOCKERFILE | head -n 1 | awk '{print $2}')
    fi
    DISTRO=$(docker run --rm --entrypoint "" $DOCKER_BASE_IMAGE cat /etc/os-release | grep VERSION_CODENAME | cut -d= -f2)
    if [ -z "$DISTRO" ]; then
        DISTRO=$(docker run --rm --entrypoint "" $DOCKER_BASE_IMAGE cat /etc/apt/sources.list | grep deb.debian.org | awk '{print $3}')
        [ -z "$DISTRO" ] && DISTRO=jessie
    fi
fi

if [[ "$IMAGENAME" == sonic-slave-* ]] || [[ "$IMAGENAME" == docker-base-* ]] || [[ "$IMAGENAME" == docker-ptf ]]; then
    scripts/build_mirror_config.sh ${DOCKERFILE_PATH} $ARCH $DISTRO
	mkdir -p "${DOCKERFILE_PATH}/files/apt/apt.conf.d"
	cp -f files/apt/apt.conf.d/* "${DOCKERFILE_PATH}/files/apt/apt.conf.d/"
fi

# add script for reproducible build. using sha256 instead of tag for docker base image.
scripts/docker_version_control.sh $@

DOCKERFILE_APT_PRE_SCRIPT=
DOCKERFILE_APT_POST_SCRIPT=
if [ "${SONIC_DOCKER_APT_RETRIES:-}" = 0 ]; then
    APT_RETRY_CONTENT='// Temporary SONiC native Docker CI override; Debian bug #1117797.
Acquire::Retries "0";'
    APT_RETRY_SHA=$(printf '%s\n' "$APT_RETRY_CONTENT" | sha256sum | cut -d' ' -f1)
    DOCKERFILE_APT_PRE_SCRIPT='# SONIC_DOCKER_APT_RETRIES=0
RUN set -eu; conf=/etc/apt/apt.conf.d/zzzz-sonic-ci-acquire-retries; \
    if [ -e "$conf" ] || [ -L "$conf" ]; then echo "Refusing to overwrite $conf" >&2; exit 1; fi; \
    (umask 022; set -C; printf "%s\n" "// Temporary SONiC native Docker CI override; Debian bug #1117797." '\''Acquire::Retries "0";'\'' > "$conf"); \
    [ "$(apt-config shell retries Acquire::Retries)" = "retries='\''0'\''" ] || { echo "Temporary APT override did not set effective Acquire::Retries to 0" >&2; exit 1; }
'
    DOCKERFILE_APT_POST_SCRIPT='RUN set -eu; conf=/etc/apt/apt.conf.d/zzzz-sonic-ci-acquire-retries; \
    [ -f "$conf" ] && [ ! -L "$conf" ]; \
    [ "$(sha256sum "$conf" | cut -d" " -f1)" = "'${APT_RETRY_SHA}'" ] || { echo "Temporary APT override changed unexpectedly" >&2; exit 1; }; \
    rm -- "$conf"
'
fi

DOCKERFILE_PRE_SCRIPT='# Auto-Generated for buildinfo
'${DOCKERFILE_APT_PRE_SCRIPT}'ARG SONIC_VERSION_CACHE
ARG SONIC_VERSION_CONTROL_COMPONENTS
ARG ENABLE_SBOM=n
COPY ["buildinfo", "/usr/local/share/buildinfo"]
COPY vcache/ /sonic/target/vcache/'${IMAGENAME}'
RUN dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb
ENV IMAGENAME='${IMAGENAME}'
ENV DISTRO='${DISTRO}'
ENV ENABLE_SBOM=$ENABLE_SBOM
RUN pre_run_buildinfo '${IMAGENAME}'
'

DOCKERFILE_POST_SCRIPT='
RUN post_run_buildinfo '${IMAGENAME}'
RUN post_run_cleanup '${IMAGENAME}'
'
DOCKERFILE_POST_SCRIPT+="${DOCKERFILE_APT_POST_SCRIPT}"

# Add the auto-generate code if it is not added in the target Dockerfile
if [ ! -f $DOCKERFILE_TARGET ] || ! grep -q "Auto-Generated for buildinfo" $DOCKERFILE_TARGET; then
    COPY_BASE_LINE_NUMBER=$(grep -n -m 1 'FROM \$BASE$' $DOCKERFILE | cut -d: -f1)
    if [ -z "$COPY_BASE_LINE_NUMBER" ]; then
        COPY_BASE_LINE_NUMBER=$(grep -n -m 1 'FROM scratch$' $DOCKERFILE | cut -d: -f1)
    fi
    TEMP_FILE=$(mktemp)
    # Each build stage starts from its own image. In particular, the watchdog
    # Rust builder does not pass its hooks to the independent runtime stage.
    # Initialize before each stage's first RUN, except the final flattening
    # stage, which copies the already finalized filesystem from the base stage.
    # ENVIRON preserves literal shell escapes; awk -v would interpret printf's
    # backslash-n and split the generated Docker RUN instruction.
    SONIC_BUILDINFO_PRE_SCRIPT="${DOCKERFILE_PRE_SCRIPT}" \
    SONIC_BUILDINFO_POST_SCRIPT="${DOCKERFILE_POST_SCRIPT}" \
    SONIC_BUILDINFO_APT_CLEANUP="${DOCKERFILE_APT_POST_SCRIPT}" \
    awk -v copybaselinenumber="$COPY_BASE_LINE_NUMBER" '
        BEGIN {
            prescript = ENVIRON["SONIC_BUILDINFO_PRE_SCRIPT"]
            postscript = ENVIRON["SONIC_BUILDINFO_POST_SCRIPT"]
            aptcleanup = ENVIRON["SONIC_BUILDINFO_APT_CLEANUP"]
        }
        NR == copybaselinenumber {
            if (!initialized) print prescript
            print postscript
        }
        /^FROM[[:space:]]/ {
            # A later stage can COPY from any earlier stage. Remove only our
            # temporary policy before that stage becomes a copyable image.
            if (aptcleanup && stage_active && NR != copybaselinenumber) {
                if (!initialized) print prescript
                print aptcleanup
            }
            initialized = 0
            stage_active = (!copybaselinenumber || NR < copybaselinenumber)
        }
        /^RUN[[:space:]]/ && !initialized && (!copybaselinenumber || NR < copybaselinenumber) {
            print prescript
            initialized = 1
        }
        { print }
        END {
            if (!copybaselinenumber) {
                if (!initialized) print prescript
                print postscript
            }
        }
    ' "$DOCKERFILE" > "$TEMP_FILE"

    configure_apt_mounts "$TEMP_FILE" || { rm -f "$TEMP_FILE"; exit 1; }
    cat "$TEMP_FILE" > "$DOCKERFILE_TARGET" || { rm -f "$TEMP_FILE"; exit 1; }
    rm -f "$TEMP_FILE"
else
    configure_apt_mounts "$DOCKERFILE_TARGET" || exit 1
fi

# Copy the build info config
mkdir -p ${BUILDINFO_PATH}
cp -rf src/sonic-build-hooks/buildinfo/* $BUILDINFO_PATH

# Stage the shared cargo-auditable wrapper inside the buildinfo
# directory so each sonic-slave-* Dockerfile can COPY it from a
# single source of truth (files/build/cargo-wrapper) — only needs
# to apply to slave-base images where Rust is installed.
if [[ "$IMAGENAME" == sonic-slave-* ]] && [ -f files/build/cargo-wrapper ]; then
    cp files/build/cargo-wrapper $BUILDINFO_PATH/cargo-wrapper
    chmod 0755 $BUILDINFO_PATH/cargo-wrapper
fi

# Generate the version lock files
scripts/versions_manager.py generate -t "$BUILDINFO_VERSION_PATH" -n "$IMAGENAME" -d "$DISTRO" -a "$ARCH"

touch $BUILDINFO_VERSION_PATH/versions-deb

# Create the cache directories
LOCAL_CACHE_DIR=target/vcache/${IMAGENAME}
mkdir -p ${LOCAL_CACHE_DIR} ${DOCKER_PATH}/vcache/
chmod -f 777 ${LOCAL_CACHE_DIR} ${DOCKER_PATH}/vcache/

if [[ "$SKIP_BUILD_HOOK" == y || ${ENABLE_VERSION_CONTROL_DOCKER} != y ]]; then
	exit 0
fi

# Version cache
DOCKER_IMAGE_NAME=${IMAGENAME}
IMAGE_DBGS_NAME=${DOCKER_IMAGE_NAME//-/_}_image_dbgs

if [[ ${DOCKER_IMAGE_NAME} == sonic-slave-* ]]; then
	GLOBAL_CACHE_DIR=${SONIC_VERSION_CACHE_SOURCE}/${DOCKER_IMAGE_NAME}
else
	GLOBAL_CACHE_DIR=/vcache/${DOCKER_IMAGE_NAME}
fi

SRC_VERSION_PATH=files/build/versions
if [ ! -z ${SONIC_VERSION_CACHE} ]; then

	# Version files for SHA calculation
	VERSION_FILES="${SRC_VERSION_PATH}/dockers/${DOCKER_IMAGE_NAME}/versions-*-${DISTRO}-${ARCH} ${SRC_VERSION_PATH}/default/versions-*"
	DEP_FILES="Dockerfile.j2"
	if [[ ${DOCKER_IMAGE_NAME} =~ '-dbg' ]]; then
		DEP_DBG_FILES="build_debug_docker_j2.sh"
	fi

	#Calculate the version SHA
	VERSION_SHA="$( (echo -n "${!IMAGE_DBGS_NAME}"; \
		(cd ${DOCKER_PATH}; cat ${DEP_FILES}); \
		cat ${DEP_DBG_FILES} ${VERSION_FILES}) \
		| sha1sum | awk '{print substr($1,0,23);}')"

	GLOBAL_CACHE_FILE=${GLOBAL_CACHE_DIR}/${DOCKER_IMAGE_NAME}-${VERSION_SHA}.tgz
	LOCAL_CACHE_FILE=${LOCAL_CACHE_DIR}/cache.tgz
	GIT_FILE_STATUS=$(git status -s ${DEP_FILES})

	# Create the empty cache tar file as local cache
	if [[ ! -f ${LOCAL_CACHE_FILE} ]]; then
		tar -zcf ${LOCAL_CACHE_FILE} -T /dev/null
		chmod -f 777 ${LOCAL_CACHE_FILE}
	fi

	# Global cache file exists, load from global cache.
	if [[  -e ${GLOBAL_CACHE_FILE} ]]; then
		cp ${GLOBAL_CACHE_FILE} ${LOCAL_CACHE_FILE}
		touch ${GLOBAL_CACHE_FILE}
	else
		# When file is modified, Global SHA is calculated with the local change.
		# Load from the previous version of build cache if exists
		VERSIONS=( "HEAD" "HEAD~1" "HEAD~2" )
		for VERSION in ${VERSIONS[@]}; do
			VERSION_PREV_SHA="$( (echo -n "${!IMAGE_DBGS_NAME}"; \
				(cd ${DOCKER_PATH}; git --no-pager show $(ls -f ${DEP_FILES}|sed 's|.*|'${VERSION}':./&|g')); \
				(git --no-pager show $(ls -f ${DEP_DBG_FILES} ${VERSION_FILES}|sed 's|.*|'${VERSION}':&|g'))) \
				| sha1sum | awk '{print substr($1,0,23);}')"
			GLOBAL_PREV_CACHE_FILE=${GLOBAL_CACHE_DIR}/${DOCKER_IMAGE_NAME}-${VERSION_PREV_SHA}.tgz
			if [[  -e ${GLOBAL_PREV_CACHE_FILE} ]]; then
				cp ${GLOBAL_PREV_CACHE_FILE} ${LOCAL_CACHE_FILE}
				touch ${GLOBAL_PREV_CACHE_FILE}
				break
			fi
		done
	fi

	rm -f ${DOCKER_PATH}/vcache/cache.tgz
	ln -f ${LOCAL_CACHE_FILE} ${DOCKER_PATH}/vcache/cache.tgz


else
	# Delete the cache file if version cache is disabled.
	rm -f ${DOCKER_PATH}/vcache/cache.tgz
fi

