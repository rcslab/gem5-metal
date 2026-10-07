FROM fedora:44

ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

RUN dnf -y upgrade --refresh && \
    dnf -y install \
        \
        # compiler toolchain
        clang \
        llvm \
        lld \
        make \
        git \
        m4 \
        pkgconf-pkg-config \
        \
        # python
        python3 \
        python3-devel \
        python3-scons \
        python3-pydot \
        python3-tkinter \
        \
        # gem5 libraries
        zlib-ng-compat-devel \
        protobuf \
        protobuf-devel \
        protobuf-compiler \
        boost-devel \
        gperftools-devel \
        libpng-devel \
        elfutils-libelf-devel \
    && dnf clean all \
    && rm -rf /var/cache/dnf


RUN mkdir /artifacts

# clone sources
COPY gem5-metal /artifacts/gem5-metal
COPY copper /artifacts/copper

# copy scripts
COPY --chmod=0755 \
    gem5-metal/docker/build_copper_arm64.sh \
    gem5-metal/docker/build_copper_cobalt.sh \
    gem5-metal/docker/build_gem5.sh \
    gem5-metal/docker/run.sh \
    gem5-metal/docker/attach.sh \
    /artifacts/

WORKDIR /artifacts
CMD ["sleep", "infinity"]
