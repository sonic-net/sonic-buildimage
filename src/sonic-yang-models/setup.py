import shutil
import os
import glob
import jinja2
import sys
from setuptools import setup, find_packages
from setuptools.command.build_py import build_py

# read me
with open('README.rst') as readme_file:
    readme = readme_file.read()

def validate_yang_files_completeness(yang_files_list):
    """
    Validate that all YANG files are included in the yang_files list.
    
    This function checks:
    1. All .yang files in yang-models/ directory (excluding auto-generated ones)
    2. All .yang.j2 template files in yang-templates/ directory
    3. Compares the total count with the yang_files list
    
    Returns:
        tuple: (is_valid, expected_files, missing_files, extra_files)
    """
    # Get all template files and convert to .yang names
    template_files = set()
    if os.path.exists('./yang-templates/'):
        for template_path in glob.glob('./yang-templates/*.yang.j2'):
            template_name = os.path.basename(template_path).replace('.j2', '')
            template_files.add(template_name)

    # Get all actual YANG files (excluding auto-generated ones)
    actual_yang_files = set()
    if os.path.exists('./yang-models/'):
        for yang_path in glob.glob('./yang-models/*.yang'):
            yang_name = os.path.basename(yang_path)
            actual_yang_files.add(yang_name)

    # Expected files = actual YANG files + template files
    expected_files = actual_yang_files.union(template_files)

    # Convert yang_files list to set for comparison
    yang_files_set = set(yang_files_list)

    # Find discrepancies
    missing_files = expected_files - yang_files_set
    extra_files = yang_files_set - expected_files

    is_valid = len(missing_files) == 0 and len(extra_files) == 0

    return is_valid, expected_files, missing_files, extra_files

def print_yang_files_validation_report(yang_files_list):
    """Print a validation report for YANG files completeness."""
    is_valid, expected_files, missing_files, extra_files = validate_yang_files_completeness(yang_files_list)

    print(f"Expected files count: {len(expected_files)}")
    print(f"yang_files list count: {len(yang_files_list)}")

    if is_valid:
        print("VALIDATION PASSED: All YANG files are properly included!")
    else:
        print("VALIDATION FAILED: Discrepancies found!")

        if missing_files:
            print(f"\nMISSING from yang_files list ({len(missing_files)} files):")
            for file in sorted(missing_files):
                print(f"  - {file}")

        if extra_files:
            print(f"\nEXTRA in yang_files list ({len(extra_files)} files):")
            for file in sorted(extra_files):
                print(f"  - {file}")

    return is_valid

# List of yang file names to be included in the wheel.
# Specify only the file basenames here; directory prefixes will be added automatically.
yang_files = [
    'sonic-acl.yang',
    'sonic-asic-sensors.yang',
    'sonic-auto_techsupport.yang',
    'sonic-banner.yang',
    'sonic-bgp-aggregate-address.yang',
    'sonic-bgp-allowed-prefix.yang',
    'sonic-bgp-bbr.yang',
    'sonic-bgp-common.yang',
    'sonic-bgp-device-global.yang',
    'sonic-bgp-global.yang',
    'sonic-bgp-internal-neighbor.yang',
    'sonic-bgp-monitor.yang',
    'sonic-bgp-neighbor.yang',
    'sonic-bgp-peergroup.yang',
    'sonic-bgp-peerrange.yang',
    'sonic-bgp-prefix-list.yang',
    'sonic-bgp-sentinel.yang',
    'sonic-bgp-voq-chassis-neighbor.yang',
    'sonic-bmp.yang',
    'sonic-breakout_cfg.yang',
    'sonic-buffer-pg.yang',
    'sonic-buffer-pool.yang',
    'sonic-buffer-port-egress-profile-list.yang',
    'sonic-buffer-port-ingress-profile-list.yang',
    'sonic-buffer-profile.yang',
    'sonic-buffer-queue.yang',
    'sonic-cable-length.yang',
    'sonic-chassis-module.yang',
    'sonic-console.yang',
    'sonic-copp.yang',
    'sonic-crm.yang',
    'sonic-dash.yang',
    'sonic-debug-counter.yang',
    'sonic-default-lossless-buffer-parameter.yang',
    'sonic-device_metadata.yang',
    'sonic-device_neighbor.yang',
    'sonic-device_neighbor_metadata.yang',
    'sonic-dhcp-server-ipv4.yang',
    'sonic-dhcp-server.yang',
    'sonic-dhcpv4-relay.yang',
    'sonic-dhcpv6-relay.yang',
    'sonic-dns.yang',
    'sonic-dot1p-tc-map.yang',
    'sonic-dscp-fc-map.yang',
    'sonic-dscp-tc-map.yang',
    'sonic-events-bgp.yang',
    'sonic-events-common.yang',
    'sonic-events-dhcp-relay.yang',
    'sonic-events-host.yang',
    'sonic-events-swss.yang',
    'sonic-events-syncd.yang',
    'sonic-exp-fc-map.yang',
    'sonic-extension.yang',
    'sonic-fabric-monitor.yang',
    'sonic-fabric-port.yang',
    'sonic-feature.yang',
    'sonic-fine-grained-ecmp.yang',
    'sonic-fips.yang',
    'sonic-flex_counter.yang',
    'sonic-gnmi.yang',
    'sonic-grpcclient.yang',
    'sonic-hash.yang',
    'sonic-heartbeat.yang',
    'sonic-high-frequency-telemetry.yang',
    'sonic-interface.yang',
    'sonic-kdump.yang',
    'sonic-kubernetes_master.yang',
    'sonic-lldp.yang',
    'sonic-logger.yang',
    'sonic-loopback-interface.yang',
    'sonic-lossless-traffic-pattern.yang',
    'sonic-macsec.yang',
    'sonic-mclag.yang',
    'sonic-memory-statistics.yang',
    'sonic-mgmt_interface.yang',
    'sonic-mgmt_port.yang',
    'sonic-mgmt_vrf.yang',
    'sonic-mirror-session.yang',
    'sonic-mpls-tc-map.yang',
    'sonic-mux-cable.yang',
    'sonic-mux-linkmgr.yang',
    'sonic-nat.yang',
    'sonic-neigh.yang',
    'sonic-ntp.yang',
    'sonic-nvgre-tunnel.yang',
    'sonic-passwh.yang',
    'sonic-pbh.yang',
    'sonic-peer-switch.yang',
    'sonic-pfc-priority-priority-group-map.yang',
    'sonic-pfc-priority-queue-map.yang',
    'sonic-pfcwd.yang',
    'sonic-policer.yang',
    'sonic-port-qos-map.yang',
    'sonic-port.yang',
    'sonic-portchannel.yang',
    'sonic-queue.yang',
    'sonic-restapi.yang',
    'sonic-route-common.yang',
    'sonic-route-map.yang',
    'sonic-routing-policy-sets.yang',
    'sonic-scheduler.yang',
    'sonic-serial-console.yang',
    'sonic-sflow.yang',
    'sonic-smart-switch.yang',
    'sonic-snmp.yang',
    'sonic-spanning-tree.yang',
    'sonic-srv6.yang',
    'sonic-ssh-server.yang',
    'sonic-static-route.yang',
    'sonic-storm-control.yang',
    'sonic-stormond-config.yang',
    'sonic-subnet-decap.yang',
    'sonic-suppress-asic-sdk-health-event.yang',
    'sonic-syslog.yang',
    'sonic-system-aaa.yang',
    'sonic-system-defaults.yang',
    'sonic-system-ldap.yang',
    'sonic-system-port.yang',
    'sonic-system-radius.yang',
    'sonic-system-tacacs.yang',
    'sonic-tc-dscp-map.yang',
    'sonic-tc-priority-group-map.yang',
    'sonic-tc-queue-map.yang',
    'sonic-telemetry.yang',
    'sonic-telemetry_client.yang',
    'sonic-trimming.yang',
    'sonic-tunnel.yang',
    'sonic-types.yang',
    'sonic-versions.yang',
    'sonic-vlan-sub-interface.yang',
    'sonic-vlan.yang',
    'sonic-vnet.yang',
    'sonic-voq-inband-interface.yang',
    'sonic-vrf.yang',
    'sonic-vxlan.yang',
    'sonic-warm-restart.yang',
    'sonic-wred-profile.yang',
    'sonic-xcvrd-log.yang',
    'sonic-ztp.yang',
    'sonic-fast-linkup.yang',
]

class my_build_py(build_py):
    def run(self):
        if not self.dry_run:
            pass

        # Validate YANG files completeness before building
        print("RUNNING YANG FILES VALIDATION...")
        validation_passed = print_yang_files_validation_report(yang_files)

        if not validation_passed:
            print("Build failed due to YANG files validation errors.")
            print("Please update the yang_files list in setup.py to include all missing files.")
            sys.exit(1)
        print("Validation passed. Proceeding with build...")

        if not os.path.exists("./yang-models"):
            os.makedirs("./yang-models")

        if not os.path.exists("./cvlyang-models"):
            os.makedirs("./cvlyang-models")

        # copy non-template yang model to internal yang model directory
        for fname in glob.glob("./yang-models/*.yang"):
            bfname = os.path.basename(fname)
            shutil.copyfile("./yang-models/{}".format(bfname), "./cvlyang-models/{}".format(bfname))

        # templated yang models
        env = jinja2.Environment(loader=jinja2.FileSystemLoader('./yang-templates/'), trim_blocks=True)
        for fname in glob.glob("./yang-templates/*.yang.j2"):
            bfname = os.path.basename(fname)
            template = env.get_template(bfname)
            yang_model = template.render(yang_model_type="py")
            cvlyang_model = template.render(yang_model_type="cvl")
            with open("./yang-models/{}".format(bfname.strip(".j2")), 'w') as f:
                f.write(yang_model)
            with open("./cvlyang-models/{}".format(bfname.strip(".j2")), 'w') as f:
                f.write(cvlyang_model)

        build_py.run(self)

setup(
    author="lnos-coders",
    author_email='lnos-coders@linkedin.com',
    python_requires='>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*, !=3.4.*',
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Natural Language :: English',
        "Programming Language :: Python :: 2",
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
    description="Package contains YANG models for sonic.",
    license="GNU General Public License v3",
    long_description=readme + '\n\n',
    install_requires = [
    ],
    tests_require = [
        'pytest',
        'ijson==3.2.3'
    ],
    setup_requires = [
        'pytest-runner',
        'wheel'
    ],
    extras_require = {
        "testing": [
            'pytest',
            'ijson==3.2.3'
        ],
    },
    include_package_data=True,
    keywords='sonic-yang-models',
    name='sonic-yang-models',
    py_modules=[],
    packages=find_packages(),
    version='1.0',
    cmdclass={'build_py': my_build_py},
    data_files=[
        ('yang-models', ['./yang-models/'+y for y in yang_files]),
        ('cvlyang-models', ['./cvlyang-models/'+y for y in yang_files]),
        ('yang-models', ['./yang-models/sonic-acl.yang',
                         './yang-models/sonic-ars.yang',
                         './yang-models/sonic-auto_techsupport.yang',
                         './yang-models/sonic-bgp-bbr.yang',
                         './yang-models/sonic-banner.yang',
                         './yang-models/sonic-bgp-common.yang',
                         './yang-models/sonic-bgp-device-global.yang',
                         './yang-models/sonic-bgp-global.yang',
                         './yang-models/sonic-bgp-monitor.yang',
                         './yang-models/sonic-bgp-internal-neighbor.yang',
                         './yang-models/sonic-bgp-neighbor.yang',
                         './yang-models/sonic-bgp-peergroup.yang',
                         './yang-models/sonic-bgp-peerrange.yang',
                         './yang-models/sonic-bgp-allowed-prefix.yang',
                         './yang-models/sonic-bgp-voq-chassis-neighbor.yang',
                         './yang-models/sonic-breakout_cfg.yang',
                         './yang-models/sonic-buffer-pg.yang',
                         './yang-models/sonic-buffer-pool.yang',
                         './yang-models/sonic-buffer-port-ingress-profile-list.yang',
                         './yang-models/sonic-buffer-port-egress-profile-list.yang',
                         './yang-models/sonic-buffer-profile.yang',
                         './yang-models/sonic-buffer-queue.yang',
                         './yang-models/sonic-cable-length.yang',
                         './yang-models/sonic-chassis-module.yang',
                         './yang-models/sonic-copp.yang',
                         './yang-models/sonic-console.yang',
                         './yang-models/sonic-crm.yang',
                         './yang-models/sonic-dash.yang',
                         './yang-models/sonic-default-lossless-buffer-parameter.yang',
                         './yang-models/sonic-device_metadata.yang',
                         './yang-models/sonic-device_neighbor.yang',
                         './yang-models/sonic-device_neighbor_metadata.yang',
                         './yang-models/sonic-dhcp-server.yang',
                         './yang-models/sonic-dhcpv6-relay.yang',
                         './yang-models/sonic-dns.yang',
                         './yang-models/sonic-events-bgp.yang',
                         './yang-models/sonic-events-common.yang',
                         './yang-models/sonic-events-dhcp-relay.yang',
                         './yang-models/sonic-events-host.yang',
                         './yang-models/sonic-events-swss.yang',
                         './yang-models/sonic-events-syncd.yang',
                         './yang-models/sonic-extension.yang',
                         './yang-models/sonic-fabric-monitor.yang',
                         './yang-models/sonic-fabric-port.yang',
                         './yang-models/sonic-flex_counter.yang',
                         './yang-models/sonic-fine-grained-ecmp.yang',
                         './yang-models/sonic-feature.yang',
                         './yang-models/sonic-fips.yang',
                         './yang-models/sonic-hash.yang',
                         './yang-models/sonic-system-defaults.yang',
                         './yang-models/sonic-interface.yang',
                         './yang-models/sonic-kdump.yang',
                         './yang-models/sonic-kubernetes_master.yang',
                         './yang-models/sonic-loopback-interface.yang',
                         './yang-models/sonic-lossless-traffic-pattern.yang',
                         './yang-models/sonic-memory-statistics.yang',
                         './yang-models/sonic-mgmt_interface.yang',
                         './yang-models/sonic-mgmt_port.yang',
                         './yang-models/sonic-mgmt_vrf.yang',
                         './yang-models/sonic-mirror-session.yang',
                         './yang-models/sonic-mpls-tc-map.yang',
                         './yang-models/sonic-mux-cable.yang',
                         './yang-models/sonic-mux-linkmgr.yang',
                         './yang-models/sonic-neigh.yang',
                         './yang-models/sonic-ntp.yang',
                         './yang-models/sonic-nat.yang',
                         './yang-models/sonic-nvgre-tunnel.yang',
                         './yang-models/sonic-passwh.yang',
                         './yang-models/sonic-ssh-server.yang',
                         './yang-models/sonic-pbh.yang',
                         './yang-models/sonic-port.yang',
                         './yang-models/sonic-policer.yang',
                         './yang-models/sonic-portchannel.yang',
                         './yang-models/sonic-pfcwd.yang',
                         './yang-models/sonic-route-common.yang',
                         './yang-models/sonic-route-map.yang',
                         './yang-models/sonic-routing-policy-sets.yang',
                         './yang-models/sonic-sflow.yang',
                         './yang-models/sonic-snmp.yang',
                         './yang-models/sonic-suppress-asic-sdk-health-event.yang',
                         './yang-models/sonic-syslog.yang',
                         './yang-models/sonic-system-aaa.yang',
                         './yang-models/sonic-system-tacacs.yang',
                         './yang-models/sonic-system-radius.yang',
                         './yang-models/sonic-system-ldap.yang',
                         './yang-models/sonic-subnet-decap.yang',
                         './yang-models/sonic-telemetry.yang',
                         './yang-models/sonic-telemetry_client.yang',
                         './yang-models/sonic-gnmi.yang',
                         './yang-models/sonic-tunnel.yang',
                         './yang-models/sonic-types.yang',
                         './yang-models/sonic-versions.yang',
                         './yang-models/sonic-vlan.yang',
                         './yang-models/sonic-vnet.yang',
                         './yang-models/sonic-voq-inband-interface.yang',
                         './yang-models/sonic-vxlan.yang',
                         './yang-models/sonic-vrf.yang',
                         './yang-models/sonic-mclag.yang',
                         './yang-models/sonic-vlan-sub-interface.yang',
                         './yang-models/sonic-warm-restart.yang',
                         './yang-models/sonic-lldp.yang',
                         './yang-models/sonic-scheduler.yang',
                         './yang-models/sonic-wred-profile.yang',
                         './yang-models/sonic-queue.yang',
                         './yang-models/sonic-restapi.yang',
                         './yang-models/sonic-dscp-fc-map.yang',
                         './yang-models/sonic-exp-fc-map.yang',
                         './yang-models/sonic-dscp-tc-map.yang',
                         './yang-models/sonic-dhcp-server-ipv4.yang',
                         './yang-models/sonic-dot1p-tc-map.yang',
                         './yang-models/sonic-storm-control.yang',
                         './yang-models/sonic-tc-priority-group-map.yang',
                         './yang-models/sonic-tc-queue-map.yang',
                         './yang-models/sonic-peer-switch.yang',
                         './yang-models/sonic-tc-dscp-map.yang',
                         './yang-models/sonic-pfc-priority-queue-map.yang',
                         './yang-models/sonic-pfc-priority-priority-group-map.yang',
                         './yang-models/sonic-logger.yang',
                         './yang-models/sonic-port-qos-map.yang',
                         './yang-models/sonic-static-route.yang',
                         './yang-models/sonic-system-port.yang',
                         './yang-models/sonic-macsec.yang',
                         './yang-models/sonic-bgp-sentinel.yang',
                         './yang-models/sonic-bgp-prefix-list.yang',
                         './yang-models/sonic-asic-sensors.yang',
                         './yang-models/sonic-bmp.yang',
                         './yang-models/sonic-xcvrd-log.yang',
                         './yang-models/sonic-grpcclient.yang',
                         './yang-models/sonic-serial-console.yang',
                         './yang-models/sonic-smart-switch.yang',
                         './yang-models/sonic-srv6.yang']),
        ('cvlyang-models', ['./cvlyang-models/sonic-acl.yang',
                         './cvlyang-models/sonic-banner.yang',
                         './cvlyang-models/sonic-bgp-common.yang',
                         './cvlyang-models/sonic-bgp-global.yang',
                         './cvlyang-models/sonic-bgp-monitor.yang',
                         './cvlyang-models/sonic-bgp-neighbor.yang',
                         './cvlyang-models/sonic-bgp-peergroup.yang',
                         './cvlyang-models/sonic-bgp-peerrange.yang',
                         './cvlyang-models/sonic-bgp-allowed-prefix.yang',
                         './cvlyang-models/sonic-breakout_cfg.yang',
                         './cvlyang-models/sonic-copp.yang',
                         './cvlyang-models/sonic-crm.yang',
                         './cvlyang-models/sonic-device_metadata.yang',
                         './cvlyang-models/sonic-device_neighbor.yang',
                         './cvlyang-models/sonic-events-bgp.yang',
                         './cvlyang-models/sonic-events-common.yang',
                         './cvlyang-models/sonic-events-dhcp-relay.yang',
                         './cvlyang-models/sonic-events-host.yang',
                         './cvlyang-models/sonic-events-swss.yang',
                         './cvlyang-models/sonic-events-syncd.yang',
                         './cvlyang-models/sonic-device_neighbor_metadata.yang',
                         './cvlyang-models/sonic-extension.yang',
                         './cvlyang-models/sonic-fabric-monitor.yang',
                         './cvlyang-models/sonic-fabric-port.yang',
                         './cvlyang-models/sonic-flex_counter.yang',
                         './cvlyang-models/sonic-feature.yang',
                         './cvlyang-models/sonic-fine-grained-ecmp.yang',
                         './cvlyang-models/sonic-fips.yang',
                         './cvlyang-models/sonic-hash.yang',
                         './cvlyang-models/sonic-system-defaults.yang',
                         './cvlyang-models/sonic-interface.yang',
                         './cvlyang-models/sonic-kdump.yang',
                         './cvlyang-models/sonic-kubernetes_master.yang',
                         './cvlyang-models/sonic-loopback-interface.yang',
                         './cvlyang-models/sonic-mgmt_interface.yang',
                         './cvlyang-models/sonic-memory-statistics.yang',
                         './cvlyang-models/sonic-mgmt_port.yang',
                         './cvlyang-models/sonic-mgmt_vrf.yang',
                         './cvlyang-models/sonic-ntp.yang',
                         './cvlyang-models/sonic-nat.yang',
                         './cvlyang-models/sonic-nvgre-tunnel.yang',
                         './cvlyang-models/sonic-pbh.yang',
                         './cvlyang-models/sonic-ssh-server.yang',
                         './cvlyang-models/sonic-policer.yang',
                         './cvlyang-models/sonic-port.yang',
                         './cvlyang-models/sonic-portchannel.yang',
                         './cvlyang-models/sonic-pfcwd.yang',
                         './cvlyang-models/sonic-route-common.yang',
                         './cvlyang-models/sonic-route-map.yang',
                         './cvlyang-models/sonic-routing-policy-sets.yang',
                         './cvlyang-models/sonic-sflow.yang',
                         './cvlyang-models/sonic-snmp.yang',
                         './cvlyang-models/sonic-system-aaa.yang',
                         './cvlyang-models/sonic-system-tacacs.yang',
                         './cvlyang-models/sonic-telemetry.yang',
                         './cvlyang-models/sonic-telemetry_client.yang',
                         './cvlyang-models/sonic-gnmi.yang',
                         './cvlyang-models/sonic-types.yang',
                         './cvlyang-models/sonic-versions.yang',
                         './cvlyang-models/sonic-vlan.yang',
                         './cvlyang-models/sonic-vrf.yang',
                         './cvlyang-models/sonic-warm-restart.yang',
                         './cvlyang-models/sonic-lldp.yang',
                         './cvlyang-models/sonic-scheduler.yang',
                         './cvlyang-models/sonic-wred-profile.yang',
                         './cvlyang-models/sonic-queue.yang',
                         './cvlyang-models/sonic-dscp-tc-map.yang',
                         './cvlyang-models/sonic-dot1p-tc-map.yang',
                         './cvlyang-models/sonic-tc-priority-group-map.yang',
                         './cvlyang-models/sonic-tc-queue-map.yang',
                         './cvlyang-models/sonic-pfc-priority-queue-map.yang',
                         './cvlyang-models/sonic-pfc-priority-priority-group-map.yang',
                         './cvlyang-models/sonic-logger.yang',
                         './cvlyang-models/sonic-port-qos-map.yang',
                         './cvlyang-models/sonic-static-route.yang',
                         './cvlyang-models/sonic-system-port.yang',
                         './cvlyang-models/sonic-macsec.yang',
                         './cvlyang-models/sonic-bmp.yang',
                         './cvlyang-models/sonic-serial-console.yang',
                         './cvlyang-models/sonic-bgp-sentinel.yang']),
    ],
    zip_safe=False,
)
