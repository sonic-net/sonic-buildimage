#!/bin/bash

# This script is for reproducible build.
# Reproducible build for docker enabled: Before build docker image, this script will change image:tag to image:sha256 in DOCKERFILE.
# And record image sha256 to a target file.

IMAGENAME=$1
DOCKERFILE=$2
ARCH=$3
DOCKERFILE_TARGE=$4
DISTRO=$5

version_file=files/build/versions/default/versions-docker
new_version_file=target/versions/default/versions-docker
mkdir -p target/versions/default

. src/sonic-build-hooks/buildinfo/config/buildinfo.config

image_tag=`grep "^ARG BASE=" $DOCKERFILE | cut -d= -f 2`
if [ -z $image_tag ]; then
	image_tag=`grep "^FROM " $DOCKERFILE | awk '{print$2}'`
fi
image_tag_noprefix=$image_tag
[ -n "$DEFAULT_CONTAINER_REGISTRY" ] && image_tag_noprefix=$(echo $image_tag | sed "s#$DEFAULT_CONTAINER_REGISTRY##")
image=`echo $image_tag | cut -f1 -d:`
tag=`echo $image_tag | cut -f2 -d:`

if [[ ",$SONIC_VERSION_CONTROL_COMPONENTS," == *,all,* ]] || [[ ",$SONIC_VERSION_CONTROL_COMPONENTS," == *,docker,* ]]; then
    # if docker image not in white list, exit
    if [[ "$image_tag" != */debian:* ]] && [[ "$image_tag" != debian:* ]] && [[ "$image_tag" != multiarch/debian-debootstrap:* ]];then
        exit 0
    fi
    if [ -f $version_file ];then
        hash_value=`grep "${ARCH}:${image_tag_noprefix}" $version_file | awk -F== '{print$2}'`
    fi
    if [ -z $hash_value ];then
        hash_value=unknown
        echo "$image_tag sha256 value is unknown" >> ${new_version_file}.log
        exit 0
    fi
    oldimage=${image_tag//\//\\/}
    newimage="${oldimage}@$hash_value"
    echo "sed -i \"s/$oldimage/$newimage/\" $DOCKERFILE" >> ${new_version_file}.log
    sed -i "s/$oldimage/$newimage/" $DOCKERFILE
else
    hash_value=`docker pull $image_tag 2> ${new_version_file}.log | grep Digest | awk '{print$2}'`
    if [ -z "$hash_value" ];then
        hash_value=unknown
    fi
fi
if [[ "$hash_value" != "unknown" ]];then
    echo -e "${ARCH}:${image_tag_noprefix}==$hash_value" >> $new_version_file
fi
