# FRR-SONiC Communication Channel (`dplane_fpm_sonic` module) #

<!-- omit from toc -->
## Table of Content 

<!-- TOC -->

- [Overview](#overview)
- [Enable `dplane_fpm_sonic` before building the SONiC image](#enable-dplane_fpm_sonic-before-building-the-sonic-image)
- [Enable `dplane_fpm_sonic` on a running SONiC image](#enable-dplane_fpm_sonic-on-a-running-sonic-image)

This folder contains the SONiC-specific FPM module (`dplane_fpm_sonic`).

## Overview

FRR uses the FPM module `dplane_fpm_nl` to send routing information to SONiC.
`dplane_fpm_nl` first encodes the routing information in a Netlink message and subsequently sends the Netlink message to SONiC.

Some features require information that the `dplane_fpm_nl` module does not include in the Netlink messages.

To overcome this limitation, SONiC now supports a new FPM module called `dplane_fpm_sonic`. `dplane_fpm_sonic` is a SONiC-specific FPM module. 
`dplane_fpm_sonic` is a copy of `dplane_fpm_nl`. It can be extended by adding new TLVs to support any information required by SONiC.

There are two options to enable the `dplane_fpm_sonic` module in SONiC: either enable the module before building the SONiC image or enable in a running SONiC image.


## Enable `dplane_fpm_sonic` before building the SONiC image

1. Open the `dockers/docker-fpm-frr/frr/supervisord/supervisord.conf.j2` file with a text editor (e.g. `vim`) and replace the `-M dplane_fpm_nl` command-line option with `-M dplane_fpm_sonic`:

```diff
[program:zebra]
-command=/usr/lib/frr/zebra -A 127.0.0.1 -s 90000000 -M dplane_fpm_nl -M snmp --asic-offload=notify_on_offload
+command=/usr/lib/frr/zebra -A 127.0.0.1 -s 90000000 -M dplane_fpm_sonic -M snmp --asic-offload=notify_on_offload
priority=4
autostart=false
autorestart=false
startsecs=0
stdout_logfile=syslog
stderr_logfile=syslog
dependent_startup=true
dependent_startup_wait_for=rsyslogd:running
```

2. Build the SONiC image as usual.


## Enable `dplane_fpm_sonic` on a running SONiC image

1. Login inside the `bgp` container

```shell
sonic$ docker exec -it bgp bash
```

2. Open the `/etc/supervisord/supervisor.conf` file with a text editor (e.g. `vim`) and replace the `-M dplane_fpm_nl` command-line option with `-M dplane_fpm_sonic`:

```diff
[program:zebra]
-command=/usr/lib/frr/zebra -A 127.0.0.1 -s 90000000 -M dplane_fpm_nl -M snmp --asic-offload=notify_on_offload
+command=/usr/lib/frr/zebra -A 127.0.0.1 -s 90000000 -M dplane_fpm_sonic -M snmp --asic-offload=notify_on_offload
priority=4
autostart=false
autorestart=false
startsecs=0
stdout_logfile=syslog
stderr_logfile=syslog
dependent_startup=true
dependent_startup_wait_for=rsyslogd:running
```

3. Apply the changes (this will restart `zebra` with the `dplane_fpm_sonic` module):

```sh
supervisorctl update
```

4. Exit from the container
