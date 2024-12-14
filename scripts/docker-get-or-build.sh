#!/bin/sh

git_root() {
    git rev-parse --show-toplevel
}

get_dockerfile() {
  echo Dockerfile
}

get_version_hash() {
  cat $(get_dockerfile) scripts/prepare-docker-base-dependencies.sh scripts/prepare-docker-wasm-dependencies.sh scripts/prepare-docker-android-dependencies.sh | md5sum - | head -c8
}

docker_base() {
  registry_image=${CI_REGISTRY_IMAGE:-"docker.io/michele1835/libgaussianblur"}
  echo $registry_image
}

docker_base_image() {
  echo "$(docker_base):$(get_version_hash)"
}

docker_image_exists_locally() {
    [ $(docker images "$1" | wc -l) -gt 1 ]
}

docker_push_image() {
  echo "Pushing docker image to remote"
  # Create :latest alias for current revisioned image
  docker tag $(docker_base_image) $(docker_base):latest
  docker push $(docker_base_image)
  docker push $(docker_base):latest
}

parse_arguments() {
  while test $# -gt 0; do
    case "$1" in
      --push)
        shift ; docker_push_flag=true ; prepare_dependencies=true
        ;;
      *)
        shift
        ;;
    esac
  done
}

cd $(git_root)
parse_arguments "$@"

echo "Checking if docker image already exists: $(docker_base_image)"
if ! docker_image_exists_locally $(docker_base_image) && ! docker pull $(docker_base_image) 2>/dev/null ; then
  echo "Docker image not found."
  if [ $prepare_dependencies ] ; then
    echo "Preparing to build now."
    ./scripts/prepare-docker-base-dependencies.sh
    ./scripts/prepare-docker-wasm-dependencies.sh
    ./scripts/prepare-docker-android-dependencies.sh
    echo "Dependencies downloaded."
  fi

  echo "Building image now."
  docker build . -f $(get_dockerfile) -t $(docker_base_image)
  if [ $docker_push_flag ] ; then
    docker_push_image
  fi
else
  echo "Docker image found. Skipping pull/build/push operations"
  if [ $docker_push_flag ] && ! docker pull $(docker_base_image) 2>/dev/null ; then
    echo "Docker image not found on remote, and --push flag set. Pushing now."
    docker_push_image
  fi
fi
