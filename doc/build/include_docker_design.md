# _INCLUDE_DOCKER: Composable Docker Feature System

## Overview

`_INCLUDE_DOCKER` is a build system extension that allows docker images to compose features from other docker images without duplicating code. It enables a docker target (e.g., `docker-sonic-vs`) to include programs, dependencies, and configuration from other docker images (e.g., `docker-lldp`) by referencing shared templates and leveraging Docker's multi-stage build contexts.

## Motivation

`docker-sonic-vs` is an all-in-one container that runs the entire SONiC stack in a single Docker container. It is used as:
- The DUT image for VS (Virtual Switch) KVM testing
- cSONiC neighbor containers in sonic-mgmt KVM testbeds (replacing cEOS)

Previously, adding a new feature (like LLDP) to docker-sonic-vs required:
1. Copying Dockerfile fragments manually
2. Duplicating supervisord program definitions
3. Manually tracking dependency packages and Python wheels
4. Keeping two copies of configuration in sync

This approach doesn't scale and leads to drift between the standalone docker (e.g., `docker-lldp`) and the composed version in `docker-sonic-vs`.

## Design

### Architecture

```
┌─────────────────────────────────────┐
│         docker-sonic-vs.mk          │
│  _INCLUDE_DOCKER += docker-lldp.gz  │
│  _J2_INCLUDE_PATHS += ...           │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│         rules/functions             │
│  add_docker_feature macro           │
│  - Merges DEPENDS, PYTHON_WHEELS    │
│  - Adds build context references    │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│           slave.mk                  │
│  - Passes --build-context for each  │
│    included docker                  │
│  - Uses j2_include.py for template  │
│    resolution across directories    │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│    Source Docker (docker-lldp)       │
│  Dockerfile.common.j2  ─── shared   │
│  supervisord.conf.common.j2 ─ shared│
│  Dockerfile.j2 ──────── standalone  │
│  supervisord.conf.j2 ── standalone  │
└─────────────────────────────────────┘
```

### Key Components

#### 1. `_INCLUDE_DOCKER` Variable

Declared in the target's `.mk` file (e.g., `docker-sonic-vs.mk`):

```makefile
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_LLDP)
```

This tells the build system to compose `docker-lldp` features into `docker-sonic-vs`.

#### 2. `add_docker_feature` Macro (`rules/functions`)

Automatically merges build dependencies:

```makefile
define add_docker_feature
$(1)_DEPENDS += $$($(2)_DEPENDS)
$(1)_PYTHON_WHEELS += $$($(2)_PYTHON_WHEELS)
$(1)_DBG_DEPENDS += $$($(2)_DBG_DEPENDS)
$(1)_DBG_IMAGE_PACKAGES += $$($(2)_DBG_IMAGE_PACKAGES)
endef
```

Called for each entry in `_INCLUDE_DOCKER`:
```makefile
$(foreach dep,$($(1)_INCLUDE_DOCKER),\
  $(eval $(call add_docker_feature,$(1),$(dep))))
```

#### 3. `--build-context` in `slave.mk`

For each included docker, a `--build-context` argument is passed to `docker build`:

```makefile
# Build context for included docker features
$(foreach dep,$($(1)_INCLUDE_DOCKER),\
  --build-context $(notdir $(basename $(dep)))=$(DOCKERS_PATH)/$(notdir $(basename $(dep))))
```

This enables `COPY --from=<feature>` in the target Dockerfile without file copying.

#### 4. `_J2_INCLUDE_PATHS` and `j2_include.py`

Jinja2 templates need to resolve `{% include %}` directives across multiple directories. The `j2_include.py` wrapper extends the template search path:

```makefile
$(DOCKER_SONIC_VS)_J2_INCLUDE_PATHS += dockers/docker-lldp
```

`scripts/j2_include.py` wraps `sonic-cfggen` and adds these paths to Jinja2's `FileSystemLoader`, allowing templates like:

```jinja2
{% include "supervisord.conf.common.j2" %}
```

to resolve from the source docker's directory.

#### 5. Shared Templates (`.common.j2`)

Each composable feature provides shared templates:

- **`Dockerfile.common.j2`**: Package installation, file copying — included via `COPY --from=`
- **`supervisord.conf.common.j2`**: Program definitions with configurable priority and dependent_startup

Example `supervisord.conf.common.j2`:
```jinja2
{% set _pbase = lldp_priority_base | default(3) %}

[program:lldpd]
command=/usr/sbin/lldpd -d -I Ethernet[0-9]*,eth0 -C eth0
priority={{ _pbase }}
autostart=false
autorestart=false
dependent_startup=true
dependent_startup_wait_for=start:exited
```

The target sets variables before including:
```jinja2
{% set lldp_priority_base = 28 %}
{% include "supervisord.conf.lldp.j2" %}
```

### supervisord dependent_startup

docker-sonic-vs uses `supervisord-dependent-startup` to chain service startup:

```
[eventlistener:dependent-startup]
command=python3 -m supervisord_dependent_startup
autostart=true
autorestart=unexpected
startretries=0
exitcodes=0,3
events=PROCESS_STATE
buffer_size=1024
```

Services declare dependencies:
```
start.sh (dependent_startup=true)
  └→ lldpd (wait_for=start:exited)
      └→ waitfor_lldp_ready (wait_for=lldpd:running)
          └→ lldp-syncd (wait_for=waitfor_lldp_ready:exited)
              └→ lldpmgrd (wait_for=lldp-syncd:running)
```

## Usage

### Adding a New Feature to docker-sonic-vs

1. Create shared templates in the source docker directory:
   - `Dockerfile.common.j2` — packages and files to install
   - `supervisord.conf.common.j2` — supervisord program definitions

2. Update the source docker's `Dockerfile.j2` and `supervisord.conf.j2` to include the common templates (avoid duplication).

3. In `docker-sonic-vs.mk`:
   ```makefile
   $(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_<FEATURE>)
   $(DOCKER_SONIC_VS)_J2_INCLUDE_PATHS += dockers/docker-<feature>
   ```

4. In `docker-sonic-vs/Dockerfile.j2`:
   ```dockerfile
   COPY --from=<feature> /relevant/files /destination/
   ```

5. In `docker-sonic-vs/supervisord.conf.j2`:
   ```jinja2
   {% set feature_priority_base = <N> %}
   {% include "supervisord.conf.common.j2" %}
   ```

### Example: LLDP

**`docker-sonic-vs.mk`**:
```makefile
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_LLDP)
$(DOCKER_SONIC_VS)_J2_INCLUDE_PATHS += dockers/docker-lldp
```

**`docker-sonic-vs/Dockerfile.j2`**:
```dockerfile
COPY --from=docker-lldp /usr/sbin/lldpd /usr/sbin/
COPY --from=docker-lldp /usr/bin/lldp* /usr/bin/
```

**`docker-sonic-vs/supervisord.conf.j2`**:
```jinja2
{% set lldp_priority_base = 28 %}
{% include "supervisord.conf.lldp.j2" %}
```

## Files Changed

| File | Purpose |
|---|---|
| `rules/functions` | `add_docker_feature` macro |
| `slave.mk` | `--build-context` and `j2_include.py` integration |
| `scripts/j2_include.py` | Jinja2 multi-directory template resolver |
| `dockers/docker-lldp/Dockerfile.common.j2` | Shared LLDP Dockerfile fragment |
| `dockers/docker-lldp/supervisord.conf.common.j2` | Shared LLDP supervisord programs |
| `platform/vs/docker-sonic-vs.mk` | `_INCLUDE_DOCKER += docker-lldp` |
| `platform/vs/docker-sonic-vs/Dockerfile.j2` | `COPY --from=docker-lldp` |
| `platform/vs/docker-sonic-vs/supervisord.conf.j2` | Include LLDP programs + dependent_startup |
| `platform/vs/docker-sonic-vs/start.sh` | Call `start-lldp.sh`, hostname fix |
