#!/bin/sh


. $(dirname $0)/common.sh

PREFIX="gaussian_blur"
TEMP_EXTERNAL_BUILD_DIR=.deps
GAUSSIANBLUR_VERSION="1.0.1"
GAUSSIANBLUR_FOLDER="v${GAUSSIANBLUR_VERSION}"
GAUSSIANBLUR_BUILD_DIR="${PREFIX}-${GAUSSIANBLUR_FOLDER}"

download_gaussian_blur_source() {
    printf "${GREEN}Downloading gaussian_blur source and unpacking to ${GAUSSIANBLUR_BUILD_DIR} directory:${RESET}\n"
    if [ -d ${GAUSSIANBLUR_BUILD_DIR} ] ; then
        printf "${YELLOW}Source code seems to be present. Skipping${RESET}\n" ; return
    fi
    # If we are in a docker container skip because the source code has been already copied in the image.
    if ! is_docker_container; then
        git clone -c advice.detachedHead=false --depth 1 --branch $GAUSSIANBLUR_VERSION https://github.com/michelerenzullo/LibGaussianBlur.git $PREFIX
        git -C $PREFIX submodule update --init --recursive
    fi
    cp -R $PREFIX $GAUSSIANBLUR_BUILD_DIR
    rm -rf "$GAUSSIANBLUR_BUILD_DIR/.git"
}

#1: Platform
#2: ABI
compile_gaussian_blur() {
    PLATFORM=$1
    # Default ABI for iOS is always arm64, while wasm is always wasm32.
    if [ "$PLATFORM" = "ios" ]; then ABI="arm64"; else ABI=$2; fi
    if [ "$PLATFORM" = "wasm" ]; then ABI="wasm32"; else ABI=$2; fi
    TEMP_PREFIX_DIR=$(git_root)/build/temporary/$PLATFORM/$ABI
    FINAL_PREFIX_DIR=$(git_root)/external/$PLATFORM/$ABI
    N_CPU_CORES=$(get_cpu_cores)

    printf "${GREEN}Compiling GAUSSIANBLUR code and place built files under $TEMP_PREFIX_DIR: ${RESET}\n"
    cd $(git_root)/${TEMP_EXTERNAL_BUILD_DIR}/${GAUSSIANBLUR_BUILD_DIR}
    if [ -d build ] ; then rm -rf build ; fi
    mkdir -p build && cd build

    if [ "$PLATFORM" = "android" ] ; then
        if [ ! -d "$ANDROID_HOME" ] ; then
            printf "${RED}ANDROID_HOME environment not valid.${RESET}\n" 1>&2 ; exit 1
        fi
        NDK="$ANDROID_HOME/ndk/27.0.12077973"
        CMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake
        CMAKE_MAKE_PROGRAM=$(find $ANDROID_HOME/cmake -name ninja -type f)
        cmake -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_MULTITHREADING=ON \
            -DWITH_EXAMPLES=OFF \
            -DCMAKE_MAKE_PROGRAM=$CMAKE_MAKE_PROGRAM \
            -GNinja \
            -DCMAKE_INSTALL_PREFIX="$TEMP_PREFIX_DIR" \
            -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
            -DANDROID_PLATFORM=23 \
            -DANDROID_ABI=$ABI \
            -DBUILD_SHARED_LIBS=ON \
            -DWITH_TESTS=OFF \
            -DANDROID_STL="c++_shared" ..
        printf "${GREEN}Compiling with ${N_CPU_CORES} cores. This might still take some time \n"
        nice cmake --build . --config Release -j $N_CPU_CORES
        cmake --install . --config Release

        mkdir -p "$FINAL_PREFIX_DIR/lib"
        mkdir -p "$FINAL_PREFIX_DIR/include"

        if [ -d ${FINAL_PREFIX_DIR}/include/gaussianblur ] ; then rm -rf ${FINAL_PREFIX_DIR}/include/gaussianblur ; fi
        mv ${TEMP_PREFIX_DIR}/include/gaussianblur ${FINAL_PREFIX_DIR}/include/
        mv ${TEMP_PREFIX_DIR}/lib/*.so* ${FINAL_PREFIX_DIR}/lib/
    elif [ "$PLATFORM" = "linux" ] ; then
        cmake -DENABLE_MULTITHREADING=ON \
            -DWITH_EXAMPLES=ON \
            -DCMAKE_INSTALL_PREFIX="$TEMP_PREFIX_DIR" \
            -DCMAKE_INSTALL_RPATH=$FINAL_PREFIX_DIR/lib \
            -DBUILD_SHARED_LIBS=ON \
            -DWITH_TESTS=ON \
            ..

        printf "${GREEN}Compiling with ${N_CPU_CORES} cores. This might still take some time\n"
        nice cmake --build . --config Release -j $N_CPU_CORES
        cmake --install . --config Release

        mkdir -p "$FINAL_PREFIX_DIR/lib"
        mkdir -p "$FINAL_PREFIX_DIR/include"
        mkdir -p "$FINAL_PREFIX_DIR/bin"

        if [ -d ${FINAL_PREFIX_DIR}/include/gaussianblur ] ; then rm -rf ${FINAL_PREFIX_DIR}/include/gaussianblur ; fi
        mv ${TEMP_PREFIX_DIR}/include/gaussianblur ${FINAL_PREFIX_DIR}/include/
        mv ${TEMP_PREFIX_DIR}/lib/*.so* ${FINAL_PREFIX_DIR}/lib/
        mv ${TEMP_PREFIX_DIR}/bin/* ${FINAL_PREFIX_DIR}/bin/
    elif [ "$PLATFORM" = "wasm" ] ; then
        cmake -DENABLE_MULTITHREADING=ON \
            -DCMAKE_INSTALL_PREFIX="$TEMP_PREFIX_DIR" \
            -DCMAKE_INSTALL_RPATH=$FINAL_PREFIX_DIR/lib \
            -DBUILD_SHARED_LIBS=OFF \
            -DWASM=ON \
            -DWITH_EXAMPLES=ON \
            -DWITH_TESTS=OFF \
            -DCMAKE_TOOLCHAIN_FILE="$HOME/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
            ..

        printf "${GREEN}Compiling with ${N_CPU_CORES} cores. This might still take some time\n"
        nice cmake --build . -j $N_CPU_CORES
        cmake --install .

        mkdir -p "$FINAL_PREFIX_DIR/lib"
        mkdir -p "$FINAL_PREFIX_DIR/include"
        mkdir -p "$FINAL_PREFIX_DIR/bin"

        if [ -d "${FINAL_PREFIX_DIR}/include/gaussianblur" ] ; then rm -rf "${FINAL_PREFIX_DIR}/include/gaussianblur" ; fi
        mv ${TEMP_PREFIX_DIR}/include/gaussianblur ${FINAL_PREFIX_DIR}/include/
        mv ${TEMP_PREFIX_DIR}/lib/*.a* ${FINAL_PREFIX_DIR}/lib/
        mv ${TEMP_PREFIX_DIR}/bin/* ${FINAL_PREFIX_DIR}/bin/
        cp ../examples/wasm/multi-thread/gaussianblur.html ${FINAL_PREFIX_DIR}/bin/
        ######## Find the closure compiled Module var name to set up in worker.js in order to load the wasm module
        GREP_CMD="grep"
        SED_CMD="sed -i"
        if command -v ggrep; then
            GREP_CMD="ggrep"
            SED_CMD="sed -i ''"
        fi
        MODULE_NAME=$($GREP_CMD -oP '\b[a-zA-Z]\b(?=[^a-zA-Z]*typeof Module(?![\s\S]*typeof Module))' "${FINAL_PREFIX_DIR}/bin/GaussianBlur.js")
        if [ -z "$MODULE_NAME" ]; then
            printf "${RED}Error: Unable to find the Module value in GaussianBlur.js${RESET}\n"
            exit 1
        fi
        cp ../examples/wasm/multi-thread/gaussianblur_worker.js ${FINAL_PREFIX_DIR}/bin/
        $SED_CMD "s|const Module = h|const Module = "${MODULE_NAME}";|" "${FINAL_PREFIX_DIR}/bin/gaussianblur_worker.js"
        printf "${GREEN}Module var name closure compiled is '${MODULE_NAME}', succesfully set in ${FINAL_PREFIX_DIR}/bin/gaussianblur_worker.js${RESET}\n"
        ####################
    elif [ "$PLATFORM" = "ios" ] ; then
        # Framework files
        FRAMEWORK_FILES="$(git_root)/bootstrap/gaussianblur_framework"
        GAUSSIANBLUR_FRAMEWORK="Gaussianblur.framework"
        #DEV_TEAM="your_dev_team_id" # Set your dev team id to sign iOS build
        DEPLOYMENT_TARGET="14.3"
        # This should not be necessary, but had trouble after upgrading the mac to Monterey.
        SDK_PATH="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
        if [ -z "$DEV_TEAM"]; then
            printf "${RED}DEV_TEAM environment variable not set.${RESET}\n" 1>&2 ; exit 1
        fi
        printf "${GREEN}Using iOS DEV_TEAM: $DEV_TEAM${RESET}\n"
    
        IOS_TOOLCHAIN="ios-cmake-4.4.0/ios.toolchain.cmake"
        if [ ! -f "$IOS_TOOLCHAIN" ] ; then
            TOOLCHAIN_ZIP="ios-cmake-4.4.0.zip"
            curl -o "$TOOLCHAIN_ZIP" --location "https://github.com/leetal/ios-cmake/archive/refs/tags/4.4.0.zip"
            unzip "$TOOLCHAIN_ZIP" "$IOS_TOOLCHAIN"
            rm "$TOOLCHAIN_ZIP"
        fi
        BUNDLE_ID=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" ${FRAMEWORK_FILES}/Resources/Info.plist)
        cmake -GXcode \
            -DPLATFORM="OS64" \
            -DARCHS="$ABI" \
            -DENABLE_BITCODE=OFF \
            -DDEPLOYMENT_TARGET="$DEPLOYMENT_TARGET" \
            -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
            -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="$DEV_TEAM" \
            -DCMAKE_INSTALL_PREFIX="$TEMP_PREFIX_DIR" \
            -DCMAKE_TOOLCHAIN_FILE="$IOS_TOOLCHAIN" \
            -DENABLE_MULTITHREADING=ON \
            -DWITH_EXAMPLES=OFF \
            -DWITH_TESTS=OFF \
            -DCMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER="$BUNDLE_ID" \
            ..
        printf "${GREEN}Compiling with ${N_CPU_CORES} cores. This might still take some time\n"
        nice cmake --build . --config Release -- -allowProvisioningUpdates -j $N_CPU_CORES
        cmake --install . --config Release

        mkdir -p "$FINAL_PREFIX_DIR/lib"
        mkdir -p "$FINAL_PREFIX_DIR/include"

        # Copy Headers and Libs
        if [ -d ${FINAL_PREFIX_DIR}/include/gaussianblur ] ; then rm -rf ${FINAL_PREFIX_DIR}/include/gaussianblur ; fi
        cp -R ${TEMP_PREFIX_DIR}/include/gaussianblur ${FINAL_PREFIX_DIR}/include/gaussianblur
        cp "$TEMP_PREFIX_DIR/lib/libGaussianblur.a" "${FINAL_PREFIX_DIR}/lib/libGaussianblur.a"

        # Create Framework
        if [ -d "$FINAL_PREFIX_DIR/$GAUSSIANBLUR_FRAMEWORK" ] ; then rm -rf "$FINAL_PREFIX_DIR/$GAUSSIANBLUR_FRAMEWORK" ; fi
        latest_framework="$FINAL_PREFIX_DIR/$GAUSSIANBLUR_FRAMEWORK/Versions/A"
        mkdir -p "$latest_framework/Headers"
        cp -R "$FRAMEWORK_FILES/Resources" "$latest_framework"
        mv ${TEMP_PREFIX_DIR}/include/gaussianblur/* "$latest_framework/Headers"
        mv "$TEMP_PREFIX_DIR/lib/libGaussianblur.a" "$latest_framework/Gaussianblur"

        # Create symlinks
        (cd $FINAL_PREFIX_DIR/$GAUSSIANBLUR_FRAMEWORK/Versions && ln -s "A" "Current")
        (cd $FINAL_PREFIX_DIR/$GAUSSIANBLUR_FRAMEWORK/ \
            && ln -s "Versions/Current/Headers" "Headers" \
            && ln -s "Versions/Current/Resources" "Resources" \
            && ln -s "Versions/Current/Gaussianblur" "Gaussianblur")
    else
        # MacOS arm64
        cmake -DENABLE_MULTITHREADING=ON \
            -DWITH_EXAMPLES=ON \
            -DCMAKE_INSTALL_PREFIX=${TEMP_PREFIX_DIR} \
            -DCMAKE_INSTALL_RPATH=$FINAL_PREFIX_DIR/lib \
            -DBUILD_SHARED_LIBS=ON \
            -DWITH_TESTS=ON \
            ..
        printf "${GREEN}Compiling with ${N_CPU_CORES} cores. This might still take some time\n"
        nice cmake --build . --config Release -j $N_CPU_CORES
        cmake --install . --config Release

        mkdir -p "$FINAL_PREFIX_DIR/lib"
        mkdir -p "$FINAL_PREFIX_DIR/include"
        mkdir -p "$FINAL_PREFIX_DIR/bin"

        if [ -d ${FINAL_PREFIX_DIR}/include/gaussianblur ] ; then rm -rf ${FINAL_PREFIX_DIR}/include/gaussianblur ; fi
        mv ${TEMP_PREFIX_DIR}/include/gaussianblur ${FINAL_PREFIX_DIR}/include/
        mv ${TEMP_PREFIX_DIR}/lib/*.dylib ${FINAL_PREFIX_DIR}/lib/
        mv ${TEMP_PREFIX_DIR}/bin/* ${FINAL_PREFIX_DIR}/bin/
    fi
}

init_into_build_dir
download_gaussian_blur_source
compile_gaussian_blur ${1:-linux} ${2:-x86_64}
