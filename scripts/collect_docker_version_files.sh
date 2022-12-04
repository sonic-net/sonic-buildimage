#!/bin/bash
  
set -x 
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

docker container rm $DOCKER_CONTAINER
