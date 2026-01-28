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
        mtools \
        net-tools \
        openssh-client \
        ovmf \
        procps \
        python3 \
        python3-virt-firmware \
        qemu-system-x86 \
        swtpm \
        swtpm-tools \
        tcpdump && \
    rm -rf /var/lib/apt/lists/*

# Copy runner scripts and templates
COPY qemu_runner.sh /usr/local/bin/qemu_runner.sh
COPY http_server_progress.py /usr/local/bin/http_server_progress.py
COPY templates /etc/qemu-runner/templates

RUN chmod +x /usr/local/bin/qemu_runner.sh
