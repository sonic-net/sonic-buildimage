"""Actual OCI producers for the installed mainline default VS container set."""

VS_IMAGES = {
    "docker-bmp-watchdog": "//dockers/docker-bmp-watchdog:docker-bmp-watchdog",
    "docker-dash-engine": "//platform/vs/docker-dash-engine:docker-dash-engine",
    "docker-dash-ha": "//dockers/docker-dash-ha:docker-dash-ha",
    "docker-database": "//dockers/docker-database:docker-database",
    "docker-dhcp-relay": "//dockers/docker-dhcp-relay:docker-dhcp-relay",
    "docker-eventd": "//dockers/docker-eventd:docker-eventd",
    "docker-fpm-frr": "//dockers/docker-fpm-frr:docker-fpm-frr",
    "docker-gbsyncd-vs": "//platform/vs/docker-gbsyncd-vs:docker-gbsyncd-vs",
    "docker-gnmi-sidecar": "//dockers/docker-gnmi-sidecar:docker-gnmi-sidecar",
    "docker-gnmi-watchdog": "//dockers/docker-gnmi-watchdog:docker-gnmi-watchdog",
    "docker-lldp": "//dockers/docker-lldp:docker-lldp",
    "docker-macsec": "//dockers/docker-macsec:docker-macsec",
    "docker-mux": "//dockers/docker-mux:docker-mux",
    "docker-nat": "//dockers/docker-nat:docker-nat",
    "docker-orchagent": "//dockers/docker-orchagent:docker-orchagent",
    "docker-platform-monitor": "//dockers/docker-platform-monitor:docker-platform-monitor",
    "docker-restapi-sidecar": "//dockers/docker-restapi-sidecar:docker-restapi-sidecar",
    "docker-router-advertiser": "//dockers/docker-router-advertiser:docker-router-advertiser",
    "docker-sflow": "//dockers/docker-sflow:docker-sflow",
    "docker-snmp": "//dockers/docker-snmp:docker-snmp",
    "docker-sonic-bmp": "//dockers/docker-sonic-bmp:docker-sonic-bmp",
    "docker-sonic-gnmi": "//dockers/docker-sonic-gnmi:docker-sonic-gnmi",
    "docker-sonic-mgmt-framework": "//dockers/docker-sonic-mgmt-framework:docker-sonic-mgmt-framework",
    "docker-sonic-otel": "//dockers/docker-sonic-otel:docker-sonic-otel",
    "docker-sonic-p4rt": "//dockers/docker-sonic-p4rt:docker-sonic-p4rt",
    "docker-syncd-vs": "//platform/vs/docker-syncd-vs:docker-syncd-vs",
    "docker-sysmgr": "//dockers/docker-sysmgr:docker-sysmgr",
    "docker-teamd": "//dockers/docker-teamd:docker-teamd"
}

# one-image.mk keeps runtime families without an installed debug archive.
# docker-dash-engine.mk declares its debug companion for validation only.
VS_DEBUG_IMAGES = {
    repository: label if repository == "docker-dash-engine" else label + "-dbg"
    for repository, label in VS_IMAGES.items()
}
