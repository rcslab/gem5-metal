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
RUN <<EOF
set -euo pipefail
cd /artifacts
git clone --single-branch --depth 1 --branch master https://github.com/rcslab/copper.git
git clone --single-branch --depth 1 --branch v23.1.0.0-metal https://github.com/rcslab/gem5-metal.git
EOF

# copy scripts
COPY --chmod=0755 \
    docker/build_copper_arm64.sh \
    docker/build_copper_cobalt.sh \
    docker/build_gem5.sh \
    docker/run.sh \
    docker/attach.sh \
    /artifacts/

WORKDIR /artifacts
CMD ["sleep", "infinity"]
