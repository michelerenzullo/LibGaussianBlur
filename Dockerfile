FROM debian:bookworm-slim AS base

# Install dependencies
RUN apt-get -qq update; \
    apt-get install -qqy --no-install-recommends \
        gnupg2 wget ca-certificates apt-transport-https curl unzip make cmake \
        clang-16 clang-tidy-16 clang-format-16 lld-16 xz-utils


# Install LLVM
RUN echo "deb https://apt.llvm.org/bookworm llvm-toolchain-bookworm-16 main" \
        > /etc/apt/sources.list.d/llvm.list && \
    wget -qO /etc/apt/trusted.gpg.d/llvm.asc \
        https://apt.llvm.org/llvm-snapshot.gpg.key && \
    apt-get -qq update && \
    apt-get install -qqy -t llvm-toolchain-bookworm-16 clang-16 clang-tidy-16 clang-format-16 lld-16 && \
    for f in /usr/lib/llvm-16/bin/*; do ln -sf "$f" /usr/bin; done && \
    rm -rf /var/lib/apt/lists/*

FROM base AS builder-env
WORKDIR /app
# Copy gaussianblur git submodule (without .git subfolder, ignored in .dockerignore)
COPY .deps/gaussian_blur .deps/gaussian_blur
# Bootstrap must be top-level and find gaussian_blur in .deps. Proably need to change this.
RUN ln -s /app/.deps/gaussian_blur/bootstrap /app/bootstrap

FROM builder-env AS linux
# Build gaussian_blur
RUN bootstrap/bootstrap.sh linux && rm -rf build
RUN ln -s /app/external/linux/x86_64/bin/GaussianBlurTests /app/GaussianBlurTests

FROM builder-env AS android
# Install OpenJDK 21
COPY .docker/openjdk.tar.gz /opt/
RUN mkdir -p /usr/lib/jvm/jdk && \
    tar -xzf /opt/openjdk.tar.gz --strip-components=1 -C /usr/lib/jvm/jdk && \
    rm /opt/openjdk.tar.gz && \
    ln -s /usr/lib/jvm/jdk/bin/* /usr/bin/

# Set JAVA_HOME environment variable
ENV JAVA_HOME=/usr/lib/jvm/jdk
ENV PATH="${JAVA_HOME}/bin:${PATH}"

# Install Android tools
COPY .docker/commandlinetools-linux.zip /opt/
# Set up android SDK and platform tools environment
ENV ANDROID_HOME=/opt/android-sdk-linux
ENV ANDROID_SDK_ROOT=$ANDROID_HOME
RUN mkdir -p $ANDROID_HOME/cmdline-tools/
RUN unzip /opt/commandlinetools-linux.zip -d $ANDROID_HOME/cmdline-tools/
ENV PATH="${ANDROID_HOME}/cmdline-tools/cmdline-tools/bin:${ANDROID_HOME}/platform-tools:${ANDROID_HOME}/emulator:${PATH}"
RUN mkdir -p ${HOME}/.android && touch ${HOME}/.android/repositories.cfg
# Install android SDK and remaining tools/images
RUN yes | sdkmanager  --licenses
RUN yes | sdkmanager "platform-tools"
RUN yes | sdkmanager --update --channel=3
RUN yes | sdkmanager --channel=3 "platforms;android-34"
RUN yes | sdkmanager --channel=3 "build-tools;34.0.0"
RUN yes | sdkmanager --channel=3 "cmake;3.22.1"
RUN yes | sdkmanager --channel=3 "ndk;27.0.12077973"
RUN yes | sdkmanager  --licenses
RUN bootstrap/bootstrap.sh android && rm -rf build


FROM builder-env AS wasm
# Install Emscripten
COPY .docker/emsdk.zip /opt/
# Extract emsdk and create in symlink in /root (aka $HOME)
ENV EMSDK=/opt/emsdk
RUN unzip /opt/emsdk.zip -d /opt/ && mv /opt/emsdk-* /opt/emsdk && rm /opt/emsdk.zip && ln -s /opt/emsdk /root/emsdk
# Add emscripten to PATH and install latest
ENV PATH="${EMSDK}:${EMSDK}/upstream/emscripten:${PATH}"
RUN ${EMSDK}/emsdk install latest
RUN ${EMSDK}/emsdk activate latest
RUN bootstrap/bootstrap.sh wasm && rm -rf build
