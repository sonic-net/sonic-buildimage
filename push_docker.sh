#! /bin/bash

sonic_version=""
sonic_platform=""

while getopts ":v:p:" opt
do
    case ${opt} in
        v ) # SONiC image version
            sonic_version=${OPTARG}
            ;;
        p ) # Platform info
            sonic_platform=${OPTARG}
            ;;
        \? ) echo "\
Usage: [-v <version> ] [ -p <platform> ] <DOCKER_IMAGE_FILE> <REGISTRY_SERVER> <REGISTRY_PORT> \
<REGISTRY_USERNAME> <REGISTRY_PASSWD> [<DOCKER_IMAGE_TAG>]" 
            ;;
    esac
done

shift $((OPTIND -1))

DOCKER_IMAGE_FILE=$1
REGISTRY_SERVER=$2
REGISTRY_PORT=$3
REGISTRY_USERNAME=$4
REGISTRY_PASSWD=$5
DOCKER_IMAGE_TAG=$6
REGISTRY_SERVER_WITH_PORT=${REGISTRY_SERVER}${REGISTRY_PORT:+:$REGISTRY_PORT}

push_it() {
    # $1 - Given image name
    # $2 - Remote image name

    docker tag $1 $2
    echo "Pushing $2"
    image_sha=$(docker push $2 | sed -n "s/.*: digest: sha256:\([0-9a-f]*\).*/\\1/p")
    echo "Remove $2"
    docker rmi $2 || true
    echo "Image sha256: $image_sha"
}

set -e

echo "Loading image ${DOCKER_IMAGE_FILE}"
docker load < ${DOCKER_IMAGE_FILE}

## Login the docker image registry server
## Note: user name and password are passed from command line
docker login -u ${REGISTRY_USERNAME} -p "${REGISTRY_PASSWD}" ${REGISTRY_SERVER_WITH_PORT}

## Get Docker image name
docker_image_name=$(basename ${DOCKER_IMAGE_FILE} | cut -d. -f1)
remote_image_name=${REGISTRY_SERVER_WITH_PORT}/${docker_image_name}

for tag in ${DOCKER_IMAGE_TAG}
do
    push_it ${docker_image_name} ${remote_image_name}:$tag
done

if [ -n "${sonic_version}" ] && [ -n "${sonic_platform}" ]
then
    remote_image_name=${REGISTRY_SERVER_WITH_PORT}/sonic-dockers/${sonic_platform}/${docker_image_name}:${sonic_version}
    push_it ${docker_image_name} ${remote_image_name}
else
    ## Fetch the Jenkins build number if inside it
    [ ${BUILD_NUMBER} ] || {
        echo "No BUILD_NUMBER found, setting to 0."
        BUILD_NUMBER="0"
    }

    timestamp="$(date -u +%Y%m%d)"
    build_version="${timestamp}.bld-${BUILD_NUMBER}"
    push_it ${docker_image_name} ${remote_image_name}:${build_version}
fi

docker rmi $docker_image_name || true
echo "Job completed"

