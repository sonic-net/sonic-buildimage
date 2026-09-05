# **ARM64 Build on AMD64 for SONiC BuildImage**

## **Table of Contents**

1. [About This Guide](#about-this-guide)  
2. [Hardware Specifications](#hardware-specifications)  
3. [Test Server Details](#test-server-details)  
4. [Pre-requisites](#pre-requisites)  
5. [Build Procedure](#build-procedure)  
6. [Expected Errors and Workarounds](#expected-errors-and-workarounds)  
7. [Console Output](#console-output)  
8. [Reference Links](#reference-links)  

## **About This Guide**

This document provides a detailed guide for building the SONiC BuildImage for the ARM64 architecture on an AMD64 system. During this process, build errors were encountered. The steps below outline the necessary hardware specifications, pre-requisites, build procedures, and solutions to known issues to ensure a successful build. This guide is based on the build conducted using both Debian 11 (Bullseye) Virtual Machine and Ubuntu 22.04.2 Host server.

## **Hardware Specifications**
```
CPU: Multiple cores (recommended for faster build speed)
Memory: Plenty of RAM (62 GiB used for this test)
Disk: 300 GiB of free disk space
Virtualization Support: KVM support enabled
```

## **Test Server Details**
```
The image has been built on both Ubuntu & Debian systems.
Ubuntu Server:
CPU(s): 4
Memory: 62 GiB
Disk: 1.1 TiB
Operating System: Ubuntu 22.04.2

Debian VM:
CPU(s): 4
Memory: 20 GiB
Disk: 670 GB
Operating System: Debian GNU/Linux 11 (bullseye)
```

## **Pre-requisites**

Install the following dependencies before proceeding with the build:

1\. Install pip and jinja
```
sudo apt install -y python3-pip
sudo pip3 install j2cli
```

2\. Install docker engine on debian
Follow the instructions at: [Docker Engine Install for Debian](https://docs.docker.com/engine/install/debian/)

3\. Add user to the docker group
```
sudo gpasswd -a ${USER} docker
sudo usermod -aG docker $USER
```

## **Build Procedure**

### **Step 1: Clone the Repository**
```
cd /home
git clone --recurse-submodules https://github.com/sonic-net/sonic-buildimage.git
```

**Note**:
At the time of the build, the master branch was pointing to the following commit:
```
Commit: 9293b8c52aab66f1660db05b9198bb0f7b836c97
Author: Brad House <brad@brad-house.com>
Date: Fri Dec 6 04:41:45 2024 -0500
Message: [yang] LOGGER missing require_manual_refresh (#20969)
```

###  **Step 2: Load the overlay Kernel Module**
```
sudo modprobe overlay
```

### **Step 3: Navigate to the Source Directory**
```
cd sonic-buildimage
```

## **Step 4: Apply the Necessary Patch**

Apply the below patch to avoid mount point errors.
```
$~/sonic-buildimage$ git diff Makefile.work
diff --git a/Makefile.work b/Makefile.work
index fe14eebae..9c4f806cc 100644
--- a/Makefile.work
+++ b/Makefile.work
@@ -393,7 +393,7 @@ endif
DOCKER_RUN += -v /var/run/march/docker.sock:/var/run/docker.sock
DOCKER_RUN += -v /var/run/march/docker.pid:/var/run/docker.pid
DOCKER_RUN += -v /var/run/march/docker:/var/run/docker
- DOCKER_RUN += -v $(DOCKER_DATA_ROOT_FOR_MULTIARCH):/var/lib/docker
+ DOCKER_RUN += -v $(DOCKER_DATA_ROOT_FOR_MULTIARCH):/var/lib/march/docker
SONIC_USERFACL_DOCKERD_FOR_MULTIARCH := setfacl -m user:$(USER):rw /var/run/march/docker.sock
```

###  **Step 5: Initialize the Repository**
```
make init
```

### **Step 6: Configure for the Build**
```
NOJESSIE=1 NOSTRETCH=1 NOBUSTER=1 NOBULLSEYE=1 BLDENV=bookworm DOCKER_BUILDKIT=0 KEEP_SLAVE_ON=yes SONIC_BUILD_JOBS=4 make configure PLATFORM=marvell PLATFORM_ARCH=arm64
```

### **Step 7: Build the ARM64 Image**
```
Execute Arm64 build using QEMU emulator environment.  
NOJESSIE=1 NOSTRETCH=1 NOBUSTER=1 NOBULLSEYE=1 BLDENV=bookworm MULTIARCH_QEMU_ENVIRON=y make SONIC_BUILD_JOBS=4 target/sonic-marvell-arm64.bin
```
## **Expected Errors and Workarounds**

### **Error at \`libnl\`**
```
$ NOJESSIE=1 NOSTRETCH=1 NOBUSTER=1 NOBULLSEYE=1 SONIC_BUILD_JOBS=4 /usr/bin/time -v make target/sonic-marvell-arm64.bin 2>&1 | tee make_target_arm64.log
.....
libtool: link: gcc -g -O2 -ffile-prefix-map=/sonic/src/libnl3/libnl3-3.7.0=. -fstack-protector-strong -Wformat -Werror=format-security -Wl,-z -Wl,relro -o tests/check-direct tests/check_direct-check-direct.o li
b/.libs/libnl-3.a lib/.libs/libnl-nf-3.a lib/.libs/libnl-genl-3.a lib/.libs/libnl-route-3.a tests/.libs/libnl-test-util.a /sonic/src/libnl3/libnl3-3.7.0/lib/.libs/libnl-nf-3.a /sonic/src/libnl3/libnl3-3.7.0/lib/
.libs/libnl-genl-3.a /sonic/src/libnl3/libnl3-3.7.0/lib/.libs/libnl-route-3.a /sonic/src/libnl3/libnl3-3.7.0/lib/.libs/libnl-3.a -lcheck_pic -lrt -lm -lsubunit -lpthread
make[4]: Leaving directory '/sonic/src/libnl3/libnl3-3.7.0'
/usr/bin/make check-TESTS
make[4]: Entering directory '/sonic/src/libnl3/libnl3-3.7.0'
make[5]: Entering directory '/sonic/src/libnl3/libnl3-3.7.0'
PASS: tests/check-direct
FAIL: tests/check-all
===================================
libnl 3.7.0: ./test-suite.log
===================================
.....
make[5]: *** [Makefile:6814: test-suite.log] Error 1
make[5]: Leaving directory '/sonic/src/libnl3/libnl3-3.7.0'
```
**Analysis**
This is an expected failure as per [Issue #361](https://github.com/thom311/libnl/issues/361). Exclude \`make check-TESTS\` during the libnl build.

**Workaround**
Edit \`Makefile.in\` to exclude \`make check-TESTS\`.
```
$~/arm-testing/sonic-buildimage/src/libnl3/libnl3-3.7.0$ git diff Makefile.in
diff --git a/Makefile.in b/Makefile.in
index 787a4d7..81512ab 100644
--- a/Makefile.in
+++ b/Makefile.in
@@ -7138,7 +7138,7 @@ distcleancheck: distclean
               exit 1; } >&2
 check-am: all-am
        $(MAKE) $(AM_MAKEFLAGS) $(check_PROGRAMS) $(check_LTLIBRARIES)
-       $(MAKE) $(AM_MAKEFLAGS) check-TESTS
+       $(MAKE) $(AM_MAKEFLAGS)
 check: check-am
 all-am: Makefile $(PROGRAMS) $(LTLIBRARIES) $(MANS) $(DATA) $(HEADERS)
 install-binPROGRAMS: install-libLTLIBRARIES
@@ -7808,7 +7808,7 @@ uninstall-man: uninstall-man8
 .MAKE: check-am install-am install-strip
 
 .PHONY: CTAGS GTAGS TAGS all all-am am--depfiles am--refresh check \
-       check-TESTS check-am clean clean-binPROGRAMS \
+       check-am clean clean-binPROGRAMS \
        clean-checkLTLIBRARIES clean-checkPROGRAMS clean-cscope \
        clean-generic clean-libLTLIBRARIES clean-libtool \
        clean-noinstLTLIBRARIES clean-noinstPROGRAMS \
$~/arm-testing/sonic-buildimage/src/libnl3/libnl3-3.7.0$
```
### **Error at \`iptables\`**
```
make[4]: Entering directory '/sonic/src/iptables/iptables-1.8.9'^M
/usr/bin/make check-TESTS^M
make[5]: Entering directory '/sonic/src/iptables/iptables-1.8.9'^M
make[6]: Entering directory '/sonic/src/iptables/iptables-1.8.9'^M
SKIP: iptables/tests/shell/run-tests.sh^M
SKIP: iptables-test.py^M
FAIL: xlate-test.py^M
…
============================================================================^M
Testsuite summary for iptables 1.8.9^M
============================================================================^M
# TOTAL: 3^M
# PASS: 0^M
# SKIP: 2^M
# XFAIL: 0^M
# FAIL: 1^M
# XPASS: 0^M
# ERROR: 0^M
============================================================================^M
See ./test-suite.log^M
============================================================================^M
make[6]: *** [Makefile:798: test-suite.log] Error 1^M
make[6]: Leaving directory '/sonic/src/iptables/iptables-1.8.9'^M
```
**Workaround**
Edit \`Makefile.in\` to exclude \`make check-TESTS\`.
```
$~/arm-testing/sonic-buildimage_bookworm/src/iptables/iptables-1.8.9$ git diff Makefile.in
check-am: all-am
- $(MAKE) $(AM_MAKEFLAGS) check-TESTS
+ $(MAKE) $(AM_MAKEFLAGS)
check: check-recursive
all-am: Makefile $(DATA) config.h
installdirs: installdirs-recursive
@@ -1267,7 +1266,7 @@ uninstall-am: uninstall-dist_confDATA
.MAKE: $(am__recursive_targets) all check-am install-am install-strip

.PHONY: $(am__recursive_targets) CTAGS GTAGS TAGS all all-am \
- am--refresh check check-TESTS check-am clean clean-cscope \
+ am--refresh check check-am clean clean-cscope \
clean-generic clean-libtool cscope cscopelist-am ctags \
ctags-am dist dist-all dist-bzip2 dist-gzip dist-lzip \
dist-shar dist-tarZ dist-xz dist-zip dist-zstd distcheck \
```
#### **Details about the Errors**

The external repo's **libnl** and **iptables** are being pulled by the sonic build system during the make target. As per [https://github.com/thom311/libnl/issues/361](https://github.com/thom311/libnl/issues/361), this seems to be a known issue. This test suite needs a network namespace with required permissions to run otherwise it was suggested to skip the tests to gracefully handle the error as these unit tests are for developers of this library itself.

### **Error at \`docker-base-bookworm.gz\`**
```
[ FAIL LOG START ] [ target/docker-base-bookworm.gz ]
Build start time: Tue Dec 10 20:28:24 UTC 2024
[ REASON ] : target/docker-base-bookworm.gz does not exist NON-EXISTENT PREREQUISITES: docker-start
[ FLAGS FILE ] : []
[ FLAGS DEPENDS ] : []
[ FLAGS DIFF ] : []
Unable to find image 'cat:latest' locally
docker: Error response from daemon: pull access denied for cat, repository does not exist or may require 'docker login': denied: requested access to the resource is denied.
........
---> Running in 0e3e004683bd
WARNING: apt does not have a stable CLI interface. Use with caution in scripts.
Ign:1 http://debian-archive.trafficmanager.net/debian jessie InRelease
Ign:2 http://debian-archive.trafficmanager.net/debian jessie-updates InRelease
….
E: The repository 'http://debian-archive.trafficmanager.net/debian jessie-backports Release' does not have a Release file.
[ FAIL LOG END ] [ target/docker-base-bookworm.gz ]
make: *** [slave.mk:1136: target/docker-base-bookworm.gz] Error 1
```
**Workaround**
Edit \`scripts/prepare\_docker\_buildinfo.sh\` to set the correct \`DISTRO\`.
```
$~/arm-testing/sonic-buildimage_bookworm$ git diff scripts/prepare_docker_buildinfo.sh
diff --git a/scripts/prepare_docker_buildinfo.sh b/scripts/prepare_docker_buildinfo.sh
index 6dfd63bdd..50de56caf 100755
--- a/scripts/prepare_docker_buildinfo.sh
+++ b/scripts/prepare_docker_buildinfo.sh

@@ -35,8 +34,13 @@ if [ -z "$DISTRO" ]; then
DOCKER_BASE_IMAGE=$(grep "^FROM" $DOCKERFILE | head -n 1 | awk '{print $2}')
DISTRO=$(docker run --rm --entrypoint "" $DOCKER_BASE_IMAGE cat /etc/os-release | grep VERSION_CODENAME | cut -d= -f2)
if [ -z "$DISTRO" ]; then
DISTRO=$(docker run --rm --entrypoint "" $DOCKER_BASE_IMAGE cat /etc/apt/sources.list | grep deb.debian.org | awk '{print $3}')
[ -z "$DISTRO" ] && DISTRO=jessie
+ DISTRO=bookworm
fi
fi
```

## **Console Output**
```
$~/arm-testing/sonic-buildimage_bookworm$ NOJESSIE=1 NOSTRETCH=1 NOBUSTER=1 NOBULLSEYE=1 BLDENV=bookworm SONIC_BUILD_JOBS=4 MULTIARCH_QEMU_ENVIRON=y /usr/bin/time -v make target/sonic-marvell-arm64.bin 2>&1 | tee make_target.log
....
[ 01 ] [ target/debs/bookworm/linux-headers-6.1.0-22-2-common_6.1.94-1_all.deb ]
[ 02 ] [ target/debs/bookworm/syncd_1.0.0_arm64.deb ]
[ 01 ] [ target/debs/bookworm/linux-headers-6.1.0-22-2-common_6.1.94-1_all.deb ]
[ 02 ] [ target/debs/bookworm/swss_1.0.0_arm64.deb ]
[ 01 ] [ target/sonic-marvell-arm64.bin__m/usr/bin/basename: missing operand
Try '/usr/bin/basename --help' for more information.
make[1]: Leaving directory '/home/arm-testing/sonic-buildimage_bookworm'
BLDENV=bookworm make -f Makefile.work docker-cleanup
make[1]: Entering directory '/home/arm-testing/sonic-buildimage_bookworm'
"SECURE_UPGRADE_PROD_SIGNING_TOOL": ""
~/arm-testing/sonic-buildimage_bookworm/src/sonic-build-hooks ~/arm-testing/sonic-buildimage_bookworm
….
make[1]: Leaving directory '/home/arm-testing/sonic-buildimage_bookworm'
Command being timed: "make target/sonic-marvell-arm64.bin"
User time (seconds): 9.88
System time (seconds): 3.37
Percent of CPU this job got: 0%
Elapsed (wall clock) time (h:mm:ss or m:ss): 23:20:36
Average shared text size (kbytes): 0
Average unshared data size (kbytes): 0
Average stack size (kbytes): 0
Average total size (kbytes): 0
Maximum resident set size (kbytes): 44128
Average resident set size (kbytes): 0
Major (requiring I/O) page faults: 1504
Minor (reclaiming a frame) page faults: 199473
Voluntary context switches: 47272
Involuntary context switches: 5605
Swaps: 0
File system inputs: 232600
File system outputs: 1168
Socket messages sent: 0
Socket messages received: 0
Signals delivered: 0
Page size (bytes): 4096
Exit status: 0
Thu Dec 26 07:26:41 AM PST 2024

```

**cat sonic-marvell-arm64.bin.log**
```
+ echo Success: Demo install image is ready in target/sonic-marvell-arm64.bin:
Success: Demo install image is ready in target/sonic-marvell-arm64.bin:
+ ls -l target/sonic-marvell-arm64.bin
-rw-r--r-- 1 user guser 907849710 Dec 26 15:25 target/sonic-marvell-arm64.bin
+ clean_up 0
+ rm -rf /tmp/tmp.gHMtQGcmFj
+ exit 0
Build end time: Thu Dec 26 15:25:52 UTC 2024
```

## **Reference Links**

- [#16204](https://github.com/sonic-net/sonic-buildimage/issues/16204)
