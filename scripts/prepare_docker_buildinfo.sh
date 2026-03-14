#!/bin/bash

grep "^# SKIP_HOOK" $2 && exit 0

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
fi

# add script for reproducible build. using sha256 instead of tag for docker base image.
scripts/docker_version_control.sh $@

DOCKERFILE_PRE_SCRIPT='# Auto-Generated for buildinfo
ARG SONIC_VERSION_CACHE
ARG SONIC_VERSION_CONTROL_COMPONENTS
COPY ["buildinfo", "/usr/local/share/buildinfo"]
COPY vcache/ /sonic/target/vcache/'${IMAGENAME}'
RUN dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb
ENV IMAGENAME='${IMAGENAME}'
ENV DISTRO='${DISTRO}'
RUN pre_run_buildinfo '${IMAGENAME}'
'

DOCKERFILE_POST_SCRIPT='
RUN post_run_buildinfo '${IMAGENAME}'
RUN post_run_cleanup '${IMAGENAME}'
'

# Add the auto-generate code if it is not added in the target Dockerfile
if [ ! -f $DOCKERFILE_TARGET ] || ! grep -q "Auto-Generated for buildinfo" $DOCKERFILE_TARGET; then
    # Insert the docker build script before the RUN command
    LINE_NUMBER=$(grep -Fn -m 1 'RUN' $DOCKERFILE | cut -d: -f1)
    COPY_BASE_LINE_NUMBER=$(grep -n -m 1 'FROM \$BASE$' $DOCKERFILE | cut -d: -f1)
    if [ -z "$COPY_BASE_LINE_NUMBER" ]; then
        COPY_BASE_LINE_NUMBER=$(grep -n -m 1 'FROM scratch$' $DOCKERFILE | cut -d: -f1)
    fi
    TEMP_FILE=$(mktemp)
    if [ -n "$COPY_BASE_LINE_NUMBER" ]; then
        awk -v prescript="${DOCKERFILE_PRE_SCRIPT}" -v linenumber=$LINE_NUMBER -v postscript="${DOCKERFILE_POST_SCRIPT}" -v copybaselinenumber=$COPY_BASE_LINE_NUMBER 'NR==copybaselinenumber{print postscript} NR==linenumber{print prescript}1' $DOCKERFILE > $TEMP_FILE
    else
        awk -v prescript="${DOCKERFILE_PRE_SCRIPT}" -v linenumber=$LINE_NUMBER 'NR==linenumber{print prescript}1' $DOCKERFILE > $TEMP_FILE

        # Append the docker build script at the end of the docker file
        echo -e "\nRUN post_run_buildinfo ${IMAGENAME} " >> $TEMP_FILE
        echo -e "\nRUN post_run_cleanup ${IMAGENAME} " >> $TEMP_FILE
    fi

    cat $TEMP_FILE > $DOCKERFILE_TARGET
    rm -f $TEMP_FILE
fi

# Copy the build info config
mkdir -p ${BUILDINFO_PATH}
cp -rf src/sonic-build-hooks/buildinfo/* $BUILDINFO_PATH

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

