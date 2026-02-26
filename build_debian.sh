#!/bin/bash
## This script is to automate the preparation for a debian file system, which will be used for
## an ONIE installer image.
##
## USAGE:
##   USERNAME=username PASSWORD=password ./build_debian
## ENVIRONMENT:
##   USERNAME
##          The name of the default admin user
##   PASSWORD
##          The password, expected by chpasswd command

## Default user
[ -n "$USERNAME" ] || {
    echo "Error: no or empty USERNAME"
    exit 1
}

## Password for the default user
[ -n "$PASSWORD" ] || {
    echo "Error: no or empty PASSWORD"
    exit 1
}

## Include common functions
. functions.sh

## Enable debug output for script
set -x -e

CONFIGURED_ARCH=$([ -f .arch ] && cat .arch || echo amd64)

## docker engine version (with platform)
DOCKER_VERSION=5:28.2.2-1~debian.13~$IMAGE_DISTRO
CONTAINERD_IO_VERSION=1.7.27-1
LINUX_KERNEL_VERSION=6.12.41+deb13

## Working directory to prepare the file system
FILESYSTEM_ROOT=./fsroot
PLATFORM_DIR=platform
## Hostname for the linux image
HOSTNAME=sonic
DEFAULT_USERINFO="Default admin user,,,"
BUILD_TOOL_PATH=src/sonic-build-hooks/buildinfo
TRUSTED_GPG_DIR=$BUILD_TOOL_PATH/trusted.gpg.d

## Read ONIE image related config file
. ./onie-image.conf
[ -n "$ONIE_IMAGE_PART_SIZE" ] || {
    echo "Error: Invalid ONIE_IMAGE_PART_SIZE in onie image config file"
    exit 1
}
[ -n "$INSTALLER_PAYLOAD" ] || {
    echo "Error: Invalid INSTALLER_PAYLOAD in onie image config file"
    exit 1
}
[ -n "$FILESYSTEM_SQUASHFS" ] || {
    echo "Error: Invalid FILESYSTEM_SQUASHFS in onie image config file"
    exit 1
}

if [ "$IMAGE_TYPE" = "aboot" ]; then
    TARGET_BOOTLOADER="aboot"
fi

## Check if not a last stage of RFS build
if [[ $RFS_SPLIT_LAST_STAGE != y ]]; then

## Prepare the file system directory
if [[ -d $FILESYSTEM_ROOT ]]; then
    sudo rm -rf $FILESYSTEM_ROOT || die "Failed to clean chroot directory"
fi
mkdir -p $FILESYSTEM_ROOT
mkdir -p $FILESYSTEM_ROOT/$PLATFORM_DIR
touch $FILESYSTEM_ROOT/$PLATFORM_DIR/firsttime

bootloader_packages=""
if [ "$TARGET_BOOTLOADER" != "aboot" ]; then
    mkdir -p $FILESYSTEM_ROOT/$PLATFORM_DIR/grub
    bootloader_packages="grub2-common"
fi

## ensure proc is mounted
sudo mount proc /proc -t proc || true

## Build the host debian base system
echo '[INFO] Build host debian base system...'
TARGET_PATH=$TARGET_PATH scripts/build_debian_base_system.sh $CONFIGURED_ARCH $IMAGE_DISTRO $FILESYSTEM_ROOT $http_proxy

# Prepare buildinfo
sudo SONIC_VERSION_CACHE=${SONIC_VERSION_CACHE} \
	DBGOPT="${DBGOPT}" \
	scripts/prepare_debian_image_buildinfo.sh $CONFIGURED_ARCH $IMAGE_DISTRO $FILESYSTEM_ROOT $http_proxy


sudo chown root:root $FILESYSTEM_ROOT

## Config hostname and hosts, otherwise 'sudo ...' will complain 'sudo: unable to resolve host ...'
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '$HOSTNAME' > /etc/hostname"
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '127.0.0.1       $HOSTNAME' >> /etc/hosts"
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '127.0.0.1       localhost' >> /etc/hosts"

## Config basic fstab
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c 'echo "proc /proc proc defaults 0 0" >> /etc/fstab'
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c 'echo "sysfs /sys sysfs defaults 0 0" >> /etc/fstab'

## Setup proxy
[ -n "$http_proxy" ] && sudo /bin/bash -c "echo 'Acquire::http::Proxy \"$http_proxy\";' > $FILESYSTEM_ROOT/etc/apt/apt.conf.d/01proxy"

trap_push 'sudo LANG=C chroot $FILESYSTEM_ROOT umount /proc || true'
sudo LANG=C chroot $FILESYSTEM_ROOT mount proc /proc -t proc
## Note: mounting is necessary to makedev and install linux image
echo '[INFO] Mount all'
## Output all the mounted device for troubleshooting
sudo LANG=C chroot $FILESYSTEM_ROOT mount

## Install the trusted gpg public keys
[ -d $TRUSTED_GPG_DIR ] && [ ! -z "$(ls $TRUSTED_GPG_DIR)" ] && sudo cp $TRUSTED_GPG_DIR/* ${FILESYSTEM_ROOT}/etc/apt/trusted.gpg.d/

## Pointing apt to public apt mirrors and getting latest packages, needed for latest security updates
scripts/build_mirror_config.sh files/apt $CONFIGURED_ARCH $IMAGE_DISTRO
sudo cp files/apt/sources.list.$CONFIGURED_ARCH $FILESYSTEM_ROOT/etc/apt/sources.list
sudo cp files/apt/apt-retries-count $FILESYSTEM_ROOT/etc/apt/apt.conf.d/
sudo cp files/apt/apt.conf.d/{81norecommends,apt-{clean,gzip-indexes,no-languages},no-check-valid-until} $FILESYSTEM_ROOT/etc/apt/apt.conf.d/

## Note: set lang to prevent locale warnings in your chroot
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y update
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y upgrade

echo '[INFO] Install and setup eatmydata'
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install eatmydata
sudo LANG=C chroot $FILESYSTEM_ROOT ln -s /usr/bin/eatmydata /usr/local/bin/dpkg
echo 'Dir::Bin::dpkg "/usr/local/bin/dpkg";' | sudo tee $FILESYSTEM_ROOT/etc/apt/apt.conf.d/00image-install-eatmydata > /dev/null
## Note: dpkg hook conflict with eatmydata
sudo LANG=C chroot $FILESYSTEM_ROOT rm /usr/local/sbin/dpkg -f

echo '[INFO] Install packages for building image'
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install psmisc

if [[ $CROSS_BUILD_ENVIRON == y ]]; then
    sudo LANG=C chroot $FILESYSTEM_ROOT dpkg --add-architecture $CONFIGURED_ARCH
fi

## docker and mkinitramfs on target system will use pigz/unpigz automatically
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install pigz

## Install initramfs-tools and linux kernel
## Note: initramfs-tools recommends depending on busybox, and we really want busybox for
## 1. commands such as touch
## 2. mount supports squashfs
## However, 'dpkg -i' plus 'apt-get install -f' will ignore the recommended dependency. So
## we install busybox explicitly
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install busybox linux-base
echo '[INFO] Install SONiC linux kernel image'
## Note: duplicate apt-get command to ensure every line return zero
sudo cp $debs_path/initramfs-tools-core_*.deb $debs_path/initramfs-tools_*.deb $debs_path/linux-image-${LINUX_KERNEL_VERSION}-*_${CONFIGURED_ARCH}.deb $FILESYSTEM_ROOT
basename_deb_packages=$(basename -a $debs_path/initramfs-tools-core_*.deb $debs_path/initramfs-tools_*.deb $debs_path/linux-image-${LINUX_KERNEL_VERSION}-*_${CONFIGURED_ARCH}.deb | sed 's,^,./,')
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt -y install $basename_deb_packages
( cd $FILESYSTEM_ROOT; sudo rm -f $basename_deb_packages )
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install acl
if [[ $CONFIGURED_ARCH == amd64 ]]; then
    sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install hdparm
fi

## Update initramfs for booting with squashfs+overlay
cat files/initramfs-tools/modules | sudo tee -a $FILESYSTEM_ROOT/etc/initramfs-tools/modules > /dev/null

## Install kbuild for sign-file into docker image (not fsroot)
sudo LANG=C DEBIAN_FRONTEND=noninteractive apt -y --allow-downgrades install ./$debs_path/linux-kbuild-${LINUX_KERNEL_VERSION}*_${CONFIGURED_ARCH}.deb

## Hook into initramfs: change fs type from vfat to ext4 on arista switches
sudo mkdir -p $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/
sudo cp files/initramfs-tools/arista-convertfs $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/arista-convertfs
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/arista-convertfs
sudo cp files/initramfs-tools/arista-hook $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/arista-hook
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/arista-hook
sudo cp files/initramfs-tools/mke2fs $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/mke2fs
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/mke2fs
sudo cp files/initramfs-tools/setfacl $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/setfacl
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/setfacl

# Hook into initramfs: rename the management interfaces on arista switches
sudo cp files/initramfs-tools/arista-net $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/arista-net
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/arista-net

# Hook into initramfs: resize root partition after migration from another NOS to SONiC on Dell switches
sudo cp files/initramfs-tools/resize-rootfs $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/resize-rootfs
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/resize-rootfs

# Hook into initramfs: upgrade SSD from initramfs
sudo cp files/initramfs-tools/ssd-upgrade $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/ssd-upgrade
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/ssd-upgrade

# Hook into initramfs: run fsck to repair a non-clean filesystem prior to be mounted
sudo cp files/initramfs-tools/fsck-rootfs $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/fsck-rootfs
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-premount/fsck-rootfs

## Hook into initramfs: after partition mount and loop file mount
## 1. Prepare layered file system
## 2. Bind-mount docker working directory (docker overlay storage cannot work over overlay rootfs)
sudo cp files/initramfs-tools/union-mount $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-bottom/union-mount
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-bottom/union-mount
sudo cp files/initramfs-tools/varlog $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-bottom/varlog
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-bottom/varlog
# Management interface (eth0) dhcp can be optionally turned off (during a migration from another NOS to SONiC)
#sudo cp files/initramfs-tools/mgmt-intf-dhcp $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-bottom/mgmt-intf-dhcp
#sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/scripts/init-bottom/mgmt-intf-dhcp
sudo cp files/initramfs-tools/union-fsck $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/union-fsck
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/union-fsck
pushd $FILESYSTEM_ROOT/usr/share/initramfs-tools/scripts/init-bottom && sudo patch -p1 < $OLDPWD/files/initramfs-tools/udev.patch; popd
if [[ $CONFIGURED_ARCH == armhf || $CONFIGURED_ARCH == arm64 ]]; then
    sudo cp files/initramfs-tools/uboot-utils $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/uboot-utils
    sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/uboot-utils
    cat files/initramfs-tools/modules.arm | sudo tee -a $FILESYSTEM_ROOT/etc/initramfs-tools/modules > /dev/null
fi
# Update initramfs for load platform specific modules
if [ -f platform/$CONFIGURED_PLATFORM/modules ]; then
    cat platform/$CONFIGURED_PLATFORM/modules | sudo tee -a $FILESYSTEM_ROOT/etc/initramfs-tools/modules > /dev/null
fi

## Add mtd and uboot firmware tools package
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install u-boot-tools libubootenv-tool mtd-utils device-tree-compiler

## Install docker
echo '[INFO] Install docker'
## Install apparmor utils since they're missing and apparmor is enabled in the kernel
## Otherwise Docker will fail to start
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install apparmor
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install apt-transport-https \
                                                       ca-certificates \
                                                       curl
if [[ $CONFIGURED_ARCH == armhf ]]; then
    # update ssl ca certificates for secure pem
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT c_rehash
fi
sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/docker.asc -fsSL https://download.docker.com/linux/debian/gpg
sudo LANG=C chroot $FILESYSTEM_ROOT mv /tmp/docker.asc /etc/apt/trusted.gpg.d/
sudo tee $FILESYSTEM_ROOT/etc/apt/sources.list.d/docker.list >/dev/null <<EOF
deb [arch=$CONFIGURED_ARCH] https://download.docker.com/linux/debian $IMAGE_DISTRO stable
EOF
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get update
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install docker-ce=${DOCKER_VERSION} docker-ce-cli=${DOCKER_VERSION} containerd.io=${CONTAINERD_IO_VERSION}

install_kubernetes () {
    local ver="$1"
    ## Install k8s package from storage
    local storage_prefix="https://packages.trafficmanager.net/public/kubernetes"
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/cri-tools.deb -fsSL \
        ${storage_prefix}/cri-tools_${KUBERNETES_CRI_TOOLS_VERSION}_${CONFIGURED_ARCH}.deb
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/kubernetes-cni.deb -fsSL \
        ${storage_prefix}/kubernetes-cni_${KUBERNETES_CNI_VERSION}_${CONFIGURED_ARCH}.deb
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/kubelet.deb -fsSL \
        ${storage_prefix}/kubelet_${ver}_${CONFIGURED_ARCH}.deb
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/kubectl.deb -fsSL \
        ${storage_prefix}/kubectl_${ver}_${CONFIGURED_ARCH}.deb
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/kubeadm.deb -fsSL \
        ${storage_prefix}/kubeadm_${ver}_${CONFIGURED_ARCH}.deb

    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install -f /tmp/cri-tools.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install -f /tmp/kubernetes-cni.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install -f /tmp/kubelet.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install -f /tmp/kubectl.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install -f /tmp/kubeadm.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT rm -f /tmp/{cri-tools,kubernetes-cni,kubelet,kubeadm,kubectl}.deb
}

if [ "$INCLUDE_KUBERNETES" == "y" ]
then
    ## Install Kubernetes
    echo '[INFO] Install kubernetes'
    install_kubernetes ${KUBERNETES_VERSION}
else
    echo '[INFO] Skipping Install kubernetes'
fi

if [ "$INCLUDE_KUBERNETES_MASTER" == "y" ]
then
    ## Install Kubernetes master
    echo '[INFO] Install kubernetes master'
    install_kubernetes ${MASTER_KUBERNETES_VERSION}

    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install hyperv-daemons xmlstarlet parted netcat-openbsd
    sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT curl -o /tmp/cri-dockerd.deb -fsSL \
        https://github.com/Mirantis/cri-dockerd/releases/download/v${MASTER_CRI_DOCKERD}/cri-dockerd_${MASTER_CRI_DOCKERD}.3-0.debian-${IMAGE_DISTRO}_amd64.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install -f /tmp/cri-dockerd.deb
    sudo LANG=C chroot $FILESYSTEM_ROOT rm -f /tmp/cri-dockerd.deb
else
    echo '[INFO] Skipping Install kubernetes master'
fi

## Add docker config drop-in to specify dockerd command line
sudo mkdir -p $FILESYSTEM_ROOT/etc/systemd/system/docker.service.d/
## Note: $_ means last argument of last command
sudo cp files/docker/docker.service.conf $_

## Create default user
## Note: user should be in the group with the same name, and also in sudo/docker/redis groups
sudo LANG=C chroot $FILESYSTEM_ROOT useradd -G sudo,docker $USERNAME -c "$DEFAULT_USERINFO" -m -s /bin/bash
## Create password for the default user
echo "$USERNAME:$PASSWORD" | sudo LANG=C chroot $FILESYSTEM_ROOT chpasswd

## Create redis group
sudo LANG=C chroot $FILESYSTEM_ROOT groupadd -f redis
sudo LANG=C chroot $FILESYSTEM_ROOT usermod -aG redis $USERNAME

if [[ $CONFIGURED_ARCH == amd64 ]]; then
    ## Pre-install hardware drivers
    sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y install      \
        firmware-linux-nonfree \
        firmware-intel-misc
fi

## Pre-install the fundamental packages
## Note: gdisk is needed for sgdisk in install.sh
## Note: parted is needed for partprobe in install.sh
## Note: ca-certificates is needed for easy_install
## Note: don't install python-apt by pip, older than Debian repo one
## Note: fdisk and gpg are needed by fwutil
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install      \
    file                    \
    ifmetric                \
    iproute2                \
    bridge-utils            \
    isc-dhcp-client         \
    sudo                    \
    vim                     \
    tcpdump                 \
    dbus                    \
    openssh-server          \
    python3-apt             \
    traceroute              \
    iputils-ping            \
    arping                  \
    net-tools               \
    bsdmainutils            \
    ca-certificates         \
    i2c-tools               \
    efibootmgr              \
    usbutils                \
    pciutils                \
    iptables-persistent     \
    ebtables                \
    linux-sysctl-defaults   \
    logrotate               \
    curl                    \
    kexec-tools             \
    less                    \
    unzip                   \
    fdisk                   \
    gdisk                   \
    sysfsutils              \
    e2fsprogs               \
    squashfs-tools          \
    $bootloader_packages    \
    rsyslog                 \
    screen                  \
    hping3                  \
    tcptraceroute           \
    mtr-tiny                \
    locales                 \
    cgroup-tools            \
    ipmitool                \
    ndisc6                  \
    makedumpfile            \
    conntrack               \
    python3                 \
    python3-pip             \
    python-is-python3       \
    cron                    \
    libprotobuf32t64        \
    libgrpc29t64            \
    libgrpc++1.51t64        \
    haveged                 \
    gpg                     \
    dmidecode               \
    jq                      \
    auditd                  \
    linux-perf              \
    resolvconf              \
    lsof                    \
    sysstat                 \
    xxd                     \
    wireless-regdb          \
    ethtool                 \
    zstd                    \
    nvme-cli

sudo cp files/initramfs-tools/pzstd $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/pzstd
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/pzstd

sudo cp files/initramfs-tools/file $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/file
sudo chmod +x $FILESYSTEM_ROOT/etc/initramfs-tools/hooks/file

# Have systemd create the auditd log directory
sudo mkdir -p ${FILESYSTEM_ROOT}/etc/systemd/system/auditd.service.d
sudo tee ${FILESYSTEM_ROOT}/etc/systemd/system/auditd.service.d/log-directory.conf >/dev/null <<EOF
[Service]
LogsDirectory=audit
LogsDirectoryMode=0750
EOF

# latest tcpdump control resource access with AppArmor.
# override tcpdump profile to allow tcpdump access TACACS config file.
sudo cp files/apparmor/usr.bin.tcpdump $FILESYSTEM_ROOT/etc/apparmor.d/local/usr.bin.tcpdump

## Set /etc/shadow permissions to -rw-------.
sudo LANG=c chroot $FILESYSTEM_ROOT chmod 600 /etc/shadow

## Set /etc/passwd, /etc/group permissions to -rw-r--r--.
sudo LANG=c chroot $FILESYSTEM_ROOT chmod 644 /etc/passwd
sudo LANG=c chroot $FILESYSTEM_ROOT chmod 644 /etc/group

# Needed to install kdump-tools
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "mkdir -p /etc/initramfs-tools/conf.d"
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo 'MODULES=most' >> /etc/initramfs-tools/conf.d/driver-policy"

# Ensure the relevant directories exist
sudo mkdir -p /etc/initramfs-tools/scripts/init-premount
sudo mkdir -p /etc/initramfs-tools/hooks

# Copy the network setup scriptgit
sudo cp files/scripts/network_setup.sh /etc/initramfs-tools/scripts/init-premount/network_setup.sh

# Copy the hook file
sudo cp files/scripts/network_setup /etc/initramfs-tools/hooks/network_setup

# Make the scripts executable
sudo chmod +x /etc/initramfs-tools/scripts/init-premount/network_setup.sh
sudo chmod +x /etc/initramfs-tools/hooks/network_setup

# Copy vmcore-sysctl.conf to add more vmcore dump flags to kernel
sudo cp files/image_config/kdump/vmcore-sysctl.conf $FILESYSTEM_ROOT/etc/sysctl.d/

#Adds a locale to a debian system in non-interactive mode
sudo sed -i '/^#.* en_US.* /s/^#//' $FILESYSTEM_ROOT/etc/locale.gen && \
    sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT locale-gen "en_US.UTF-8"
sudo LANG=en_US.UTF-8 DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT update-locale "LANG=en_US.UTF-8"
sudo LANG=C chroot $FILESYSTEM_ROOT bash -c "find /usr/share/i18n/locales/ ! -name 'en_US' -type f -exec rm -f {} +"

sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install \
    picocom \
    systemd \
    systemd-sysv \
    chrony

if [[ $TARGET_BOOTLOADER == grub ]]; then
	sudo cp $debs_path/grub-common*.deb $debs_path/grub2-common*.deb $FILESYSTEM_ROOT
	basename_deb_packages=$(basename -a $debs_path/grub-common*.deb $debs_path/grub2-common*.deb | sed 's,^,./,')
	sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt -y --allow-downgrades install $basename_deb_packages
	sudo rm $FILESYSTEM_ROOT/grub-common*.deb $FILESYSTEM_ROOT/grub2-common*.deb
	( cd $FILESYSTEM_ROOT; sudo rm -f $basename_deb_packages )

    if [[ $CONFIGURED_ARCH == amd64 ]]; then
        GRUB_PKG=grub-pc-bin
    elif [[ $CONFIGURED_ARCH == arm64 ]]; then
        GRUB_PKG=grub-efi-arm64-bin
    fi

    sudo cp $debs_path/${GRUB_PKG}*.deb $FILESYSTEM_ROOT/$PLATFORM_DIR/grub
fi

## Disable kexec supported reboot which was installed by default
sudo sed -i 's/LOAD_KEXEC=true/LOAD_KEXEC=false/' $FILESYSTEM_ROOT/etc/default/kexec

# Ensure that 'logrotate-config.service' is set as a dependency to start before 'logrotate.service'.
sudo mkdir $FILESYSTEM_ROOT/etc/systemd/system/logrotate.service.d
sudo cp files/image_config/logrotate/logrotateOverride.conf $FILESYSTEM_ROOT/etc/systemd/system/logrotate.service.d/logrotateOverride.conf

## Remove sshd host keys, and will regenerate on first sshd start
sudo rm -f $FILESYSTEM_ROOT/etc/ssh/ssh_host_*_key*
sudo cp files/sshd/host-ssh-keygen.sh $FILESYSTEM_ROOT/usr/local/bin/
sudo mkdir $FILESYSTEM_ROOT/etc/systemd/system/ssh.service.d
sudo cp files/sshd/override.conf $FILESYSTEM_ROOT/etc/systemd/system/ssh.service.d/override.conf
# Config sshd
# 1. Set 'UseDNS' to 'no'
# 2. Configure sshd to close all SSH connetions after 15 minutes of inactivity
sudo augtool -r $FILESYSTEM_ROOT <<'EOF'
touch /files/etc/ssh/sshd_config/EmptyLineHack
rename /files/etc/ssh/sshd_config/EmptyLineHack ""
set /files/etc/ssh/sshd_config/UseDNS no
ins #comment before /files/etc/ssh/sshd_config/UseDNS
set /files/etc/ssh/sshd_config/#comment[following-sibling::*[1][self::UseDNS]] "Disable hostname lookups"

rm /files/etc/ssh/sshd_config/ClientAliveInterval
rm /files/etc/ssh/sshd_config/ClientAliveCountMax
touch /files/etc/ssh/sshd_config/EmptyLineHack
rename /files/etc/ssh/sshd_config/EmptyLineHack ""
set /files/etc/ssh/sshd_config/ClientAliveInterval 300
set /files/etc/ssh/sshd_config/ClientAliveCountMax 0
ins #comment before /files/etc/ssh/sshd_config/ClientAliveInterval
set /files/etc/ssh/sshd_config/#comment[following-sibling::*[1][self::ClientAliveInterval]] "Close inactive client sessions after 5 minutes"
rm /files/etc/ssh/sshd_config/MaxAuthTries
set /files/etc/ssh/sshd_config/MaxAuthTries 3
rm /files/etc/ssh/sshd_config/Banner
set /files/etc/ssh/sshd_config/Banner /etc/issue
rm /files/etc/ssh/sshd_config/LogLevel
set /files/etc/ssh/sshd_config/LogLevel VERBOSE
save
quit
EOF
# Configure sshd to listen for v4 and v6 connections
sudo sed -i 's/^#ListenAddress 0.0.0.0/ListenAddress 0.0.0.0/' $FILESYSTEM_ROOT/etc/ssh/sshd_config
sudo sed -i 's/^#ListenAddress ::/ListenAddress ::/' $FILESYSTEM_ROOT/etc/ssh/sshd_config

# Use libpam_systemd, since that's now needed for limiting login sessions
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install libpam-systemd

## Config rsyslog
sudo augtool -r $FILESYSTEM_ROOT --autosave "
rm /files/lib/systemd/system/rsyslog.service/Service/ExecStart/arguments
set /files/lib/systemd/system/rsyslog.service/Service/ExecStart/arguments/1 -n
"

sudo mkdir -p $FILESYSTEM_ROOT/var/core

sudo cp files/image_config/sysctl/90-sonic.conf $FILESYSTEM_ROOT/usr/lib/sysctl.d/

sudo augtool --autosave "$sysctl_net_cmd_string" -r $FILESYSTEM_ROOT

# Specify that we want to explicitly install Python packages into the system environment, and risk breakages
sudo cp files/image_config/pip/pip.conf $FILESYSTEM_ROOT/etc/pip.conf

# For building Python packages
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install python3-setuptools python3-wheel

# docker Python API package is needed by Ansible docker module as well as some SONiC applications
sudo https_proxy=$https_proxy LANG=C chroot $FILESYSTEM_ROOT pip3 install 'docker==7.1.0'

# Install scapy
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install python3-scapy

## Note: keep pip installed for maintainance purpose

# Install GCC, needed for building/installing some Python packages
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install gcc

## Create /var/run/redis folder for docker-database to mount
sudo mkdir -p $FILESYSTEM_ROOT/var/run/redis

## Config DHCP for eth0
sudo tee -a $FILESYSTEM_ROOT/etc/network/interfaces > /dev/null <<EOF

auto eth0
allow-hotplug eth0
iface eth0 inet dhcp
EOF

sudo cp files/dhcp/rfc3442-classless-routes $FILESYSTEM_ROOT/etc/dhcp/dhclient-exit-hooks.d
sudo cp files/dhcp/sethostname $FILESYSTEM_ROOT/etc/dhcp/dhclient-exit-hooks.d/
sudo cp files/dhcp/sethostname6 $FILESYSTEM_ROOT/etc/dhcp/dhclient-exit-hooks.d/
sudo cp files/dhcp/graphserviceurl $FILESYSTEM_ROOT/etc/dhcp/dhclient-exit-hooks.d/
sudo cp files/dhcp/snmpcommunity $FILESYSTEM_ROOT/etc/dhcp/dhclient-exit-hooks.d/
sudo cp files/dhcp/vrf $FILESYSTEM_ROOT/etc/dhcp/dhclient-exit-hooks.d/

## Version file part 1
sudo mkdir -p $FILESYSTEM_ROOT/etc/sonic
if [ -f files/image_config/sonic_release ]; then
    sudo cp files/image_config/sonic_release $FILESYSTEM_ROOT/etc/sonic/
fi

# Default users info
export password_expire="$( [[ "$CHANGE_DEFAULT_PASSWORD" == "y" ]] && echo true || echo false )"
export username="${USERNAME}"
export password="$(sudo grep ^${USERNAME} $FILESYSTEM_ROOT/etc/shadow | cut -d: -f2)"
j2 files/build_templates/default_users.json.j2 | sudo tee $FILESYSTEM_ROOT/etc/sonic/default_users.json
sudo LANG=c chroot $FILESYSTEM_ROOT chmod 600 /etc/sonic/default_users.json
sudo LANG=c chroot $FILESYSTEM_ROOT chown root:shadow /etc/sonic/default_users.json

## Copy over clean-up script
sudo cp ./files/scripts/core_cleanup.py $FILESYSTEM_ROOT/usr/bin/core_cleanup.py

## Copy ASIC config checksum
sudo chmod 755 files/build_scripts/generate_asic_config_checksum.py
./files/build_scripts/generate_asic_config_checksum.py
if [[ ! -f './asic_config_checksum' ]]; then
    echo 'asic_config_checksum not found'
    exit 1
fi
sudo cp ./asic_config_checksum $FILESYSTEM_ROOT/etc/sonic/asic_config_checksum

## Check if not a last stage of RFS build
fi

if [[ $RFS_SPLIT_FIRST_STAGE == y ]]; then
    echo '[INFO] Finished with RFS first stage'
    echo '[INFO] Umount all'

    ## Display all process details access /proc
    sudo LANG=C chroot $FILESYSTEM_ROOT fuser -vm /proc
    ## Kill the processes
    sudo LANG=C chroot $FILESYSTEM_ROOT fuser -km /proc || true
    ## Wait fuser fully kill the processes
    sudo timeout 15s bash -c 'until LANG=C chroot $0 umount /proc; do sleep 1; done' $FILESYSTEM_ROOT || true

    sudo rm -f $TARGET_PATH/$RFS_SQUASHFS_NAME
    sudo mksquashfs $FILESYSTEM_ROOT $TARGET_PATH/$RFS_SQUASHFS_NAME -Xcompression-level 1

    exit 0
fi

if [[ $RFS_SPLIT_LAST_STAGE == y ]]; then
    echo '[INFO] RFS build: second stage'

    ## ensure proc is mounted
    sudo mount proc /proc -t proc || true

    sudo fuser -vm $FILESYSTEM_ROOT || true
    sudo rm -rf $FILESYSTEM_ROOT
    sudo unsquashfs -d $FILESYSTEM_ROOT $TARGET_PATH/$RFS_SQUASHFS_NAME

    ## make / as a mountpoint in chroot env, needed by dockerd
    pushd $FILESYSTEM_ROOT
    sudo mount --bind . .
    popd

    trap_push 'sudo LANG=C chroot $FILESYSTEM_ROOT umount /proc || true'
    sudo LANG=C chroot $FILESYSTEM_ROOT mount proc /proc -t proc
fi

## Version file part 2
export build_version="${SONIC_IMAGE_VERSION}"
export debian_version="$(cat $FILESYSTEM_ROOT/etc/debian_version)"
export kernel_version="${kversion}"
export asic_type="${sonic_asic_platform}"
export asic_subtype="${TARGET_MACHINE}"
export commit_id="$(git rev-parse --short HEAD)"
export branch="$(git rev-parse --abbrev-ref HEAD)"
export release="$(if [ -f $FILESYSTEM_ROOT/etc/sonic/sonic_release ]; then cat $FILESYSTEM_ROOT/etc/sonic/sonic_release; fi)"
export build_date="$(date -u)"
export build_number="${BUILD_NUMBER:-0}"
export built_by="$USER@$BUILD_HOSTNAME"
export sonic_os_version="${SONIC_OS_VERSION}"
j2 files/build_templates/sonic_version.yml.j2 | sudo tee $FILESYSTEM_ROOT/etc/sonic/sonic_version.yml

if [ -f sonic_debian_extension.sh ]; then
    ./sonic_debian_extension.sh $FILESYSTEM_ROOT $PLATFORM_DIR $IMAGE_DISTRO
fi

## Organization specific extensions such as Configuration & Scripts for features like AAA, ZTP...
if [ "${enable_organization_extensions}" = "y" ]; then
   if [ -f files/build_templates/organization_extensions.sh ]; then
      sudo chmod 755 files/build_templates/organization_extensions.sh
      ./files/build_templates/organization_extensions.sh -f $FILESYSTEM_ROOT -h $HOSTNAME
   fi
fi

## Setup ebtable rules (rule file in text format)
sudo cp files/image_config/ebtables/ebtables.filter.cfg ${FILESYSTEM_ROOT}/etc

## Debug Image specific changes
## Update motd for debug image
if [ "$DEBUG_IMG" == "y" ]
then
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '**************' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo 'Running DEBUG image' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '**************' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '/src has the sources' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '/src is mounted in each docker' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '/debug is created for core files or temp files' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo 'Create a subdir under /debug to upload your files' >> /etc/motd"
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo '/debug is mounted in each docker' >> /etc/motd"

    sudo mkdir -p $FILESYSTEM_ROOT/src
    sudo cp $DEBUG_SRC_ARCHIVE_FILE $FILESYSTEM_ROOT/src/
    sudo mkdir -p $FILESYSTEM_ROOT/debug

fi

## Set FIPS runtime default option
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "mkdir -p /etc/fips"
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo 0 > /etc/fips/fips_enable"

# #################
#   secure boot
# #################
if [[ $SECURE_UPGRADE_MODE == 'dev' || $SECURE_UPGRADE_MODE == "prod" ]]; then
    echo "Secure Boot support build stage: Starting .."

	sudo cp $debs_path/grub-efi*.deb $FILESYSTEM_ROOT
	basename_deb_packages=$(basename -a $debs_path/grub-efi*.deb | sed 's,^,./,')
	sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt -y --allow-downgrades install $basename_deb_packages
	sudo rm $FILESYSTEM_ROOT/grub-efi*.deb

    # debian secure boot dependecies
    sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install      \
        shim-unsigned

    if [ ! -f $SECURE_UPGRADE_SIGNING_CERT ]; then
        echo "Error: SONiC SECURE_UPGRADE_SIGNING_CERT=$SECURE_UPGRADE_SIGNING_CERT key missing"
        exit 1
    fi

    if [[ $SECURE_UPGRADE_MODE == 'dev' ]]; then
        # development signing & verification

        if [ ! -f $SECURE_UPGRADE_DEV_SIGNING_KEY ]; then
            echo "Error: SONiC SECURE_UPGRADE_DEV_SIGNING_KEY=$SECURE_UPGRADE_DEV_SIGNING_KEY key missing"
            exit 1
        fi

        sudo ./scripts/signing_secure_boot_dev.sh -a $CONFIGURED_ARCH \
                                                  -r $FILESYSTEM_ROOT \
                                                  -l $LINUX_KERNEL_VERSION \
                                                  -c $SECURE_UPGRADE_SIGNING_CERT \
                                                  -p $SECURE_UPGRADE_DEV_SIGNING_KEY
    elif [[ $SECURE_UPGRADE_MODE == "prod" ]]; then
        #  Here Vendor signing should be implemented
        OUTPUT_SEC_BOOT_DIR=$FILESYSTEM_ROOT/boot

        if [ ! -f $sonic_su_prod_signing_tool ]; then
            echo "Error: SONiC sonic_su_prod_signing_tool=$sonic_su_prod_signing_tool script missing"
            exit 1
        fi

        sudo $sonic_su_prod_signing_tool -a $CONFIGURED_ARCH \
                                         -r $FILESYSTEM_ROOT \
                                         -l $LINUX_KERNEL_VERSION \
                                         -o $OUTPUT_SEC_BOOT_DIR \
                                         $SECURE_UPGRADE_PROD_TOOL_ARGS

        # verifying all EFI files and kernel modules in $OUTPUT_SEC_BOOT_DIR
        sudo ./scripts/secure_boot_signature_verification.sh -e $OUTPUT_SEC_BOOT_DIR \
                                                             -c $SECURE_UPGRADE_SIGNING_CERT \
                                                             -k ${FILESYSTEM_ROOT}/usr/lib/modules

        # verifying vmlinuz file.
        sudo ./scripts/secure_boot_signature_verification.sh -e $FILESYSTEM_ROOT/boot/vmlinuz-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH} \
                                                             -c $SECURE_UPGRADE_SIGNING_CERT
    fi
    echo "Secure Boot support build stage: END."
fi

## Update initramfs
sudo chroot $FILESYSTEM_ROOT update-initramfs -u
## Convert initrd image to u-boot format
if [[ $TARGET_BOOTLOADER == uboot ]]; then
    INITRD_FILE=initrd.img-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH}
    KERNEL_FILE=vmlinuz-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH}
    if [[ $CONFIGURED_ARCH == armhf ]]; then
        INITRD_FILE=initrd.img-${LINUX_KERNEL_VERSION}-sonic-armmp
        sudo LANG=C chroot $FILESYSTEM_ROOT mkimage -A arm -O linux -T ramdisk -C gzip -d /boot/$INITRD_FILE /boot/u${INITRD_FILE}
        ## Overwriting the initrd image with uInitrd
        sudo LANG=C chroot $FILESYSTEM_ROOT mv /boot/u${INITRD_FILE} /boot/$INITRD_FILE
    elif [[ $CONFIGURED_ARCH == arm64 ]]; then
        if [[ $CONFIGURED_PLATFORM == pensando ]]; then
            ## copy device tree file into boot (XXX: need to compile dtb from dts)
            sudo cp -v $FILESYSTEM_ROOT/usr/lib/linux-image-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH}/pensando/elba-asic-psci.dtb $FILESYSTEM_ROOT/boot/
            sudo cp -v $FILESYSTEM_ROOT/usr/lib/linux-image-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH}/pensando/elba-asic-psci-lipari.dtb $FILESYSTEM_ROOT/boot/
            sudo cp -v $FILESYSTEM_ROOT/usr/lib/linux-image-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH}/pensando/elba-asic-psci-mtfuji.dtb $FILESYSTEM_ROOT/boot/
            sudo cp -v $PLATFORM_DIR/pensando/install_file $FILESYSTEM_ROOT/boot/
            ## make kernel as gzip file
            sudo LANG=C chroot $FILESYSTEM_ROOT gzip /boot/${KERNEL_FILE}
            sudo LANG=C chroot $FILESYSTEM_ROOT mv /boot/${KERNEL_FILE}.gz /boot/${KERNEL_FILE}
            ## Convert initrd image to u-boot format
            sudo LANG=C chroot $FILESYSTEM_ROOT mkimage -A arm64 -O linux -T ramdisk -C gzip -d /boot/$INITRD_FILE /boot/u${INITRD_FILE}
            ## Overwriting the initrd image with uInitrd
            sudo LANG=C chroot $FILESYSTEM_ROOT mv /boot/u${INITRD_FILE} /boot/$INITRD_FILE
        else
            sudo cp -v $PLATFORM_DIR/$CONFIGURED_PLATFORM/sonic_fit.its $FILESYSTEM_ROOT/boot/
            sudo LANG=C chroot $FILESYSTEM_ROOT mkimage -f /boot/sonic_fit.its /boot/sonic_${CONFIGURED_ARCH}.fit
        fi
    fi
fi

# Collect host image version files before cleanup
SONIC_VERSION_CACHE=${SONIC_VERSION_CACHE}  \
	DBGOPT="${DBGOPT}" \
	scripts/collect_host_image_version_files.sh $CONFIGURED_ARCH $IMAGE_DISTRO $TARGET_PATH $FILESYSTEM_ROOT

# Remove GCC
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y remove gcc

# Remove eatmydata
sudo rm $FILESYSTEM_ROOT/etc/apt/apt.conf.d/00image-install-eatmydata $FILESYSTEM_ROOT/usr/local/bin/dpkg
sudo LANG=C DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y remove eatmydata

## Clean up apt
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get -y autoremove
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get autoclean
sudo LANG=C chroot $FILESYSTEM_ROOT apt-get clean
sudo LANG=C chroot $FILESYSTEM_ROOT bash -c 'rm -rf /usr/share/doc/* /usr/share/locale/* /var/lib/apt/lists/* /tmp/*'

## Clean up proxy
[ -n "$http_proxy" ] && sudo rm -f $FILESYSTEM_ROOT/etc/apt/apt.conf.d/01proxy

## Clean up pip cache
sudo LANG=C chroot $FILESYSTEM_ROOT pip3 cache purge

## Umount all
echo '[INFO] Umount all'
## Display all process details access /proc
sudo LANG=C chroot $FILESYSTEM_ROOT fuser -vm /proc
## Kill the processes
sudo LANG=C chroot $FILESYSTEM_ROOT fuser -km /proc || true
## Wait fuser fully kill the processes
sudo timeout 15s bash -c 'until LANG=C chroot $0 umount /proc; do sleep 1; done' $FILESYSTEM_ROOT || true

## Prepare empty directory to trigger mount move in initramfs-tools/mount_loop_root, implemented by patching
sudo mkdir $FILESYSTEM_ROOT/host


if [[ "$CHANGE_DEFAULT_PASSWORD" == "y" ]]; then
    ## Expire default password for exitsing users that can do login
    default_users=$(cat $FILESYSTEM_ROOT/etc/passwd | grep "/home"|  grep ":/bin/bash\|:/bin/sh" | awk -F ":" '{print $1}' 2> /dev/null)
    for user in $default_users
    do
        sudo LANG=C chroot $FILESYSTEM_ROOT passwd -e ${user}
    done
fi

## Compress most file system into squashfs file
sudo rm -f $INSTALLER_PAYLOAD $FILESYSTEM_SQUASHFS
## Output the file system total size for diag purpose
## Note: -x to skip directories on different file systems, such as /proc
sudo du -hsx $FILESYSTEM_ROOT
sudo mkdir -p $FILESYSTEM_ROOT/var/lib/docker

## Clear DNS configuration inherited from the build server
sudo rm -f $FILESYSTEM_ROOT/etc/resolvconf/resolv.conf.d/original
sudo cp files/image_config/resolv-config/resolv.conf.head $FILESYSTEM_ROOT/etc/resolvconf/resolv.conf.d/head
sudo rm -f $FILESYSTEM_ROOT/etc/resolv.conf
sudo touch $FILESYSTEM_ROOT/etc/resolv.conf

## Optimize filesystem size
if [ "$BUILD_REDUCE_IMAGE_SIZE" = "y" ]; then
   sudo scripts/build-optimize-fs-size.py "$FILESYSTEM_ROOT" \
      --image-type "$IMAGE_TYPE" \
      --hardlinks var/lib/docker \
      --hardlinks usr/share/sonic/device \
      --remove-docs \
      --remove-mans \
      --remove-licenses
fi

sudo mksquashfs $FILESYSTEM_ROOT $FILESYSTEM_SQUASHFS -comp zstd -b 1M -e boot -e var/lib/docker -e $PLATFORM_DIR

## Reduce /boot permission
sudo chmod -R go-wx $FILESYSTEM_ROOT/boot

# Ensure admin gid is 1000
gid_user=$(sudo LANG=C chroot $FILESYSTEM_ROOT id -g $USERNAME) || gid_user="none"
if [ "${gid_user}" != "1000" ]; then
    die "expect gid 1000. current:${gid_user}"
fi

# ALERT: This bit of logic tears down the qemu based build environment used to
# perform builds for the ARM architecture. This must be the last step in this
# script before creating the Sonic installer payload zip file.
if [[ $MULTIARCH_QEMU_ENVIRON == y || $CROSS_BUILD_ENVIRON == y ]]; then
    # Remove qemu arm bin executable used for cross-building
    sudo rm -f $FILESYSTEM_ROOT/usr/bin/qemu*static || true
    DOCKERFS_PATH=../dockerfs/
fi

## Compress docker files
if [ "$BUILD_REDUCE_IMAGE_SIZE" = "y" ]; then
    pushd $FILESYSTEM_ROOT && sudo tar -I pzstd -cf $OLDPWD/$FILESYSTEM_DOCKERFS -C ${DOCKERFS_PATH}var/lib/docker .; popd
else
    pushd $FILESYSTEM_ROOT && sudo tar -I pigz -cf $OLDPWD/$FILESYSTEM_DOCKERFS -C ${DOCKERFS_PATH}var/lib/docker .; popd
fi

## Compress together with /boot, /var/lib/docker and $PLATFORM_DIR as an installer payload zip file
pushd $FILESYSTEM_ROOT && sudo tar -I pigz -cf platform.tar.gz -C $PLATFORM_DIR . && sudo zip -n .gz $OLDPWD/$INSTALLER_PAYLOAD -r boot/ platform.tar.gz; popd
sudo zip -g -n .squashfs:.gz $INSTALLER_PAYLOAD $FILESYSTEM_SQUASHFS $FILESYSTEM_DOCKERFS
