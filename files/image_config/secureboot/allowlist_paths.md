# Configuration Guide
It is the patterns of the relative paths in /host/image-{{hash}}/rw folder.
The patterns will not be used if the Sonic Secure Boot feature is not enabled.
The files that are not in the allowlist will be removed when the Sonic System cold reboot.

### Example config to add all the files in a folder to allowlist
home/.*

### Example config to add a file to allowlist
etc/nsswitch.conf

