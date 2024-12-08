#!/bin/sh

GREEN='\033[0;32m'
GRAY='\033[0;37m'
RED='\033[0;31m'
BOLD_RED='\033[1;31m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
BOLD_YELLOW='\033[1;33m'
LOG_COLOR='\033[1;30m'
RESET='\033[0m'

TEMP_EXTERNAL_BUILD_DIR=.deps

init_into_build_dir() {
    cd $(git_submodule_or_root_dir)
    if [ ! -d "${TEMP_EXTERNAL_BUILD_DIR}" ] ; then mkdir "${TEMP_EXTERNAL_BUILD_DIR}" ; fi
    cd $TEMP_EXTERNAL_BUILD_DIR
}

is_docker_container() {
    # If file /.dockerenv exists, we are in a docker container, otherwise
    #   if file /proc/1/cgroup does not exist, we are not in a container
    #     otherwise if contains '0::/' ended with a newline or 'docker', we are in a container.
    [ -f /.dockerenv ] && return 0 || { [ ! -f /proc/1/cgroup ] && return 1 || { grep -qE '^0::/$|docker' /proc/1/cgroup && return 0 || return 1; }; }
}

git_submodule_or_root_dir() {
    if is_docker_container; then echo '/app'; return ; fi
    git rev-parse --show-superproject-working-tree --show-toplevel | tail -n1
}

git_root() {
    git_submodule_or_root_dir
}

command_exists() {
    [ ! -z $(command -v $1 2>/dev/null) ]
}

#1 Command
#2 Hint about how to get command
command_exists_with_info() {
    if ! command_exists $1 ; then
        echo_error "${BOLD_RED}$1${RED} not found, ${YELLOW}\"$2\" might help"
        exit 1
    fi
}

get_cpu_cores() {
    if command_exists system_profiler ; then
        echo $(system_profiler SPHardwareDataType | awk '/Total Number of Cores/ {print $5}')
        return
    fi
    if test -f "/proc/cpuinfo"; then
        echo $(cat /proc/cpuinfo | grep 'cpu cores' | wc -l)
        return
    fi
    echo 6
}

echo_error() { echo "$@" 1>&2; }

#1 URL
#2 Optional argument for output file name
curl_or_wget() {
    if type wget >/dev/null; then
        if [ -z "$2" ] ; then
            wget $1
        else
            wget $1 --output-document "$2"
        fi
        return
    fi
    if type curl >/dev/null; then
        if [ -z "$2" ] ; then
            curl -OL $1
        else
            curl -L $1 > $2
        fi
        return
    fi
    echo "wget or curl is required to download source files"
    exit 1
}
