# Native gNMI default configuration

When native gNMI is enabled without any incoming or existing `GNMI` configuration,
host `sonic-cfggen --write-to-db` and retained-database image migration provision
`GNMI|gnmi` with `port=8080` and `user_auth=password,jwt`. This explicit ConfigDB
profile uses the launcher's certificate-free, loopback-only listener. Clients must
authenticate; it does not enable unauthenticated access or change the server's
certificate-only/noTLS rejection.

Existing `GNMI` profiles are never overwritten or repaired, including incomplete
profiles. Legacy `DEVICE_METADATA|x509` TLS configuration, SmartSwitch DPU
defaults, disabled gNMI, and ASIC namespaces are excluded. Read-only
`sonic-cfggen` queries and legacy `TELEMETRY` configuration are unchanged.
The native UDS-only launcher mode still disables TCP, and the server enforces
Unix socket access through filesystem permissions.

For network access, configure a valid TLS certificate/key pair and the intended
`user_auth` policy explicitly. The loopback password/JWT profile does not encrypt
transport and must not be exposed on a non-default VRF. See the
[GNMI configuration schema](../sonic-yang-models/doc/Configuration.md#gnmi).
