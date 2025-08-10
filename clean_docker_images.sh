#!/bin/sh

DOCKER_CMD=docker
if [ ! `which $DOCKER_CMD` ]
then  echo "no docker command available" >&2
    exit 1
fi
#if "docker ps" cannot be run without error, prepend sudo
if ( ! $DOCKER_CMD ps >/dev/null 2>&1 );then
    echo "docker command only usable as root, using sudo" >&2
    DOCKER_CMD="sudo docker"
fi


IMAGES_LIST=$($DOCKER_CMD image ls "sonic*" | sed 's/  */\t/g' | cut -f3 | tail -n +2)

if [ -z $IMAGES_LIST ]; then
    echo "No docker image to delete"
    exit 0
fi

echo -n "Run '$DOCKER_CMD image rm -f $(echo $IMAGES_LIST) ' (y/n)?"
initial_stty_cfg=$(stty -g)
stty raw -echo
answer=$( while ! head -c 1 | grep -i '[yn]' ;do true ;done )
stty $initial_stty_cfg
if echo "$answer" | grep -iq "^y" ;then
    $DOCKER_CMD image rm -f $(echo $IMAGES_LIST)
else
    echo "Nothing done"
    exit 0
fi
echo -n "Run '$DOCKER_CMD image prune -af' (y/n)?"
initial_stty_cfg=$(stty -g)
stty raw -echo
answer=$( while ! head -c 1 | grep -i '[yn]' ;do true ;done )
stty $initial_stty_cfg
if echo "$answer" | grep -iq "^y" ;then
    $DOCKER_CMD image prune -af
fi
