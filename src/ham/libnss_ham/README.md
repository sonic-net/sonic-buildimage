# HAM Name Service Switch Module

This Name Service Switch (NSS) module is designed for containers that needs access to user accounts
on the host. It allows applications running in containers to use standard POSIX
APIs such as `getpwnam()` and retrieve user/group account information from the
host.

The NSS module gets installed in the container and configure in
`/etc/nsswitch.conf`.