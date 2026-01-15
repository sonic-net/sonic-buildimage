FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    qemu-system-x86 \
    gettext-base \
    dnsmasq \
    python3 \
    python3-requests \
    iproute2 \
    dosfstools \
    mtools \
    gdisk \
    tcpdump \
    swtpm \
    swtpm-tools \
    python3-virt-firmware \
    && rm -rf /var/lib/apt/lists/*

COPY qemu_runner.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/qemu_runner.sh

COPY http_server_progress.py /usr/local/bin/
# Ensure templates dir exists
RUN mkdir -p /etc/qemu-runner/templates
COPY templates/* /etc/qemu-runner/templates/
