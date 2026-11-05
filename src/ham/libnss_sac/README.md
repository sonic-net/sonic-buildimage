# System-Assigned Credentials (SAC)

The SAC Name Service Switch (NSS) module automatically assigns a `UID` and primary `GID` to RADIUS/TACACS+
users.

This is used to fill a gap with protocols such as RADIUS and TACACS+ that were
never designed for UNIX-like systems. These protocols have no concept of User
IDs (`UID`) and Group IDs (`GID`), which are essential to any program running on
UNIX-like systems.

This NSS module gets installed on the host and configured in `/etc/nsswitch.conf`.

