#!/bin/sh

GREEN='\033[0;32m'
GRAY='\033[0;37m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
LOG_COLOR='\033[1;30m'
RESET='\033[0m'

LIBRARY_WORKDIR=.deps

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
  git clone -c advice.detachedHead=false --depth 1 --branch $GAUSSIANBLUR_VERSION https://github.com/michelerenzullo/LibGaussianBlur.git "${LIBRARY_WORKDIR}/$PREFIX"
  git -C "${LIBRARY_WORKDIR}/$PREFIX" submodule update --init --recursive 
}

get_gaussianblur_sources