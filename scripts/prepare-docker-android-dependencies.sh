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
OPENJDK_VERSION=21.0.2

DOCKER_WORKDIR=.docker

if [ ! -d $DOCKER_WORKDIR ] ; then mkdir $DOCKER_WORKDIR ; fi

msg() {
    echo "${GREEN}$@${RESET}"
}
errmsg() {
    echo "${RED}$@${RESET}" 1>&2
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
        wget -q "$download_url" -O $expected_archive_file
    fi
}

download_openjdk() {
    archive_name=openjdk.tar.gz
    expected_archive_file="${DOCKER_WORKDIR}/$archive_name"
    if [ -f $expected_archive_file ] ; then
        msg "OpenJDK ${OPENJDK_VERSION} archive '$expected_archive_file' exists. ${YELLOW}[Skipping]"
    else
        msg "Downloading OpenJDK ${OPENJDK_VERSION} archive."
        download_host_url="https://download.java.net/java/GA/jdk${OPENJDK_VERSION}/f2283984656d49d69e91c558476027ac/13/GPL/openjdk-${OPENJDK_VERSION}_linux-x64_bin.tar.gz"
        wget -q "$download_host_url" -O $expected_archive_file
    fi
}

download_openjdk
download_android_commandline_tools