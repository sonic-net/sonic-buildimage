#!/bin/bash

[[ ! -z "${DBGOPT}" && $0 =~ ${DBGOPT} ]] && set -x

BUILDINFO_BASE=/usr/local/share/buildinfo

SCRIPT_SRC_PATH=src/sonic-build-hooks
if [ -e ${SCRIPT_SRC_PATH} ]; then
	. ${SCRIPT_SRC_PATH}/scripts/utils.sh
else
	. ${BUILDINFO_BASE}/scripts/utils.sh
fi

DOCKER_IMAGE=$1
TARGET_PATH=$2
DOCKER_IMAGE_TAG=$3
DOCKER_PATH=$4
DOCKER_FILE=$5

[ -z "$TARGET_PATH" ] && TARGET_PATH=./target

DOCKER_IMAGE_NAME=$(echo $DOCKER_IMAGE | cut -d: -f1 | sed "s/-$DOCKER_USERNAME\$//")
#Create the container specific to the user tag and slave tag
DOCKER_CONTAINER=${DOCKER_IMAGE_TAG/:/-}
TARGET_VERSIONS_PATH=$TARGET_PATH/versions/dockers/$DOCKER_IMAGE_NAME
BUILD_LOG_PATH=target/versions/log/$DOCKER_IMAGE_NAME
mkdir -p ${BUILD_LOG_PATH}

[ -d $TARGET_VERSIONS_PATH ] && rm -rf $TARGET_VERSIONS_PATH
mkdir -p $TARGET_VERSIONS_PATH

export DOCKER_CLI_EXPERIMENTAL=enabled

# Remove the old docker container if existing
if docker container inspect $DOCKER_IMAGE > /dev/null 2>&1; then
    docker container rm $DOCKER_IMAGE > /dev/null
fi
docker create --name $DOCKER_CONTAINER --entrypoint /bin/bash $DOCKER_IMAGE_TAG
docker cp -L $DOCKER_CONTAINER:/etc/os-release $TARGET_VERSIONS_PATH/
docker cp -L $DOCKER_CONTAINER:/usr/local/share/buildinfo/pre-versions $TARGET_VERSIONS_PATH/
docker cp -L $DOCKER_CONTAINER:/usr/local/share/buildinfo/post-versions $TARGET_VERSIONS_PATH/

# Save the cache contents from docker build
IMAGENAME=${DOCKER_IMAGE_TAG} j2 files/build_templates/build_docker_cache.j2 > ${DOCKER_FILE}.cleanup
docker tag ${DOCKER_IMAGE_TAG} tmp-${DOCKER_IMAGE_TAG}
DOCKER_BUILDKIT=1 docker build -f ${DOCKER_PATH}/Dockerfile.cleanup  --target output -o target/vcache/${DOCKER_IMAGE_NAME} ${DOCKER_PATH}
DOCKER_BUILDKIT=1 docker build -f ${DOCKER_PATH}/Dockerfile.cleanup  --no-cache --target final --tag ${DOCKER_IMAGE_TAG} ${DOCKER_PATH}
docker rmi tmp-${DOCKER_IMAGE_TAG}
docker cp -L $DOCKER_CONTAINER:/usr/local/share/buildinfo/log ${BUILD_LOG_PATH}/


# Save the cache contents from docker build
LOCAL_CACHE_FILE=target/vcache/${DOCKER_IMAGE_NAME}/cache.tgz
CACHE_ENCODE_FILE=${DOCKER_PATH}/vcache/cache.base64
sleep 1; sync ${CACHE_ENCODE_FILE}

# Decode the cache content into gz format
SRC_VERSION_PATH=files/build/versions
if [[ -e ${CACHE_ENCODE_FILE} ]]; then

	cat ${CACHE_ENCODE_FILE} | base64 -d  >${LOCAL_CACHE_FILE}
	rm -f ${CACHE_ENCODE_FILE}
fi

# Version package cache
IMAGE_DBGS_NAME=${DOCKER_IMAGE_NAME//-/_}_image_dbgs
if [[ ${DOCKER_IMAGE_NAME} == sonic-slave-* ]]; then
	GLOBAL_CACHE_DIR=${SONIC_VERSION_CACHE_SOURCE}/${DOCKER_IMAGE_NAME}
else
	GLOBAL_CACHE_DIR=/vcache/${DOCKER_IMAGE_NAME}
fi

if [[ ! -z ${SONIC_VERSION_CACHE} && -e ${CACHE_ENCODE_FILE} ]]; then

	# Select version files for SHA calculation
	VERSION_FILES="${SRC_VERSION_PATH}/dockers/${DOCKER_IMAGE_NAME}/versions-*-${DISTRO}-${ARCH} ${SRC_VERSION_PATH}/default/versions-*"
	DEP_FILES="${DOCKER_PATH}/Dockerfile.j2"
	if [[ ${DOCKER_IMAGE_NAME} =~ '-dbg' ]]; then
		DEP_FILES="${DEP_FILES} build_debug_docker_j2.sh"
	fi

	# Calculate the version SHA
	VERSION_SHA="$( (echo -n "${!IMAGE_DBGS_NAME}"; cat ${DEP_FILES} ${VERSION_FILES}) | sha1sum | awk '{print substr($1,0,23);}')"
    GLOBAL_CACHE_FILE=${GLOBAL_CACHE_DIR}/${DOCKER_IMAGE_NAME}-${VERSION_SHA}.tgz

	GIT_FILE_STATUS=$(git status -s ${DEP_FILES})

	# If the cache file is not exists in the global cache for the given SHA,
	# store the new cache file into version cache path.
	if [ -f ${LOCAL_CACHE_FILE} ]; then
		if [[ -z ${GIT_FILE_STATUS} && ! -e ${GLOBAL_CACHE_FILE} ]]; then
			mkdir -p ${GLOBAL_CACHE_DIR}
			chmod -f 777 ${GLOBAL_CACHE_DIR}
			FLOCK ${GLOBAL_CACHE_FILE}
			cp ${LOCAL_CACHE_FILE} ${GLOBAL_CACHE_FILE}
			chmod -f 777 ${LOCAL_CACHE_FILE} ${GLOBAL_CACHE_FILE}
			FUNLOCK ${GLOBAL_CACHE_FILE}
		fi
	fi
fi

docker container rm $DOCKER_CONTAINER
