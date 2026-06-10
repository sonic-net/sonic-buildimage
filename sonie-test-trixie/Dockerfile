FROM debian:trixie
RUN apt-get update && \
    apt-get install -y --fix-missing \
        bridge-utils \
        curl \
        dnsmasq \
        dosfstools \
        ethtool \
        gdisk \
        gettext \
        iproute2 \
        iputils-ping \
        libcbor0.10 \
        libfido2-1 \
        libxmuu1 \
        mtools \
        net-tools \
        openssh-client \
        ovmf \
        procps \
        python3 \
        sshpass \
        python3-portpicker \
        python3-virt-firmware \
        qemu-system-x86 \
        swtpm \
        swtpm-tools \
        systemd-ukify \
        tcpdump && \
    rm -rf /var/lib/apt/lists/*

# Copy runner scripts and templates
COPY sonie-test-trixie/qemu_runner.sh /usr/local/bin/qemu_runner.sh
COPY sonie-test-trixie/http_server_progress.py /usr/local/bin/http_server_progress.py
COPY sonie-test-trixie/templates /etc/qemu-runner/templates

RUN chmod +x /usr/local/bin/qemu_runner.sh
