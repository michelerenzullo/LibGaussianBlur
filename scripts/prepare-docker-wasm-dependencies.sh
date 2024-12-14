#!/bin/sh

GREEN='\033[0;32m'
GRAY='\033[0;37m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
LOG_COLOR='\033[1;30m'
RESET='\033[0m'

# VERSION CONFIGURATION
EMSCRIPTEN_VERSION=3.1.66

DOCKER_WORKDIR=.docker

if [ ! -d $DOCKER_WORKDIR ] ; then mkdir $DOCKER_WORKDIR ; fi

msg() {
    echo "${GREEN}$@${RESET}"
}
errmsg() {
    echo "${RED}$@${RESET}" 1>&2
}

download_emscripten_sources() {
  archive_name=emsdk.zip
  expected_archive_file="${DOCKER_WORKDIR}/$archive_name"
  if [ -f $expected_archive_file ] ; then
    msg "Emscripten SDK archive '$expected_archive_file' exists. ${YELLOW}[Skipping]"
  else
    msg "Downloading emscripten sdk archive for version $EMSCRIPTEN_VERSION."
    download_host_url="https://github.com/emscripten-core/emsdk/archive/refs/tags/"
    download_url="${download_host_url}/$EMSCRIPTEN_VERSION.zip"
    wget -q "$download_url" -O $expected_archive_file
  fi
}

download_emscripten_sources
