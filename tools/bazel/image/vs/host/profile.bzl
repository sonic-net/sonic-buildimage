"""Default VS host render values from rules/config and onie-image.conf."""

VS_CONTEXT = {"sonic_asic_platform": "vs", "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64", "IMAGE_DISTRO": "trixie", "include_kubernetes": "n", "include_kubernetes_master": "n", "include_restapi": "n", "include_system_telemetry": "n", "include_dhcp_server": "n", "include_iccpd": "n", "include_pde": "n", "include_stp": "n", "include_redfish": "y", "shutdown_bgp_on_start": "", "enable_pfcwd_on_start": "", "default_buffer_model": "", "enable_auto_tech_support": "y", "BUILD_REDUCE_IMAGE_SIZE": "n", "build_reduce_image_size": "n", "ENABLE_FRR_SNMP_AGENT": "y", "ENABLE_MULTIDB": "", "SECURE_UPGRADE_MODE": "no_sign", "FRR_USER_UID": "300", "FRR_USER_GID": "300", "SONIC_ROUTING_STACK": "frr", "FILESYSTEM_DOCKERFS": "dockerfs.tar.gz", "DOCKERFS_DIR": "docker", "DOCKER_RAMFS_SIZE": "3500M"}

HOST_GENERATED_SERVICES = [
    "sed-pw-tpm-recovery.service",
    "serial-config.service",
    "warmboot-finalizer.service",
    "rsyslog-config.service",
    "system-health.service",
    "logrotate-config.service",
    "interfaces-config.service",
    "copp-config.service",
    "dhcp_dos_logger.service",
    "resolv-config.service",
    "hostname-config.service",
    "banner-config.service",
    "topology.service",
    "config-topology.service",
    "config-setup.service",
    "tacacs-config.timer",
    "tacacs-config.service",
    "config-chassisdb.service",
    "backend-acl.service",
    "pcie-check.service"
]
