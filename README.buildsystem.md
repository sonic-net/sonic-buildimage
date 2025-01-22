# SONiC Buildimage Guide
## Overview
SONiC buildimage is a *GNU make* based environment for build process automation.
It consists of two main parts:
 * Backend - collection of makefiles and other helpers that define generic target groups, used by recipes
 * Frontend - collection of recipes, that define metadata for each build target

## Structure
File structure of SONiC Buildimage is as follows:  
```
sonic-buildimage/  
  Makefile  
  slave.mk  
  sonic-slave/  
    Dockerfile  
  rules/  
    config  
    functions
    recipe1.mk
    ..
  dockers/  
    docker1/  
      Dockerfile.template    
    ..  
  src/  
    submodule1/    
    ..  
    package1/  
      Makefile  
    ..  
  platform/  
    vendor1/  
    ..  
  target/  
    debs/  
    python-wheels/  
```
### Backend  
**Makefile**, **slave.mk** and **sonic-slave/Dockerfile** are the backend of buildimage.  
*slave.mk* is the actual makefile. It defines a set of rules for *target groups* (more on that later).
You can find a make rule for every target that is defined in recipe there.  
*Makefile* is a wrapper over sonic-slave docker image.  

Every part of build is executed in a docker container called sonic-slave, specifically crafted for this environment.
If build is started for the first time on a particular host, a new sonic-slave image will be built form *sonic-slave/Dockerfile* on the machine.
It might take some time, so be patient.
After that all subsequent make commands will be executed inside this container.
*Makefile* takes every target that is passed to make command and delegates it as an entry point to a container,
making process of running container transparent.  
  
### Frontend  
**rules/** has a collection of recipes for platform independent targets.
Every recipe is a file that describes a metadata of a specific target, that is needed for its build.  
You might find **rules/config** very useful, as it is a configuration file for a build system, which enables/disables some tweaks.  
**dockers/** directory is a place where you can find Dockerfiles for generic docker images.  
**src/** is a place where a source code for generic packages goes.
It has both submodules (simple case, just run dpkg-buildpackage to build),
and directories with more complicated components, that provide their own Makefiles.
**platform/** contains all vendor-specific recipes, submodules etc.  
Every **platform/[VENDOR]/** directory is a derived part of buildimage frontend, that defines rules and targets for a concrete vendor.  

### Build output
**target/** is basically a build output. You can find all build artifacts there.  

## Recipes and target groups
Now let's go over a definition of recipes and target groups.  
**Recipe** is a small makefile that defines a target and set of variables for building it.
If you want to add a new target to buildimage (.deb package or docker image), you have to create a recipe for this target.  
**Target group** is a set of targets that are built according to the same rules.
Every recipe sets a target group to which this target belongs.  

### Recipe example
Lets take a recipe for swsscommon as an example:  
```make
# libswsscommon package

LIBSWSSCOMMON = libswsscommon_1.0.0_amd64.deb
$(LIBSWSSCOMMON)_SRC_PATH = $(SRC_PATH)/sonic-swss-common
$(LIBSWSSCOMMON)_DEPENDS += $(LIBHIREDIS_DEV) $(LIBNL3_DEV) $(LIBNL_GENL3_DEV) \
                            $(LIBNL_ROUTE3_DEV) $(LIBNL_NF3_DEV) \
			    $(LIBNL_CLI_DEV)
$(LIBSWSSCOMMON)_RDEPENDS += $(LIBHIREDIS) $(LIBNL3) $(LIBNL_GENL3) \
                             $(LIBNL_ROUTE3) $(LIBNL_NF3) $(LIBNL_CLI)
SONIC_DPKG_DEBS += $(LIBSWSSCOMMON)

LIBSWSSCOMMON_DEV = libswsscommon-dev_1.0.0_amd64.deb
$(eval $(call add_derived_package,$(LIBSWSSCOMMON),$(LIBSWSSCOMMON_DEV)))
```
First we define our package swsscommon.  
Then we specify **SRC_PATH** (path to sources),
**DEPENDS** (build dependencies),
and **RDEPENDS** (runtime dependencies for docker installation).  
Then we add our target to SONIC_DPKG_DEBS target group.  
At the end we define a dev package for swsscommon and make it derived from main one.
Using **add_derived_package** just makes a deep copy of package's metadata, so that we don't have to repeat ourselves.  

### Target groups
**SONIC_DPKG_DEBS**  
Main target group for building .deb packages.
Define:  
```make
SOME_NEW_DEB = some_new_deb.deb # name of your package
$(SOME_NEW_DEB)_SRC_PATH = $(SRC_PATH)/project_name # path to directory with sources
$(SOME_NEW_DEB)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ... # build dependencies
$(SOME_NEW_DEB)_RDEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ... # runtime dependencies
SONIC_DPKG_DEBS += $(SOME_NEW_DEB) # add package to this target group
```

**SONIC_PYTHON_STDEB_DEBS**  
Same as above, but instead of building package using dpkg-buildpackage it executes `python setup.py --command-packages=stdeb.command bdist_deb`.
Define:  
```make
SOME_NEW_DEB = some_new_deb.deb # name of your package
$(SOME_NEW_DEB)_SRC_PATH = $(SRC_PATH)/project_name # path to directory with sources
$(SOME_NEW_DEB)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ... # build dependencies
$(SOME_NEW_DEB)_RDEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ... # runtime dependencies
SONIC_PYTHON_STDEB_DEBS += $(SOME_NEW_DEB) # add package to this target group
```

**SONIC_MAKE_DEBS**  
This is a bit more flexible case.
If you have to do some specific type of build or apply paths prior to build, just define your own Makefile and add it to buildimage.
Define:
```make
SOME_NEW_DEB = some_new_deb.deb # name of your package
$(SOME_NEW_DEB)_SRC_PATH = $(SRC_PATH)/project_name # path to directory with sources
$(SOME_NEW_DEB)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ... # build dependencies
$(SOME_NEW_DEB)_RDEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ... # runtime dependencies
SONIC_MAKE_DEBS += $(SOME_NEW_DEB) # add package to this target group
```

If some packages have to be built locally due to some legal issues or they are already prebuilt and available online, you might find next four target groups useful.  

**SONIC_COPY_DEBS**  
Those packages will be just copied from specified location on your machine.
Define:  
```make
SOME_NEW_DEB = some_new_deb.deb # name of your package
$(SOME_NEW_DEB)_PATH = path/to/some_new_deb.deb # path to file
SONIC_COPY_DEBS += $(SOME_NEW_DEB) # add package to this target group
```

**SONIC_COPY_FILES**  
Same as above, but applicable for regular files. Use when you need some regular files for installation in docker container.
Define:  
```make
SOME_NEW_FILE = some_new_file # name of your file
$(SOME_NEW_FILE)_PATH = path/to/some_new_file # path to file
SONIC_COPY_FILES += $(SOME_NEW_FILE) # add file to this target group
```

**SONIC_ONLINE_DEBS**  
Target group for debian packages that should be fetched from an online source.
Define:  
```make
SOME_NEW_DEB = some_new_deb.deb # name of your package
$(SOME_NEW_DEB)_URL = https://url/to/this/deb.deb # path to file # URL for downloading
SONIC_ONLINE_DEBS += $(SOME_NEW_DEB) # add file to this target group
```

**SONIC_ONLINE_FILES**  
Target group for regular files that should be fetched from an online source.
Define:  
```make
SOME_NEW_FILE = some_new_file # name of your file
$(SOME_NEW_FILE)_URL = https://url/to/this/file # URL for downloading
SONIC_ONLINE_FILES += $(SOME_NEW_FILE) # add file to this target group
```

Docker images also have their target groups.  
**SONIC_SIMPLE_DOCKER_IMAGES**  
As you see from a name of the group, it is intended to build a docker image from a regular Dockerfile.
Define:  
```make
SOME_DOCKER = some_docker.gz # name of your docker
$(SOME_DOCKER)_PATH = path/to/your/docker # path to your Dockerfile
SONIC_SIMPLE_DOCKER_IMAGES += $(SOME_DOCKER) # add docker to this group
```

**SONIC_DOCKER_IMAGES**  
This one is a bit more sophisticated. You can define debian packages from buildimage that will be installed to it, and corresponding Dockerfile will be dynamically generated from a template.
Define:  
```make
SOME_DOCKER = some_docker.gz # name of your docker
$(SOME_DOCKER)_PATH = path/to/your/docker # path to your Dockerfile
$(SOME_DOCKER)_DEPENDS += $(SOME_DEB1) $(SOME_DEB2) # .deb packages to install into image
$(SOME_DOCKER)_PYTHON_WHEELS += $(SOME_WHL1) $(SOME_WHL2) # python wheels to install into image
$(SOME_DOCKER)_LOAD_DOCKERS += $(SOME_OTHER_DOCkER) # docker image from which this one is built
SONIC_DOCKER_IMAGES += $(SOME_DOCKER) # add docker to this group
```

## Tips & Tricks
Although every target is built inside a sonic-slave container, which exits at the end of build, you can enter bash of sonic-slave using this command:
```
$ make sonic-slave-bash
```
It is very useful for debugging when you add a new target and facing some troubles.

sonic-slave environment is built only once, but if sonic-slave/Dockerfile was updated, you can rebuild it with this command:
```
$ make sonic-slave-build
```

One can print out all available targets by executing the following command:
```
$ make list
```

All target groups are used by one or another recipe, so use those recipes as a reference when adding new ones.

## Build debug dockers and debug SONiC installer image:
Using 'INSTALL_DEBUG_TOOLS=y' builds the debug image.

For example:
`INSTALL_DEBUG_TOOLS=y make target/sonic-broadcom.bin` 

* Builds debug docker images.
* Debug images carry a suffix of "-dbg"
    * e.g. target/docker-orchagent-dbg.gz
* target/sonic-broadcom.bin is built using debug docker images
    * Selective sources are archived and available under /src
    * An empty /debug dir is created for use during debug session.
    * All debug dockers are mounted with /src:ro and /debug:rw
    * Login banner will briefly describe these features.
  

_Note: target/sonic-broadcom.bin name is the same irrespective of built using debug or non-debug dockers._

_Recommend: Rename image built using INSTALL_DEBUG_TOOLS=y to mark it explicit. May be `mv target/sonic-broadcom.bin target/sonic-broadcom-dbg.bin`_

### Debug dockers
* Built with all available debug symbols.
* Installed with many basic packages that would be required for debugging
    * gdb
    * gdbserver
    * vim
    * strace
    * openssh-client
    * sshpass
* Loadable into any environment that supports docker
* Outside SONiC image, you may run the docker with `--entrypoint=/bin/bash` 
* Use -v to map any of your host directories
* To debug a core file in non-SONiC environment that supports docker
    * `docker load -i docker-<name>-dbg.gz`
    * copy your unzipped core file into ~/debug
    * `docker run -it --entrypoint=/bin/bash -v ~/debug:/debug <image id>`
    * `gdb /usr/bin/<your binary> -c /debug/<your core>`
    
### Debug SONiC image

* Install this image into the switch that supports this image.
	* For platform independent binary, you may use a debug image for virtual switch
* Open the archive in /src, if you would need source code for debugging
* Every debug enabled docker is mounted with /src as read-only
* The host has /debug dir and it is mapped into every debuggable docker as /debug with read-write permission.
* To debug a core
    * Copy core into /debug of host and unzip it
        * Feel free to create & use sub-dirs as needed.
        * Entire /debug is mounted inside docker
    * Get into the docker (`docker -it exec <name> bash`)
    * `gdb /usr/bin/<binary> -c /debug/<core file path>`
    * Use set-directory in gdb to map appropriate source dir from under /src
    * You may set gdb logs to go into /debug
* For live debugging
    * Use this as a regular switch
    * Get into docker, and you may
        * start process under dbg
        * attach gdb to running process
    * Set required source dir from under /src as needed
    * May use /debug to record all geb logs or any spew from debug session.
    
  ### To enhance debug dockers
  * Add to `<docker name>_DBG_IMAGE_PACKAGES`, additional debug tools that will be pre-installed during build.
	* e.g. `$(DOCKER_ORCHAGENT)_DBG_IMAGE_PACKAGES += perl`
	* Build will install these tools using "apt"
  * Add to `<docker name>_DBG_DEPENDS`, additional debug .deb packages that will be pre-installed during build.
	* e.g. `$(DOCKER_ORCHAGENT)_DBG_DEPENDS +=  $(SWSS_DBG) $(LIBSWSSCOMMON_DBG) $(LIBSAIREDIS_DBG)`
  * Add to `DBG_SRC_ARCHIVE`, the source dirs to archive
	* The source files (.c, .cpp, .h & .hpp) under this dir tree are archived.
	* e.g. rules/swss.mk has `DBG_SRC_ARCHIVE += swss`
