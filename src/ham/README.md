# Host Account Management

## Introduction

Host Account Management, or HAM, serves as the single source of truth for SONiC user accounts. HAM was created to solve several problems related to user accounts. Here's a summary:

- Allow applications running in containers to retrieve user account information on the host using standard APIs such as `getpwnam()`, `getgrname()`, etc.
- Allow applications running in containers to create/modify/delete local user accounts on the host.
- Automatically assign UNIX user credentials (UID/GID) for users authenticated by RADIUS/TACACS+ that do not natively provide UNIX credentials. 
- Allow additional credentials (such as keys or certificates) to be automatically created when a new user account is created.

HAM was introduced in [AAA Improvements](https://github.com/Azure/SONiC/blob/master/doc/aaa/AAA%20Improvements/AAA%20Improvements.md). This document lists several challenges related to AAA (Authentication, Authorization, and Accounting) and how HAM helps mitigate some of these problems. This document provides details about the HAM software.

## Components

HAM provides the following components.

| Package                       | Description                                                  |
| ----------------------------- | ------------------------------------------------------------ |
| `ham_[ver]_[arch].deb`        | Contains `hamd`, the HAM daemon, and `hamctl` a companion utility program for `hamd`. |
| `libham_[ver]_[arch].deb`     | Run-time library that primarily provides APIs to interact with `hamd` over D-Bus. |
| `libham-dev_[ver]_[arch].deb` | The counterpart development library to `libham_[version]_[arch].deb` |
| `libnss-ham_[ver]_[arch].deb` | NSS module that allows containerized apps to access user accounts on the host. |
| `libnss-sac_[ver]_[arch].deb` | NSS module for System-Assigned Credential                    |

## HAM Daemon (`hamd`)

The HAM daemon, `hamd`, is managed by `systemd`. The following `systemctl` commands are supported:

| Command                          | Description                                 |
| -------------------------------- | ------------------------------------------- |
| `systemctl start hamd.service`   | Start `hamd`                                |
| `systemctl stop hamd.service`    | Stop `hamd`                                 |
| `systemctl restart hamd.service` | Restart `hamd`. Effectively a stop + start. |
| `systemctl reload hamd.service`  | Reload configuration.                       |

### Configuration file

`hamd`'s configuration file is: `/etc/sonic/hamd/config`. This file can be modified while the daemon is running. The reload command described above can be issued to reload the configuration while the daemon is running.

The configuration file content is self-documented (look in the file to see which parameters are configurable).

### D-Bus interface

The interface to `hamd` is D-Bus. The D-Bus introspect method should be called to determine what APIs are available. The D-Bus interface is defined by the IDL (interface description language) file: [org.SONiC.HostAccountManagement.xml](./shared/org.SONiC.HostAccountManagement.xml).

#### D-Bus security

Many of the D-Bus APIs are restricted. Some are restricted to `root` only, others are restricted to groups `hamd_accounts` and `hamd`. The security policy can be found in `/etc/dbus-1/system.d/org.SONiC.HostAccountManagement.conf`.

## The hamctl utility

This is a shell utility designed for "human" users. It should not be invoked by other programs. It is a companion program to `hamd`. The `hamctl` utility is mainly used for debugging purposes. It is self-documented. Simply type "`hamctl --help`" for help.

## Roles to group mapping

It is possible to add users of a given role to specific Linux groups. For example, one may want to map users with "`admin`" role to the "`sudo`" group. The mapping can be defined in: `/etc/sonic/hamd/group-mapping`.

## Application-specific credentials

One can tell `hamd` to create supplemental user credentials with the use of plug-in scripts. For example, if one would like all new users created by `hamd` to automatically be assigned an SSH key, one need only add a script to the directory `/etc/sonic/hamd/scripts/post-create/`. Scripts added to this directory will be invoked, in asciibetical order, when users are created. Similarly, when users are deleted, `hamd` will invoke the scripts in `/etc/sonic/hamd/scripts/pre-delete/`. Note that when deleting a user account, `hamd` always deletes the whole home directory. So, it is not necessary to provide a "`/pre-delete`" to delete files from the home directory since those will be removed anyway. 

The script syntax is defined in these two files.

- `/etc/sonic/hamd/scripts/post-create/README`
- `/etc/sonic/hamd/scripts/pre-delete/README`