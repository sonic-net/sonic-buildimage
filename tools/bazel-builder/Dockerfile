FROM publicmirror.azurecr.io/debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        curl ca-certificates git make python3 python3-pip python3-yaml patch \
        build-essential pkg-config zip unzip xz-utils \
        clang llvm llvm-dev \
        flex bison \
        libicu-dev libncurses-dev libpcre2-dev libxml2-dev \
        aspell aspell-en \
        sudo gnupg lsb-release \
    && rm -rf /var/lib/apt/lists/*

# docker CLI (no daemon)
RUN install -m 0755 -d /etc/apt/keyrings && \
    curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg && \
    chmod a+r /etc/apt/keyrings/docker.gpg && \
    echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/debian bookworm stable" > /etc/apt/sources.list.d/docker.list && \
    apt-get update && apt-get install -y --no-install-recommends docker-ce-cli && \
    rm -rf /var/lib/apt/lists/*

# bazelisk (matches Bojun-Feng setup: v1.27.0)
RUN curl -fL -o /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/v1.27.0/bazelisk-linux-amd64 && \
    chmod +x /usr/local/bin/bazel

# Create matching user. UID/GID/docker-GID will be overridden at run time when needed.
RUN groupadd -g 1000 lunyue && useradd -m -u 1000 -g 1000 -s /bin/bash lunyue && \
    echo 'lunyue ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/lunyue
