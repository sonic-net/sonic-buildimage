#!/bin/bash

IMAGENAME=$1
DOCKERFILE=$2
ARCH=$3
DOCKERFILE_TARGE=$4
DISTRO=$5

[ -z "$BUILD_SLAVE" ] && BUILD_SLAVE=n
[ -z "$DOCKERFILE_TARGE" ] && DOCKERFILE_TARGE=$DOCKERFILE
DOCKERFILE_PATH=$(dirname "$DOCKERFILE_TARGE")
BUILDINFO_PATH="${DOCKERFILE_PATH}/buildinfo"
BUILDINFO_VERSION_PATH="${BUILDINFO_PATH}/versions"
DOCKER_PATH=$(dirname $DOCKERFILE)

[ -d $BUILDINFO_PATH ] && rm -rf $BUILDINFO_PATH
mkdir -p $BUILDINFO_VERSION_PATH

# Get the debian distribution from the docker base image
if [ -z "$DISTRO" ]; then
    DOCKER_BASE_IMAGE=$(grep "^FROM" $DOCKERFILE | head -n 1 | awk '{print $2}')
    DISTRO=$(docker run --rm --entrypoint "" $DOCKER_BASE_IMAGE cat /etc/os-release | grep VERSION_CODENAME | cut -d= -f2)
    if [ -z "$DISTRO" ]; then
        DISTRO=$(docker run --rm --entrypoint "" $DOCKER_BASE_IMAGE cat /etc/apt/sources.list | grep deb.debian.org | awk '{print $3}')
        [ -z "$DISTRO" ] && DISTRO=jessie
    fi
fi

if [[ "$IMAGENAME" == docker-base-* ]]; then
    scripts/build_mirror_config.sh ${DOCKERFILE_PATH} $ARCH $DISTRO
fi

# add script for reproducible build. using sha256 instead of tag for docker base image.
scripts/docker_version_control.sh $@

DOCKERFILE_PRE_SCRIPT='# Auto-Generated for buildinfo
COPY ["buildinfo", "/usr/local/share/buildinfo"]
COPY vcache/ /sonic/target/vcache/'${IMAGENAME}'
RUN dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb
ENV IMAGENAME='${IMAGENAME}'
RUN pre_run_buildinfo '${IMAGENAME}'
'

# Add the auto-generate code if it is not added in the target Dockerfile
if [ ! -f $DOCKERFILE_TARGE ] || ! grep -q "Auto-Generated for buildinfo" $DOCKERFILE_TARGE; then
    # Insert the docker build script before the RUN command
    LINE_NUMBER=$(grep -Fn -m 1 'RUN' $DOCKERFILE | cut -d: -f1)
    TEMP_FILE=$(mktemp)
    awk -v text="${DOCKERFILE_PRE_SCRIPT}" -v linenumber=$LINE_NUMBER 'NR==linenumber{print text}1' $DOCKERFILE > $TEMP_FILE

    # Append the docker build script at the end of the docker file
    echo -e "\nRUN post_run_buildinfo ${IMAGENAME} " >> $TEMP_FILE

    cat $TEMP_FILE > $DOCKERFILE_TARGE
    rm -f $TEMP_FILE
fi

# Copy the build info config
cp -rf src/sonic-build-hooks/buildinfo/* $BUILDINFO_PATH

# Generate the version lock files
scripts/versions_manager.py generate -t "$BUILDINFO_VERSION_PATH" -n "$IMAGENAME" -d "$DISTRO" -a "$ARCH"

touch $BUILDINFO_VERSION_PATH/versions-deb

# Create the cache directories
LOCAL_CACHE_DIR=target/vcache/${IMAGENAME}
mkdir -p ${LOCAL_CACHE_DIR} ${DOCKER_PATH}/vcache/
chmod -f 777 ${LOCAL_CACHE_DIR} ${DOCKER_PATH}/vcache/
