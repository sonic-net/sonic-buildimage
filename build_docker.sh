#!/bin/bash
## This script is to automate the preparation for docker images for SONiC.
## If registry server and port provided, the images will be pushed there.

set -e

. ./functions.sh

usage() {
    cat >&2 <<EOF
Usage:
  sudo ./build_docker.sh [-i DOCKER_IMAGE_NAME] [-t DOCKER_IMAGE_TAG] DOCKER_BUILD_DIR [REGISTRY_SERVER REGISTRY_PORT REGISTRY_USERNAME REGISTRY_PASSWD]
  
Description:
  -i DOCKER_IMAGE_NAME
       Specify the docker image's name, by default it is DOCKER_BUILD_DIR
  -t DOCKER_IMAGE_TAG
       Specify the docker image's tag, by default it is latest
  DOCKER_BUILD_DIR
       The directory containing Dockerfile
  REGISTRY_SERVER
       The server name of the docker registry
  REGISTRY_PORT
       The port of the docker registry
       
Example:
  ./build_docker.sh -i docker-orchagent-mlnx docker-orchagent
EOF
}

docker_image_name=''
docker_image_tag=latest
## The option-string tells getopts which options to expect and which of them must have an argument
## When you want getopts to expect an argument for an option, just place a : (colon) after the proper option flag
## If the very first character of the option-string is a :, getopts switches to "silent error reporting mode".
while getopts "i:t:" opt; do
  case $opt in
    i)
      docker_image_name=$OPTARG
      ;;
    t)
      docker_image_tag=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      exit 1
      ;;
  esac
done
shift "$((OPTIND - 1))"

## Dockerfile directory
DOCKER_BUILD_DIR=dockers/$1
shift 1

[ -f "$DOCKER_BUILD_DIR"/Dockerfile ] || {
    echo "Invalid DOCKER_BUILD_DIR directory" >&2
    exit 1
}

[ -n "$docker_image_name" ] || {
    docker_image_name=$(basename $DOCKER_BUILD_DIR)
}

## Copy dependencies
## Note: Dockerfile ADD doesn't support reference files outside the folder, so copy it locally
if ls deps/* 1>/dev/null 2>&1; then
    trap_push "rm -rf $DOCKER_BUILD_DIR/deps"
    mkdir -p $DOCKER_BUILD_DIR/deps
    cp -r deps/* $DOCKER_BUILD_DIR/deps
fi

## Copy the suggested Debian sources
## ref: https://wiki.debian.org/SourcesList
trap_push "rm -rf $DOCKER_BUILD_DIR/deps"
cp -r files $DOCKER_BUILD_DIR/files
docker_try_rmi $docker_image_name

## Build the docker image
docker build --no-cache -t $docker_image_name $DOCKER_BUILD_DIR
## Get the ID of the built image
## Note: inspect output has quotation characters, so sed to remove it as an argument
image_id=$(docker inspect --format="{{json .Id}}" $docker_image_name | sed -e 's/^"//' -e 's/"$//')

## Flatten the image by importing an exported container on this image
## Note: it will squash the image with only one layer and lost all metadata such as ENTRYPOINT,
##       so apply only to the base image
## TODO: wait docker-squash supporting Docker 1.10+
## ref: https://github.com/jwilder/docker-squash/issues/45
if [ "$docker_image_name" = "docker-base" ]; then
    ## Run old image in a container
    tmp_container=$(docker run -d ${docker_image_name} /bin/bash)
    ## Export the container's filesystem, then import as a new image
    docker export $tmp_container | docker import - ${docker_image_name}
    ## Remove the container
    docker rm -f $tmp_container || true
    ## Remove the old image
    docker rmi -f $image_id || true
fi

## Save the docker image in a gz file
mkdir -p target
command -v pigz > /dev/null && GZ_COMPRESS_PROGRAM=pigz || GZ_COMPRESS_PROGRAM=gzip
docker save $docker_image_name | $GZ_COMPRESS_PROGRAM -c > target/$docker_image_name.gz

if [ -n "$1" ]; then
    ./push_docker.sh target/$docker_image_name.gz $@ $docker_image_tag
fi
