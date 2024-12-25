FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh




# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>


RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    build-essential cmake pkg-config \
    libzmq3-dev libssl-dev libunbound-dev libsodium-dev libunwind8-dev \
    liblzma-dev libreadline-dev libldns-dev libexpat1-dev libpgm-dev \
    qttools5-dev-tools libhidapi-dev libusb-1.0-0-dev libprotobuf-dev \
    protobuf-compiler libudev-dev libboost-chrono-dev libboost-date-time-dev \
    libboost-filesystem-dev libboost-locale-dev libboost-program-options-dev \
    libboost-regex-dev libboost-serialization-dev libboost-system-dev \
    libboost-thread-dev ccache doxygen graphviz

RUN apt-get install libgtest-dev -y && \
    cd /usr/src/gtest && \
    cmake . && \
    make -j4 && \
    mv lib/libg* /usr/lib/
