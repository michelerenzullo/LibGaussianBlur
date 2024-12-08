#!/bin/sh

# Bootstrap prepares dependencies for each target platform/ABI
# The output file structure:
#
# build
# - prefix
#  - platform    [e.g. linux, android]
#    - abi       [e.g. x86_64, arm64-v8a]
#     - lib
#       ...      [e.g. libzxing.so]
#     - include
#       ...      [header files]

. $(dirname $0)/common.sh
cd $(git_submodule_or_root_dir)

check_prerequisites () {
    command_exists_with_info clang "sudo apt install clang"
    command_exists_with_info make "sudo apt install build-essential"
    command_exists_with_info cmake "sudo apt install cmake"
    command_exists_with_info unzip "sudo apt install unzip"
}

show_usage_and_exit() {
    echo "$(basename $0) <all_linux|all_darwin|android|linux|ios|macos_arm|macos_x86|wasm> [OPTIONS]"
    echo "
OPTIONS:
    -h, --help      Show this usage info and exit.
    -v, --verbose

Example: $(basename $0) linux -v
"
    exit "${1:-1}"
}

parse_arguments() {
    if [ $# -lt 1 ] ; then
        echo_error "${RED}Insufficient arguments${RESET}"
        show_usage_and_exit 1
    fi
    while test $# -gt 0; do
        case "$1" in
            -h|--help)
                show_usage_and_exit 0 ;;
            -v|--verbose)
                set -x
                shift ; verbose_flag=true
                ;;
            all_linux)
                shift ;
                android_targets=true
                linux_targets=true
                ;;
            all_darwin)
                shift ;
                ios_targets=true
                macos_arm_targets=true
                macos_x86_targets=true
                ;;
            android)
                shift ; android_targets=true
                ;;
            linux)
                shift ; linux_targets=true
                ;;
            ios)
                shift ; ios_targets=true
                ;;
            macos_arm)
                shift ; macos_arm_targets=true
                ;;
            macos_x86)
                shift ; macos_x86_targets=true
                ;;
            wasm)
                command_exists_with_info em++ "git clone emsdk and install"
                shift ; wasm_targets=true
                ;;
            -*)
                echo_error "${RED}Invalid flag: $1${RESET}"
                show_usage_and_exit 1
                ;;
            *)
                echo_error "${RED}Invalid argument: $1${RESET}"
                show_usage_and_exit 1
                ;;
        esac
    done
}

is_platform_valid_test_environment() {
    # TODO: add macos if it is relevant
    [ "$1" == "linux" ]
}

parse_arguments "$@"
check_prerequisites

if [ "$android_targets" ] ; then
    TARGETS="android x86_64
      android arm64-v8a
      android armeabi-v7a
      android x86"
fi
if [ "$linux_targets" ] ; then
    TARGETS="$TARGETS
      linux x86_64"
fi
if [ "$ios_targets" ] ; then
    TARGETS="ios arm64"
fi
if [ "$macos_arm_targets" ] ; then
    TARGETS="$TARGETS
      macos arm64"
fi
if [ "$macos_x86_targets" ] ; then
    TARGETS="$TARGETS
      macos x86_64"
fi
if [ "$wasm_targets" ] ; then
    TARGETS="$TARGETS
      wasm wasm32"
fi

export CC=clang
export CXX=clang++

echo "$TARGETS" | grep -v '^[[:blank:]]*$' | while read i ; do
    platform=$(echo "$i" | cut -d' ' -f1)
    abi=$(echo "$i" | cut -d' ' -f2)
    ./bootstrap/bootstrap.gaussianblur.sh ${platform} ${abi}
done

if [ -d $(git_root)/build/temporary ] ; then rm -rf $(git_root)/build/temporary ; fi
