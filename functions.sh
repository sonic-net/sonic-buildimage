#!/bin/bash
## Function Definitions

## Function: trap_push 'COMMAND_STRING'
## Appends a command to a trap, which is needed because default trap behavior is to replace
## previous trap for the same signal
## - 1st arg:  code to add
## - ref: http://stackoverflow.com/questions/3338030/multiple-bash-traps-for-the-same-signal
_trap_push() {
    local next="$1"
    eval "trap_push() {
        local oldcmd='$(echo "$next" | sed -e s/\'/\'\\\\\'\'/g)'
        local newcmd=\"\$1; \$oldcmd\"
        trap -- \"\$newcmd\" EXIT INT TERM HUP
        _trap_push \"\$newcmd\"
    }"
}
_trap_push true

## Function: warn MESSAGE
## Print message to stderr
warn() {
    local message="$1"
    echo "$message" >&2
}

## Function: die MESSAGE
## Print message to stderr and exit the whole process
## Note:
##   Using () makes the command inside them run in a sub-shell and calling a exit from there
##   causes you to exit the sub-shell and not your original shell, hence execution continues in
##   your original shell. To overcome this use { }
## ref: http://stackoverflow.com/questions/3822621/how-to-exit-if-a-command-failed
die() {
    local message="$1"
    warn "$message"
    exit 1
}

docker_try_rmi() {
    local image_name="$1"
    ## Note: inspect output has quotation characters, so sed to remove it as an argument
    local image_id=$(docker inspect --format="{{json .Id}}" $image_name | sed -e 's/^"//' -e 's/"$//')
    [ -z "$image_id" ] || {
        ## Remove all the exited containers from this image
        docker ps -a -q -f "status=exited" -f "ancestor=$1" | xargs --no-run-if-empty docker rm
        ## Note: If there are running containers from this image, the build system is in an
        ##   unexpected state. The 'rmi' will fail and we need investigate the build environment.
        docker rmi $image_name
    }
}

sonic_get_version() {
    local describe=$(git describe --tags 2>/dev/null)
    local latest_tag=$(git describe --tags --abbrev=0 2>/dev/null)
    local branch_name=$(git rev-parse --abbrev-ref HEAD)
    if [ -n "$(git status --untracked-files=no -s --ignore-submodules)" ]; then
        local dirty="-dirty-$BUILD_TIMESTAMP"
    fi
    BUILD_NUMBER=${BUILD_NUMBER:-0}
    ## Check if we are on tagged commit
    ## Note: escape the version string by sed: / -> _
    if [ -n "$latest_tag" ] && [ "$describe" == "$latest_tag" ]; then
        echo "${latest_tag}${dirty}" | sed 's/\//_/g'
    else
        echo "${branch_name}.${BUILD_NUMBER}${dirty:--$(git rev-parse --short HEAD)}" | sed 's/\//_/g'
    fi
}
