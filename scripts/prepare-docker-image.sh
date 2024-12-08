#!/bin/sh

GREEN='\033[0;32m'
GRAY='\033[0;37m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
LOG_COLOR='\033[1;30m'
RESET='\033[0m'

# VERSION CONFIGURATION
ANDROID_CLI_TOOLS_VERSION=11076708_latest

DOCKER_WORKDIR=.docker
LIBRARY_WORKDIR=.deps

if [ ! -d $DOCKER_WORKDIR ] ; then mkdir $DOCKER_WORKDIR ; fi

msg() {
    echo "${GREEN}$@${RESET}"
}
errmsg() {
    echo "${RED}$@${RESET}" 1>&2
}

get_gaussianblur_sources() {
  GAUSSIANBLUR_VERSION=$(sed -n 's/^GAUSSIANBLUR_VERSION=//p' bootstrap/bootstrap.gaussianblur.sh | tr -d \")
  PREFIX=$(sed -n 's/^PREFIX=//p' bootstrap/bootstrap.gaussianblur.sh | tr -d \")
  if [ -d "${LIBRARY_WORKDIR}/$PREFIX" ] ; then
    rm -rf "${LIBRARY_WORKDIR}/$PREFIX"
  fi
  git clone -c advice.detachedHead=false --depth 1 --branch $GAUSSIANBLUR_VERSION git@github.com:michelerenzullo/LibGaussianBlur.git "${LIBRARY_WORKDIR}/$PREFIX"
  git -C "${LIBRARY_WORKDIR}/$PREFIX" submodule update --init --recursive
}

download_emscripten_sources() {
  archive_name=emsdk.zip
  version="3.1.66"
  expected_archive_file="${DOCKER_WORKDIR}/$archive_name"
  if [ -f $expected_archive_file ] ; then
    msg "Emscripten SDK archive '$expected_archive_file' exists. ${YELLOW}[Skipping]"
  else
    msg "Downloading emscripten sdk archive for version $version."
    download_host_url="https://github.com/emscripten-core/emsdk/archive/refs/tags/"
    download_url="${download_host_url}/$version.zip"
    wget "$download_url" -O $expected_archive_file
  fi
}

download_android_commandline_tools() {
    archive_name=commandlinetools-linux.zip
    expected_archive_file="${DOCKER_WORKDIR}/$archive_name"
    if [ -f $expected_archive_file ] ; then
        msg "Android CLI tools archive '$expected_archive_file' exists. ${YELLOW}[Skipping]"
    else
        msg "Downloading android cli tools archive for version $ANDROID_CLI_TOOLS_VERSION."
        download_host_url="https://dl.google.com/android/repository/"
        download_url="${download_host_url}/commandlinetools-linux-$ANDROID_CLI_TOOLS_VERSION.zip"
        wget "$download_url" -O $expected_archive_file
    fi
}

download_openjdk21() {
    archive_name=openjdk-21.0.2_linux-x64_bin.tar.gz
    expected_archive_file="${DOCKER_WORKDIR}/$archive_name"
    if [ -f $expected_archive_file ] ; then
        msg "OpenJDK 21 archive '$expected_archive_file' exists. ${YELLOW}[Skipping]"
    else
        msg "Downloading OpenJDK 21 archive."
        download_host_url="https://download.java.net/java/GA/jdk21.0.2/f2283984656d49d69e91c558476027ac/13/GPL/${archive_name}"
        wget "$download_host_url" -O $expected_archive_file
    fi
}

download_openjdk21
download_android_commandline_tools
download_emscripten_sources
get_gaussianblur_sources