# SONiC Bazel Patterns In Detail

> [!warning]
> This document is a companion to the [`README.bazel.md`](./README.bazel.md).
> It expands on some of the patterns mentioned there, serving as a reference.
> It is not meant to be read from top to bottom, but rather linked from the README.

## Pre-requisites

Please ensure that you have [installed Bazel](https://bazel.build/install). We also recommend that you familiarize yourself with the basics of [Bazel terminology](https://bazel.build/reference/glossary), as well as going through [this introductory video course](https://www.youtube.com/playlist?list=PLLU28e_DRwdswrrZaNqnFFm9OawpxN4CB). Bazel has many complex moving parts, so building a solid foundation will ensure that the rest of this text makes sense.

In general, you should be comfortable with the Bazel terms "target", "label", "rule", "repository rule",  "module", "toolchain", and "action" before continuing.

## Bazel in SONiC Overview

The Bazel build for SONiC has tried to mirror that of the Make-based build:

- Most builds start from `sonic-buildimage`.
- Each component lives in `sonic-buildimage/src`, as a Git submodule.
- Each component is its own Bazel **module**, which can be built independently of the others.

> [!tip]
> To see how Bazel integrates with the rest of the SONiC build system, please refer to the [High Level Design document for Bazel](https://github.com/sonic-net/SONiC/pull/2396/changes#diff-0e576fff1d0a074c7d42c46f007869a6c26acc487769aeb7ece71d3bd168c61aR209).

## Airgapping The Bazel Build

Bazel introduces a few dependencies on external registries, such as GitHub repositories or Debian snapshot repositories.

If these dependencies are not acceptable, we recommend caching them in a service like Artifactory, or a CDN, and then modifying the [Bazel downloader configuration](https://aspect.build/blog/configuring-bazels-downloader) to re-write their URLs to your internal caches.

For instance, let's say we don't want to depend on `snapshot.debian.org`, and instead we host our own copy of the snapshot repositories in `artifacts.internal.dev/debian-snapshots`. We could write the following downloader configuration:

```
# ./.bazel_downloader.config
rewrite (snapshot\.debian\.org/.*) artifacts.internal.dev/debian-snapshots/$1
```

Then, we add this line in a `./.bazelrc` configuration file:

```
common --downloader_config=./.bazel_downloader.config
```

Please make sure you're familiar with [`.bazelrc` resolution](https://bazel.build/run/bazelrc#bazelrc-file-locations) to make sure this configuration is picked up.

Here is the list of internet sources we depend on:

| Name  | URL Prefixes |
|-------|--------------|
| Bazel module registries | `raw.githubusercontent.com/blorente/sonic-bazel-registry/`, `raw.githubusercontent.com/aspect-build/bazel-central-registry/`, `bcr.bazel.build/` |
| Bazel module archives | `github.com/`, `storage.googleapis.com/grpc-bazel-mirror/`, `mirror.bazel.build/`, `repo1.maven.org/maven2/` |
| Hermetic GCC toolchain | `github.com/blorente/gcc-builds/releases/download/` |
| Hermetic Python toolchain | `github.com/astral-sh/python-build-standalone/releases/download/` |
| `uv`, the pip dependency resolver | `github.com/astral-sh/uv/releases/download/`, `github.com/hermeticbuild/toml2json/releases/download/` |
| Prebuilt build tools (`tar`, `zstd`, `jq`, `yq`, `coreutils`, `crane`, `regctl`) | `github.com/hermeticbuild/bsdtar-prebuilt/releases/download/`, `github.com/aspect-build/bsdtar-prebuilt/releases/download/`, `github.com/aspect-build/zstd-prebuilt/releases/download/`, `github.com/stedolan/jq/releases/download/`, `github.com/mikefarah/yq/releases/download/`, `github.com/uutils/coreutils/releases/download/`, `github.com/google/go-containerregistry/releases/download/`, `github.com/regclient/regclient/releases/download/` |
| `buildifier` and `buildozer` | `github.com/bazelbuild/buildtools/releases/download/` |
| Host tools built from source (e.g. `m4`, `bison`, `flex`, used by `libnl3`) | `ftp.gnu.org/gnu/`, `ftpmirror.gnu.org/`, `mirrors.kernel.org/gnu/`, `github.com/westes/flex/releases/download/`, `sourceforge.net/projects/flex/files/`, `github.com/jmillikin/rules_m4/releases/download/`, `github.com/jmillikin/rules_bison/releases/download/` |
| Debian registry snapshots | `snapshot.debian.org/archive/debian/`, `snapshot.debian.org/archive/debian-security/` |
| Debian package sources | `debian-archive.trafficmanager.net/debian/pool/` |
| Python packages | `files.pythonhosted.org/packages/` |

See also:
- [Insulating Bazel Builds From The Internet](https://bazel.build/external/faq#how-do-i-insulate-my-builds-from-the-internet).
- [Configuring Bazel's Downloader](https://aspect.build/blog/configuring-bazels-downloader)

### Depending on Other Modules

To resolve dependencies between modules (e.g. `sonic-sysmgr` depends on `sonic-swss-common`), each module declares a normal `bazel_dep`:

```starlark
bazel_dep(name = "sonic-swss-common", version = "0.0.0-0bbc08794128e4e1d7df043c3e3f3c4cd3ec9750")
```

SONiC maintains its own Bazel registry, `blorente/sonic-bazel-registry` (soon to be `sonic-net/sonic-bazel-registry`). Everything that isn't a plain upstream BCR dependency lives in that external registry:

- First-party component modules (e.g. `sonic-build-infra`, `sonic-swss-common`, `sonic-sysmgr`), discovered automatically from `src/`.
- Modules we can't get from an upstream registry as-is, via the `OVERLAY_MODULES` list in that script. For instance, `com_github_openconfig_gnoi` is published this way because upstream hasn't migrated to bzlmod yet, and `libnl3` carries our own patch on top of the real upstream archive.
- Rulesets we need to patch from the Bazel Central Registry (e.g. `rules_go`). These are maintained directly in `sonic-bazel-registry` (there's no `sonic-buildimage`-side tooling for them), and are often temporary until the patches have been merged and released upstream.

[`publish_to_remote_registry.py`](/tools/bazel/registry/publish_to_remote_registry.py) holds the tooling to publish these modules into the SONiC Bazel registry.
It will publish first-party components automatically, plus the hardcoded list in `OVERLAY_MODULES`.

```
$ python3 tools/bazel/registry/publish_to_remote_registry.py
Cloned https://github.com/blorente/sonic-bazel-registry to /tmp/sonic-bazel-registry-erb4ycku
skip (already published): sonic-build-infra 0.0.0-d2283ad0aebb0eb78821920635e7f9ab54c6f146
skip (already published): sonic-swss-common 0.0.0-0bbc08794128e4e1d7df043c3e3f3c4cd3ec9750
new: libnl3 3.7.0.sonic-buildimage
Opened PR: https://github.com/blorente/sonic-bazel-registry/pull/3
```

Already-published `(name, version)` pairs are skipped, so it's safe to re-run after every commit.

With no arguments, the whole repo must be a clean checkout (including submodules) before publishing anything. To publish a single module instead, pass its path -- only that path needs to be clean, not the rest of the repo:

```
$ python3 tools/bazel/registry/publish_to_remote_registry.py src/libnl3
```

This also works via `bazel run` (note the `--` separating Bazel's own flags from the script's):

```sh
bazel run //tools/bazel/registry:publish_to_remote_registry -- src/libnl3
```

#### Unpinned Mode In Buildimage

*Inside* `sonic-buildimage`, that version string doesn't actually matter for modules under `src/`:
`sonic-buildimage`'s own `.bazelrc` unconditionally overrides every top-level `src/` module with [`--override_module`](https://bazel.build/reference/command-line-reference#common_options-flag--override_module),
so Bazel builds it from `src/` instead of resolving it through any registry at all.

These configurations come from [`tools/bazel/root-unpinned-modules-config.bazelrc`](/tools/bazel/root-unpinned-modules-config.bazelrc), which is generated and kept up-to-date by [`tools/bazel/registry/root_config_test.py`](/tools/bazel/registry/root_config_test.py).

#### Unpinned Mode In Submodules

By default, building a submodule standalone (e.g. `cd src/sonic-swss-common && bazel build ...`) resolves its dependencies at their real, *pinned* versions from `sonic-bazel-registry`.
To instead build against your own local, uncommitted checkout of one of those dependencies (e.g. testing a `sonic-build-infra` change together with `sonic-swss-common` before publishing it), pass `--config=unpinned-<name>`:

```sh
cd src/sonic-swss-common
bazel build --config=unpinned-sonic-build-infra ...
```

This applies `--override_module=sonic-build-infra=<path-to-src/sonic-build-infra>`, the same mechanism the root `.bazelrc` uses unconditionally for every top-level `src/` module.

> [!warning]
> Please see [`tools/bazel/submodule-config.bazelrc`](/tools/bazel/submodule-config.bazelrc) for an explanation of the gotchas.
> This mechanism is subject to change as we learn more about the development experience requirements from the SONiC community.

### `src/sonic-build-infra`

There is one new, Bazel-specific module: `sonic-build-infra`. This module holds any common build infrastructure code that could be needed by more than one component. For instance, this module holds:

- The definition of the gcc toolchain that every SONiC project should be compiled with, in `sonic-build-infra/toolchains`.
- A common list of Debian modules that modules need.
- Bazel rules to handle `swig`, in `sonic-build-infra/swig`.
- Build rules to generate debug images, such as a rule to strip the debug symbols off of a binary and expose them to Bazel.
- ...

In general, the rule is: If more than one module should use it, we should put it in `sonic-build-infra`.

## SONiC Bazel Patterns

> That's all great, but how do I _do_ things in Bazel?
>   -- You, probably

This section outlines the different patterns we've found when migrating SONiC code to use Bazel. They are presented as loose collection of examples. If you want to see how an end-to-end migration looks, head to [the end-to-end example](#end-to-end-example-migrating-docker-sysmgr).

### Creating Component Containers

This is the main entry point to Bazel. A component container is what we request from the command line, and what will eventually be deployed to a switch. You can find an example of one such container in [`dockers/docker-sysmgr/BUILD.bazel`](/dockers/docker-sysmgr/BUILD.bazel).

In Bazel, we use `rules_oci` to build [OCI Images](https://github.com/opencontainers/image-spec) without Docker. We model containers as lists of layers, where each layer is a `tar` archive:

```starlark
tar(                                # From `@tar.bzl`
	name = "loose_files",
	...
)

oci_image(
	name = "example_image",
	layers = [
		":loose_files",
		"@external_repo//pkg:dist_package",
		...
	],
)
```

As layers represent changes over the root file system, the tars we use must mirror that.

We use `mtree` to be able to place files in arbitrary locations, including `/`, as well as change permissions:

```starlark
tar(
    name = "loose_files",
    srcs = [
	    # Files in the filesystem, siblings to this `BUILD.bazel` file
        "critical_processes",
        "supervisord.conf",
        
        # Another target in the repository that creates a single file
        "//config_gen:config_status_gen",
    ],
    mtree = [
        "./etc/supervisor/conf.d/supervisord.conf time=0 type=file content=$(location :supervisord.conf)",
        "./etc/supervisor/critical_processes time=0 type=file content=$(location :critical_processes)",
        "./var/sonic type=dir time=0",
        "./var/sonic/config_status time=0 type=file content=$(location //config_gen:config_status_gen)",
    ],
)

oci_image(
	name = "example_image",
	layers = [
		":loose_files", # Add it as a layer
		...
	],
)
```

Sometimes, we need some advanced setup, such as `passwd`. In those cases, it's worth looking into `rules_distroless` , which includes utilities to create these special layers (e.g. [`passwd` in `rules_distroless`](https://github.com/bazel-contrib/rules_distroless/blob/4ca17c0969c229b93e1934df390e00f6d6ea620e/distroless/private/passwd.bzl#L10)):

```starlark
load("@rules_distroless//distroless:defs.bzl", "passwd")

passwd(
    name = "passwd",
    entries = [
        {
            "uid": 0,
            "gid": 0,
            "home": "/root",
            "shell": "/bin/bash",
            "username": "root",
        },
    ],
)
```

### Translating Debian Dependencies

SONiC relies on apt-installed dependencies in two places: As build-time dependencies, when installing them in the slave container, and at runtime, when installing them in the component containers.

In both cases, we use `rules_distroless` to fetch packages from a Debian snapshot. `rules_distroless` will resolve dependencies, unpack them, and overlay a Bazel build on top of them so that we can use them within our targets.

The full, centralized list of packages lives in the `apt.install` block of [`sonic-build-infra`'s `MODULE.bazel`](/src/sonic-build-infra/MODULE.bazel). Modules that depend on `sonic-build-infra` re-export this list (via the `trixie` hub repo), so they don't have to declare their own apt dependencies.

This will create appropriate Bazel targets for all the relevant parts of the Debian package:

```
➜ bazel query @trixie//libc6-dev:all
@trixie//libc6-dev:control
@trixie//libc6-dev:data
@trixie//libc6-dev:libc6
@trixie//libc6-dev:libc6-dev
```

#### Adding Debian packages to Images

This fits nicely with the structure of Debian packages. Their `data` sections were designed to be unpacked directly in a system's root, so they can serve directly as a layer:

```starlark
oci_image(
	name = "example_image",
	layers = [
		"@trixie//libc6-dev:data", # Add it as a layer directly
		...
	],
)
```

If your image needs several dependencies, but you want to keep them in a single image, we can use the `rules_distroless` rule `flatten` :

```starlark
flatten(
    name = "apt_deps",
    deduplicate = True,
    tars = [
	    # Note the lack of `:data`. `flatten` is smart enough to figure it out.
        "@trixie//libdbus-1-3",
        "@trixie//libdbus-c++-1-0v5",
        "@trixie//libprotobuf32t64",
    ],
)

oci_image(
	name = "example_image",
	layers = [
		":apt_deps", # Add all dependencies as a single layer
		...
	],
)
```
#### Depending on Debian packages during the build

> [!warning]
> While fetching **build** dependencies from `apt` is handy, it is not the ideal way of handling external dependencies in Bazel.
> 
> For one, dependencies fetched from `apt` are hardcoded to a single Debian snapshot may depend on host libraries themselves, such as a particular version of `libstdc++`.
> 
> Please see [this document](/tools/bazel/docs/import-external-projects.md) for alternatives and guidance on handling external dependencies.

Let's say our binary `:foo` depends on `libhiredis` being installed and present in the system. Outside Bazel, what we would do is install the `libhiredis-dev` package, which will install the required dynamic libraries and headers. For SONiC, this happens when we create the slave container.

`rules_distroless` allows us to translate this pattern easily into Bazel. First, we list `libhiredis-dev` as a dependency we want to fetch from `apt`:

```starlark
# MODULE.bazel
apt.install(
    dependency_set = "trixie",
    packages = [
        "libhiredis-dev",
        ...
    ],
    suites = [
        "trixie",
        "trixie-updates",
        "trixie-security",
    ],
)
```

This will automatically create the `@trixie/libhiredis-dev` bazel module, along with the handy `@trixie//libhiredis-dev:libc` target. This is a target that bundles `libhiredis`'s `so` files and headers:

```starlark
➜ bazel query --output label_kind '@trixie//libhiredis-dev/...'
alias rule @trixie//libhiredis-dev:control
alias rule @trixie//libhiredis-dev:data
alias rule @trixie//libhiredis-dev:libhiredis
filegroup rule @trixie//libhiredis-dev:libhiredis-dev
filegroup rule @trixie//libhiredis-dev/amd64:amd64
alias rule @trixie//libhiredis-dev/amd64:control
alias rule @trixie//libhiredis-dev/amd64:data
filegroup rule @trixie//libhiredis-dev/arm64:arm64
alias rule @trixie//libhiredis-dev/arm64:control
alias rule @trixie//libhiredis-dev/arm64:data
```

So, to make it available to `:foo`, we should be able to depend on it:

```starlark
cc_binary(
	name = "foo",
	deps = [
		"@trixie//libhiredis-dev:libhiredis",
	],
)
```

As `rules_distroless` creates standard Bazel `rules_cc` targets, this new dependency will work just as well as if we'd had vendored the source code from `libhiredis`.

### Sanitizers and Other Build Toggles

`sonic-build-infra` defines a stable interface to toggle sanitizers and other boolean build knobs, in [`config/constants.bzl`](/src/sonic-build-infra/config/constants.bzl):

```starlark
# src/sonic-build-infra/config/constants.bzl

BOOLEAN_TOGGLES = [
    "asan",
    "tsan",
    "usan",
    "gcov",
]
```

For each entry, [`config/BUILD.bazel`](/src/sonic-build-infra/config/BUILD.bazel) generates a `bool_flag` (defaulting to `False`) and a matching `config_setting` named `<toggle>_enabled`, both of which are re-exported from the repository root for convenience. This means any target can depend on `@sonic_build_infra//:<toggle>_enabled` without reaching into the `config` package.

A component opts into a toggle by `select()`-ing on the corresponding `_enabled` setting, usually from its own `flags.bzl`:

```starlark
# src/sonic-sysmgr/flags.bzl

CFLAGS_ASAN = select({
    "@sonic_build_infra//:asan_enabled": ["-fsanitize=address"],
    "//conditions:default": [],
})

LDFLAGS_ASAN = select({
    "@sonic_build_infra//:asan_enabled": ["-lasan"],
    "//conditions:default": [],
})
```

...and threading the resulting flags into the relevant `cxxopts`/`linkopts`:

```starlark
# src/sonic-sysmgr/rebootbackend/BUILD.bazel

cc_binary(
    name = "rebootbackend",
    cxxopts = DBGFLAGS + CFLAGS_COMMON + CFLAGS_ASAN,
    linkopts = LDFLAGS_ASAN,
    ...
)
```

Today, only `asan` is actually consumed this way (in `sonic-sysmgr`). The `tsan`, `usan`, and `gcov` toggles exist in `BOOLEAN_TOGGLES` but no component wires them into its flags yet; follow the `asan` pattern above to add support for one.

### Debug Builds

Separately from the toggles above, `sonic-build-infra` exposes [`@sonic_build_infra//:debug_enabled`](/src/sonic-build-infra/config/BUILD.bazel), a `config_setting` that is true whenever Bazel is invoked with `--compilation_mode=dbg` (i.e. `bazel build -c dbg ...`):

```starlark
# src/sonic-build-infra/config/BUILD.bazel

config_setting(
    name = "debug_enabled",
    values = {
        "compilation_mode": "dbg",
    },
    visibility = ["//visibility:public"],
)
```

Components `select()` on it to swap in more debug-friendly flags, instead of adding a dedicated build toggle:

```starlark
# src/sonic-swss-common/bazel/flags.bzl

DBGFLAGS = select({
    "@sonic_build_infra//:debug_enabled": [
        "-ggdb",
        "-gdwarf-5",
    ],
    "//conditions:default": ["-g"],
})
```

### Generating SWIG Bindings

We use [SWIG](https://www.swig.org/) to generate Python and Go bindings from C++ libraries, such as `libswsscommon` in [`src/sonic-swss-common`](/src/sonic-swss-common). `sonic-build-infra` wraps it in three rules, in [`swig/defs.bzl`](/src/sonic-build-infra/swig/defs.bzl):

- `swig_lib_deb` extracts SWIG's own library files (`swig.swg` and the language-specific typemaps) out of the `swig` apt package's data tar, so `SWIG_LIB` can be pointed at a hermetic directory instead of a system install.
- `swig_gen` runs `swig -c++ -python` against a `.i` interface file and a library's headers, producing a C++ wrapper and a Python module.
- `swig_gen_go` is the Go equivalent, running `swig -go -cgo -c++`, and produces a Go file, a C++ wrapper, and its header.

The `swig` binary itself comes from the Bazel Central Registry (`bazel_dep(name = "swig", version = "4.3.0.bcr.2")` in [`sonic-build-infra`'s `MODULE.bazel`](/src/sonic-build-infra/MODULE.bazel)), not from `apt`. Both generator rules pull their `-I` include paths automatically from the `CcInfo` of `deps`/`hdrs`, so you don't hand-write them:

```starlark
# src/sonic-swss-common/pyext/BUILD.bazel

swig_lib_deb(
    name = "swig_lib",
    data = "@trixie//swig",
    strip_prefix = "usr/share/swig/4.3.0",
)

swig_gen(
    name = "swsscommon_pyy",
    hdrs = ["//:hdrs"],
    cpp_out = "py3/swsscommon_wrap.cpp",
    interface = "swsscommon.i",
    python_out = "py3/swsscommon.py",
    swig_lib = ":swig_lib",
)
```

> [!warning]
> `swig_lib_deb`'s `strip_prefix` must match the SWIG version resolved from `apt` (`usr/share/swig/<version>`); bump it whenever the `swig` package updates. This is also an instance of the apt-binary antipattern flagged in [Depending on Debian packages during the build](#depending-on-debian-packages-during-the-build): we should eventually source these library files from the same hermetic `@swig` repository the binary comes from, instead of `@trixie//swig`.

From there, the generated files are consumed like any other target. For Python, compile the generated `.cpp` into a `cc_binary` extension module, and wrap it with `py_native_library`:

```starlark
# src/sonic-swss-common/pyext/BUILD.bazel

cc_binary(
    name = "_swsscommon",
    srcs = ["py3/swsscommon_wrap.cpp"],  # Generated by SWIG
    copts = ["-fvisibility=hidden", "-fPIC"],
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "//:libswsscommon",
        "@trixie//python3-dev:python3",
    ],
)

py_native_library(
    name = "swsscommon",
    cc_deps = ["//:common", "@trixie//python3-dev:python3"],
    native_py = "py3/swsscommon.py",  # Generated by SWIG
    native_so = ":_swsscommon",
    visibility = ["//visibility:public"],
)
```

For Go, [`src/sonic-swss-common/goext/BUILD.bazel`](/src/sonic-swss-common/goext/BUILD.bazel) follows the same shape, but compiles the generated `.cxx`/`.h` into a `cc_library` first — `rules_go`'s CGO support doesn't propagate transitive include paths from `cc_library` deps on its own — then embeds that into a `go_library` via `cdeps`:

```starlark
# src/sonic-swss-common/goext/BUILD.bazel

swig_gen_go(
    name = "swsscommon_go_gen",
    hdrs = ["//:hdrs"],
    cxx_out = "swsscommon_wrap.cxx",
    go_out = "swsscommon.go",
    hdr_out = "swsscommon_wrap.h",
    interface = "swsscommon.i",
    swig_lib = ":swig_lib",
)

cc_library(
    name = "swsscommon_wrap_cc",
    srcs = ["swsscommon_wrap.cxx"],  # Generated by swig_gen_go
    hdrs = ["swsscommon_wrap.h"],  # Generated by swig_gen_go
    copts = ["-fPIC"],
    deps = [":swsscommon_cgo_wrapper"],
)

go_library(
    name = "swsscommon",
    srcs = ["swsscommon.go"],  # Generated by swig_gen_go
    cdeps = [":swsscommon_wrap_cc", ":swsscommon_cgo_wrapper"],
    cgo = True,
    importpath = "github.com/sonic-net/sonic-swss-common/goext",
    visibility = ["//visibility:public"],
)
```

## End-to-end Example: Migrating `docker-sysmgr`

Let's see how all of this works on a long, end-to-end example.

We're going to retrace the steps we took to generate [this PR](https://github.com/sonic-net/sonic-buildimage/pull/28005), systematically migrating `docker-sysmgr` to Bazel. Our goal, by the end, is to have an OCI image for `docker-sysmgr` that we can just load into our switches.

The rough process is as follows:

1. Understand the current build, with the goal to identify the components we need to migrate
2. One by one, migrate those components, adding them to the registry as we go.
3. Write the container images
### 1. Understanding The Current Build

We should start from `rules/docker-sysmgr.mk`. If we open it, we'll see the following line:

```
$(DOCKER_SYSMGR)_DEPENDS += $(SYSMGR)
```

So, we need to somehow depend on `$(SYSMGR)`. A little more spelunking takes us to `rules/sysmgr.mk`, which has the following lines:

```
$(SYSMGR)_SRC_PATH = $(SRC_PATH)/sonic-sysmgr
$(SYSMGR)_DEPENDS += $(LIBSWSSCOMMON_DEV) \
 			$(PROTOBUF) $(PROTOBUF_LITE) $(PROTOBUF_DEV) $(PROTOBUF_COMPILER)

$(SYSMGR)_RDEPENDS += $(LIBSWSSCOMMON) $(PROTOBUF)
```

From here, we learn three things:
- `*_SRC_PATH`: Which source code we actually need to build.
- `*_DEPENDS`: Which dependencies we need _when we are building_ `sysmgr`. These do not need to be in the container, but we'll have to find a way to feed them to Bazel.
- `*_RDEPENDS`: Which dependencies _need to be in the container_ in order for `sysmgr` to run. These will have to be bundled in the `oci_image` somehow.

For now, we're just going to jot down the modules we need to migrate:

```
ModulesToMigrate:
 - src/sonic-sysmgr      # Because it's the source we're interested in
   
Modules to look into
 - LIBSWSSCOMMON{,DEV}
   
We may need to migrate:
 - Protobuf
```

We may or may not need to migrate protobuf, depending on a series of factors. For instance, [protobuf is already in the BCR](https://registry-preview.bazel.build/modules/protobuf), which means we can pull it from there at build time if we find a suitable version. There is also a Debian package for protobuf, which means we could put _that package_ in the final runtime container.

Now, we repeat the process with `rules/swss-common.mk`:

```
$(LIBSWSSCOMMON)_SRC_PATH = $(SRC_PATH)/sonic-swss-common
$(LIBSWSSCOMMON)_DEPENDS += $(LIBNL3_DEV) $(LIBNL_GENL3_DEV) \
                            $(LIBNL_ROUTE3_DEV) $(LIBNL_NF3_DEV) \
                            $(LIBNL_CLI_DEV) $(LIBYANG3_DEV) $(LIBYANG3) $(LIBYANG3_PY3)
$(LIBSWSSCOMMON)_RDEPENDS += $(LIBNL3) $(LIBNL_GENL3) \
                             $(LIBNL_ROUTE3) $(LIBNL_NF3) $(LIBNL_CLI) $(LIBYANG3)
$(LIBSWSSCOMMON)_WHEEL_DEPENDS += $(SONIC_YANG_MGMT_PY3) $(SONIC_YANG_MODELS_PY3) 
```

We add `src/sonic-swss-common` to our list of dependencies:

```
ModulesToMigrate:
 - src/sonic-sysmgr      # Because it's the source we're interested in
 - src/sonic-swss-common # Because it produces LIBSWSSCOMMON{,_DEV}
   
Modules to look into 
 - LIBNL3_*
 - LIBYANG3_*
   
We may need to migrate:
 - Protobuf
```

We follow this process again, and we add `src/libnl3` and `src/libyang3` to our list:
```
ModulesToMigrate:
 - src/sonic-sysmgr      # Because it's the source we're interested in
 - src/sonic-swss-common # Because it produces LIBSWSSCOMMON{,_DEV}
 - src/libyang3          # Dep of LIBSWSSCOMMON
 - src/libnl3            # Dep of LIBSWSSCOMMON
```

Luckily for us, the list of dependencies for `sysmgr` ends there.

We now have 4 targets that we need to migrate to Bazel. Also, because of the nature of our traversal, we have a topologically sorted list: Targets at the bottom don't depend on targets above them.

So, we just pick the bottom one `libnl3`, and start migrating. But, how do we do that?
### 2. Migrating Components To Bazel

> [!warning]
> In reality, we don't _have_ to migrate all of these dependencies for now. As per the HLD, we're going to consume the base images from Make, and the base images already have these dependencies baked in.
> 
> However, we _will_ have to eventually migrate these components, so it's worth learning how to do it now.
#### Migrating a patched dependency: `libnl3`

If we open up `src/libnl3`, we see [the following Makefile](https://github.com/sonic-net/sonic-buildimage/blob/master/src/libnl3/Makefile):

```make

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the libnl3
	rm -rf ./libnl3-$(LIBNL3_VERSION_BASE)
	dget https://deb.debian.org/debian/pool/main/libn/libnl3/libnl3_$(LIBNL3_VERSION).dsc
	pushd libnl3-$(LIBNL3_VERSION_BASE)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patch series
	stg init
	stg import -s ../patch/series

ifeq ($(CROSS_BUILD_ENVIRON), y)
	DPKG_GENSYMBOLS_CHECK_LEVEL=0 dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DPKG_GENSYMBOLS_CHECK_LEVEL=0 dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
```

In simple terms, the rough operations are:
- Download the source code for `libnl3` at `LIBNL3_VERSION`.
- Apply the patches in `src/libnl3/patch`.
- Use `dpkg-buildpackage` to build each deb package.

So, this is a dependency that:
- We have to build from source (because we have to patch it), and
- Has a relatively simple build process.

With these constraints, [Method 3](/tools/bazel/docs/import-external-projects.md#method-3-port-the-dependency-into-bazel) on the guide to dealing with external dependencies is the most appropriate. Please read that section for instructions on how to structure the migration.

At the end, you we'll have well-formed Bazel project in `src/libnl3`, which can be:
- built directly (and successfully) with `cd src/libnl3 && bazel build ...`, and 
- imported into any other module that depends on it with `bazel_dep(name = "libnl3", version = "3.7.0")`, and used with `@libnl3//<target>`.

Some useful tips we used to migrate `libnl3`:
- Some dependencies were assumed to be in the system, like `flex` and `bison`. In the Make build, they are installed in the slave container. Luckily, [someone has already ported them to the BCR](https://registry-preview.bazel.build/modules/rules_flex), so we can just use that ([Method 1](/tools/bazel/docs/import-external-projects.md#method-1-pull-from-the-bazel-central-registry-bcr)).
- `libnl3` is built with autotools. Sometimes, autotools creates header files after configuring itself. Because we know exactly which configuration we're going to need (from the call to `dpkg` in the SONiC Makefile), we can predict what these header files will contain, and just write them as constants into the build (see `@libnl3_src//:defs_h` for an example). Same goes for `pkg-config` files.

#### Next: `libyang3`

`libyang3` has a very similar pattern as `libnl3`: It's a dependency that we patch, that we have to replicate the build for. 

We will not dwell on libyang3, as it doesn't add anything interesting.

#### Next: `sonic-swss-common`

`sonic-swss-common` is not a large library, but it is the first time we're migrating first party code, so it's worth diving into.

If we look into the current build (`rules/swss-common.mk`), we'll see that `sonic-swss-common` exports many packages, including `LIBSWSSCOMMON`, `LIBSWSSCOMMON_DEV`, `PYTHON3_SWSSCOMMON`, and `SONIC_DB_CLI`.

Luckily, as per `rules/sonic-sysmgr.mk`, we only need two of them:
- `LIBSWSSCOMMON_DEV` as a `_DEPS`, or build time dependency, and 
- `LIBSWSSCOMMON` in the final container, as a runtime dependency.

Since `LIBSWSSCOMMON_DEV` is only needed at build time, it's very likely that it'll be replaced by the Bazel graph in the Bazel world. Therefore, **let's just focus on `LIBSWSSCOMMON`**.

##### Migrating `LIBSWSSCOMMON`

All these targets correspond with `.deb` archives. This one is no different:

```make
# rules/swss-common.mk

LIBSWSSCOMMON = $(LIBSWSSCOMMON_NAME)_$(LIBSWSSCOMMON_VERSION)_$(CONFIGURED_ARCH).deb
```

Resolving the variables, we get that:
```make
LIBSWSSCOMMON = libswsscommon_1.0.0_amd64.deb
```

And we know from the same file, that the source code is in `src/sonic-swss-common`:

```make
# rules/swss-common.mk

$(LIBSWSSCOMMON)_SRC_PATH = $(SRC_PATH)/sonic-swss-common
```

So, we have to search how `src/sonic-swss-common` builds `libswsscommon_1.0.0_amd64.deb`.

Digging around in `src/sonic-swss-common/debian`, we see that the `libswsscommon` package has three important files:

```
➜ ls src/sonic-swss-common/debian
...
libswsscommon.dirs
libswsscommon.install
```

These will tell us what we expect to see in the packages. Reading them is left as an exercise to the reader, but a useful summary would be that `libswsscommon`:

1. Builds all shared libraries, and places them in `/usr/lib/<arch>/lib` . Reading a little deeper, we see that there is only one shared library: `/usr/lib/<arch>/lib/libswsscommon.so`.
2. Places all the Lua files in `/usr/share/swss`.
3. Places the database config in `/var/run/redis/sonic-db/database_config.json`.
4. Builds the `swssloglevel` binary, and places it in `/usr/bin/swssloglevel`

Looking at this overall structure, we can start to sketch the tars (from `tar.bzl`) we'll need. We'll need one tar to hold loose files (1, 3, and 4), and another to hold all the Lua files (2). Lastly, we'll `flatten` them to expose them as a single package:

```starlark
# src/sonic-swss-common/BUILD.bazel

# All lua files
tar(
    name = "dist_lua",
    srcs = [":all_luas"],
    mtree = ":dist_lua_mtree",
)

# Loose files: shared library, config, and binaries
tar(
    name = "dist_loose",
    ...
)

# Final package: What the consumers will see
flatten(
    name = "libswsscommon_pkg",
    tars = [
        ":dist_lua",
        ":dist_loose",
    ],
    visibility = ["//visibility:public"],
)
```

Let's now fill in the blanks, starting with the easiest: The redis database configuration.

To add a single file to a tar, we can just add it to the `srcs`, and place it in the right place with `mtree`:

```starlark
# src/sonic-swss-common/BUILD.bazel

tar(
    name = "dist_loose",
    srcs = [
		"//common:database_config.json",
    ],
    mtree = [
       "var/run/redis/sonic-db/database_config.json type=file content=$(location //common:database_config.json)",
    ],
)
```

Easy.

Next, we'll work on the Lua headers. It looks like we don't do anything special. Specifically, it looks like **we don't generate any of these luas at build time**. This is great, because it means we can just glob them in a `filegroup`:

```starlark
# src/sonic-swss-common/common/BUILD.bazel

filegroup(
    name = "luas",
    srcs = glob(["*.lua"]),
)
```

And include them in the tar, with some mtree mutations to make sure they end up in the right place:

```starlark
mtree_spec(
    name = "dist_lua_mtree_base",
    srcs = [":all_luas"],
)

mtree_mutate(
    name = "dist_lua_mtree",
    mtree = ":dist_lua_mtree_base",
    strip_prefix = "common",
    package_dir = "usr/share/swss",  # Place them in `/usr/share/swss`
)

tar(
    name = "dist_lua",
    srcs = [":all_luas"],
    mtree = ":dist_lua_mtree",
)
```

Next, we have to figure out the `swssloglevel` binary. Looking at the Make build, it looks like it's using `autotools`  to build it, and the configuration lives in `common/Makefile.am`:

```make
common_swssloglevel_CXXFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CODE_COVERAGE_CXXFLAGS)
common_swssloglevel_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CODE_COVERAGE_CPPFLAGS)
common_swssloglevel_LDADD = common/libswsscommon.la $(CODE_COVERAGE_LIBS)
common_swssloglevel_LDFLAGS = -Wl,-z,now $(LDFLAGS)
```

Unfortunately, it does look like we need the static library version of `libswsscommon` to compile it (`libswsscommon.la`), so we'll put it aside for now.

Lastly, we need to figure out how to build `libswsscommon` itself.

This is a process that requires reading and understanding the autotools build, and translating it to Bazel. It's hard to generalize, but the end result should be a `cc_binary` or `cc_shared_library` target that we can depend on. This is what we ended up with, after much experimentation:

```starlark
# Release-ready .so library. This should go in the tar.
# The specific flags of this target were the result of experimentation, please do not assume your library will need the same ones.
cc_binary(
    name = "libswsscommon_consolidated_base",
    linkshared = True,
    linkopts = [
        "-static-libstdc++",
        "-static-libgcc",
        # Allow undefined symbols from external runtime deps (hiredis, zmq, etc.)
        # These are resolved at runtime when the .so is loaded
        "-Wl,--allow-shlib-undefined",
        "-Wl,--undefined-version",
    ],
    deps = [":common"],
)

# Custom transition needed to make sure some compile flags are applied to the dependencies of `libswsscommon`.
alwayslink_cc_binary(
  name = "libswsscommon_consolidated.so",
  binary = "libswsscommon_consolidated_base",
)
```

And now we can place the library in the deployment tar:

```starlark
# src/sonic-swss-common/BUILD.bazel

tar(
    name = "dist_loose",
    srcs = [
	    ":libswsscommon_consolidated.so", # <==== NEW
		"//common:database_config.json",
    ],
    mtree = [
        "usr/lib/x86_64-linux-gnu/libswsscommon.so type=file content=$(location :libswsscommon_consolidated.so)", # <==== NEW
       "var/run/redis/sonic-db/database_config.json type=file content=$(location //common:database_config.json)",
    ],
)
```

Incidentally, now that we can build `libswsscommmon`, we can build `swssloglevel` as well:
```starlark

# Bazel dependency. `swssloglevel` and other bazel targets should depend on this
cc_library(
    name = "libswsscommon",
    hdrs = swss_common_hdrs,
    include_prefix = "swss",
    strip_include_prefix = "common",
    deps = [":common"],
)

cc_binary(
    name = "swssloglevel",
    srcs = ["//common:loglevel_srcs"],
    deps = ["libswsscommon"],
    linkopts = [
        "-Wl,-z,now",
    ],
    cxxopts = CXXFLAGS_COMMON,
    linkstatic = True,
)
```

And add it to the deployment tar:

```starlark
# src/sonic-swss-common/BUILD.bazel

tar(
    name = "dist_loose",
    srcs = [
	    ":libswsscommon_consolidated.so",
		"//common:database_config.json",
		":swssloglevel", # <==== NEW
    ],
    mtree = [
        "usr/lib/x86_64-linux-gnu/libswsscommon.so type=file content=$(location :libswsscommon_consolidated.so)",
       "var/run/redis/sonic-db/database_config.json type=file content=$(location //common:database_config.json)",
       "usr/bin/swssloglevel type=file mode=0755 content=$(location :swssloglevel)", # <==== NEW
    ],
)

flatten(
    name = "libswsscommon_pkg",
    tars = [
        ":dist_lua",
        ":dist_loose",
    ],
    visibility = ["//visibility:public"],
)
```

And that's it! We have finally migrated `libswsscommon_pkg`. Please refer to [Method 3](/tools/bazel/docs/import-external-projects.md#method-3-port-the-dependency-into-bazel) to learn how to consume `libswsscommon_pkg` from the rest of the build.
##### A note about headers

When we're dealing with headers, we can probably get away with a `filegroup` that globs the sources, and pass that into a `tar`. The only potential problem are **generated headers**:

Headers like `config.h` and `defs.h` are generated at build time, so globbing the source files may not be enough. In our case, the header `cfg_schema.h` is generated at build time, and it depends on whether yang models are enabled for the build.

So, we need to take it into account when bundling the sources:

```starlark
# src/sonic-swss-common/common/BUILD.bazel

genrule(
	name = "cfg_schema_h",
	outs = ["cfg_schema.h"],
	cmd = "...", # Command replicating the Make functionality
)

# All library headers, including the generated cfg_schema.h.
filegroup(
    name = "hdrs",
    srcs = glob(
	    ["*.h", "*.hpp"], allow_empty = True,
	) + [
	    ":cfg_schema_h",
	],
)
```

#### Next: `sonic-sysmgr`

To migrate `sonic-sysmgr`, we're going to follow a process similar to `sonic-swss-common`. It does have its unique challenges and subtleties, but they're not terribly generalizable, and they are well-documented in the code itself in case you're curious.

For now, we're going to assume that it is building, and producing the deb archive `sysmgr_pkg`, which contains a single binary, `rebootbackend`:

```starlark
load("@rules_distroless//distroless:defs.bzl", "flatten")
load("@tar.bzl", "tar")
load("@sonic_build_infra//tar:sonic_deploy_tar.bzl", "sonic_deploy_tar")

sonic_deploy_tar(
    name = "sysmgr_pkg",
    srcs = ["//rebootbackend"],
    force_debug_build = True,
    binaries = {
      "./usr/bin/rebootbackend type=file": "//rebootbackend:rebootbackend",
    },
    visibility = ["//visibility:public"],
)
```

Note that we're using `sonic_deploy_tar`, instead of `tar`. This is a SONiC-specific rule that will make sure all binaries are stripped of debug information, and it will expose it in a convenient way to create debug images later. To learn more about `sonic_deploy_tar`, please read [its definition in `sonic-build-infra`](/src/sonic-build-infra/tar/sonic_deploy_tar.bzl).
### 3. Writing Container Images

We've done the hard part, migrating all the actual components. Now, it's time to assemble the container image so that we can load it.

For that, we're going to use [`rules_oci`](https://registry.bazel.build/modules/rules_oci). In particular, we're going to use `oci_image` to generate the actual images. Here is the rough shape:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
	name = "docker-sysmgr",
	base = [...],                 # Base image
	entrypoint = ["..."]          # Entry point, from the Dockerfile
    tars = [...],                 # Layers of tars
    ...
)
```

Let's go step by step, from top to bottom.

#### Base Image

The `base` attribute corresponds with the base image we want to run. As per the [Bazel HLD](https://github.com/sonic-net/SONiC/pull/2396), this base image will be imported from the Make-based build. You can read about _how_ we interface with Make in `README.buildsystem.md`, but for now we can assume that, **by the time Bazel runs, the base image will be placed in the `target/*` directory**.

So, we only have to capture it in the `BUILD.bazel` file, as any other source file:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

load("//tools/bazel/oci:docker_archive_to_oci.bzl", "docker_archive_to_oci_layout")

docker_archive_to_oci_layout(
    name = "config_engine_base_layout",
    src = "//:target/docker-config-engine-trixie.gz",
)
```

`docker_archive_to_oci_layout` is important here. The Make-based build system produces Docker images, which do not conform to the OCI standard. We have written a small script to take those images, transform them, and lay them out on disk in a format that `rules_oci` can consume.

With that done, we can fill out the `base` attribute:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
	name = "docker-sysmgr",
	base = [":config_engine_base_layout"],
	...
)
```

#### Layers

Now, we're going to assemble the layers. For now, we're going to replicate the behaviour of the current build system, which means we have to look into the `Dockerfile.j2`. Going line by line:

```jinja
# dockers/docker-sysmgr/Dockerfile.j2

{% from "dockers/dockerfile-macros.j2" import install_debian_packages, install_python_wheels, copy_files, rsync_from_builder_stage %}
ARG BASE=docker-config-engine-trixie-{{DOCKER_USERNAME}}:{{DOCKER_USERTAG}}

FROM $BASE AS base
...
```

We set the base image, which we've already done, as well as import some useful functions. Nothing new here.

```Dockerfile
# dockers/docker-sysmgr/Dockerfile.j2

ARG docker_container_name
RUN [ -f /etc/rsyslog.conf ] && sed -ri "s/%syslogtag%/$docker_container_name#%syslogtag%/;" /etc/rsyslog.conf
```

This is currently a no-op, since the base image doesn't have the `/etc/rsyslog.conf` file. We're happy to ignore it for now.

```Dockerfile
## Make apt-get non-interactive
ENV DEBIAN_FRONTEND=noninteractive
```

This sets an environment variable, which we can model in `oci_image` directly:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
	name = "docker-sysmgr",
	base = [":config_engine_base_layout"],
	env = {
        "DEBIAN_FRONTEND": "noninteractive",
    },
	...
)
```

Next, we move onto the apt installs:

```Dockerfile
RUN apt-get update        && \
    apt-get install -f -y    \
        libdbus-1-3          \
        libdbus-c++-1-0v5
```

These are the apt packages that our image needs at runtime. Referring back to [Translating Debian Dependencies](#translating-debian-dependencies), we know that we can just refer to the `@trixie` dependencies we imported via `rules_distroless`.

```starlark
# dockers/docker-sysmgr/BUILD.bazel

flatten(                   # Flatten them so that all deps are one layer.
    name = "apt_deps",
    deduplicate = True,
    tars = [
        "@trixie//libdbus-1-3",
        "@trixie//libdbus-c++-1-0v5",

        # We can add protobuf here, since it is listed in `RDEPS`
        "@trixie//libprotobuf32t64",
    ],
)

oci_image(
	name = "docker-sysmgr",
	base = [":config_engine_base_layout"],
	env = {
        "DEBIAN_FRONTEND": "noninteractive",
    },
    tars = [
	    ":apt_deps",
    ],
)
```

Next, we install the first party dependencies -- the things we spent so long migrating:

```jinja
{% if docker_sysmgr_debs.strip() -%}
# Copy locally-built Debian package dependencies
{{ copy_files("debs/", docker_sysmgr_debs.split(' '), "/debs/") }}

# Install locally-built Debian packages and implicitly install their dependencies
{{ install_debian_packages(docker_sysmgr_debs.split(' ')) }}
{%- endif %}
```

Since we have the tars already, we should be able to plop them in `tars`. Just make sure they're all `sonic_deploy_tars`:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
	name = "docker-sysmgr",
	base = [":config_engine_base_layout"],
	env = {
        "DEBIAN_FRONTEND": "noninteractive",
    },
    tars = [
	    ":apt_deps",
        # libswsscommon (and libnl3/libyang3) come from the config-engine base.
	    "@sonic_sysmgr//:sysmgr_pkg",
    ],
)
```

Next, we have a few loose source files:

```Dockerfile
# creating sonic conig_status file.
RUN mkdir -p /var/sonic
RUN echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status

COPY ["supervisord.conf", "/etc/supervisor/conf.d/"]
COPY ["critical_processes", "/etc/supervisor"]
```

These are easy to model using plain `tar`:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@tar.bzl", "tar")

write_file(
    name = "config_status_gen",
    out = "config_status",
    content = ["# Config files managed by sonic-config-engine"],
)

tar(
    name = "source_files",
    srcs = [
        "critical_processes",
        "supervisord.conf",
        ":config_status_gen",
    ],
    mtree = [
        "./etc/supervisor/conf.d/supervisord.conf time=0 type=file content=$(location :supervisord.conf)",
        "./etc/supervisor/critical_processes time=0 type=file content=$(location :critical_processes)",
        "./var/sonic type=dir time=0",
        "./var/sonic/config_status time=0 type=file content=$(location :config_status_gen)",
    ],
)

oci_image(
	name = "docker-sysmgr",
	base = [":config_engine_base_layout"],
	env = {
        "DEBIAN_FRONTEND": "noninteractive",
    },
    tars = [
	    ":apt_deps",
	    "@sonic_sysmgr//:sysmgr_pkg",
	    ":source_files",
    ],
)
```

And lastly, we set the entry point:

```Dockerfile
ENTRYPOINT ["/usr/local/bin/supervisord"]
```

Also easy to model in `rules_oci`:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
	name = "docker-sysmgr",
	base = [":config_engine_base_layout"],
    entrypoint = ["/usr/local/bin/supervisord"], # <==== New
	env = {
        "DEBIAN_FRONTEND": "noninteractive",
    },
    tars = [
	    ":apt_deps",
	    "@sonic_sysmgr//:sysmgr_pkg",
	    ":source_files",
    ],
)
```

And that's it! That's the whole image. Now `bazel build //dockers/docker-sysmgr:docker-sysmgr` should build a correct OCI image that you can move around.

However, there are a couple of niceties we can add.

#### Packaging images into `target/docker-sysmgr.gz` for Make interoperability

During the migration, Bazel will be called from Make. To make the transition easy, we need to tell Bazel to place the build artifacts where Make expects them.
In this case, Make expects the final image to be written to  `target/docker-sysmgr.gz`, as a gzipped `docker save` archive.

Getting there from an `oci_image` takes a chain of four targets, and every container needs that chain twice (once for the regular image, once for the debug image).
We have abstracted this process in a macro, [`sonic_docker_archive`](/tools/bazel/oci/sonic_docker_archive.bzl), so the actual content of the BUILD.bazel file is simple:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

load("//tools/bazel/oci:sonic_docker_archive.bzl", "sonic_docker_archive")

sonic_docker_archive(
    name = "docker-sysmgr.gz",
    image = ":docker-sysmgr",
)
```

Please read the macro's documentation for more information.

Now, Make can call Bazel with `bazel run //dockers/docker-sysmgr:write_docker-sysmgr.gz`, and Bazel will correctly place the image archive in `target/docker-sysmgr.gz`.

> [!warning]
> The repo tag is derived from `name`, and deliberately not configurable.
> `sonic_debian_extension.j2` works out which tag to re-tag from the `.gz` file name, so an archive tagged anything else fails to install.

#### Debug Images

To create a debug image (an image containing the debug symbols of the binaries in the image), we should lean heavily on the utilities in `src/sonic-build-infra`.

Specifically:
- If we wrap all relevant component archives in `sonic_deploy_tar` (like we do in `sonic-sysmgr/BUILD.bazel`),
- Then we should be able to derive a debug layer automatically with `debug_symbols_layer`:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

load("@sonic_build_infra//oci:debug_symbols_layer.bzl", "debug_symbols_layer")

debug_symbols_layer(
    name = "docker-sysmgr.debug_symbols",
    image = ":docker-sysmgr",
)
```

That target produces an OCI-compatible layer that contains the debug symbols for all the binaries in `docker-sysmgr`.

We can then create another `oci_image` for debugging, just like before:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
    name = "docker-sysmgr-dbg",
    base = ":docker-sysmgr",             # Note the base is docker-sysmgr
    tars = [
        "//tools/bazel:debug_utils_pkg", # A layer containing gdb, vim, etc.
        ":docker-sysmgr.debug_symbols",  # The debug symbols
    ],
    ...
)
```

The debug image is packaged for Make exactly like the regular one, through a second `sonic_docker_archive` call:

```starlark
# dockers/docker-sysmgr/BUILD.bazel

sonic_docker_archive(
    name = "docker-sysmgr-dbg.gz",
    image = ":docker-sysmgr-dbg",
)
```

This is what produces `target/docker-sysmgr-dbg.gz` and the `write_docker-sysmgr-dbg.gz` target that `slave.mk` runs for `SONIC_BAZEL_DBG_DOCKER_IMAGES`.

## Remote Caching

There are many remote cache backends available. Please see the [official documentation](https://bazel.build/remote/caching#cache-backend) for high-level guidance.
Here are some recommendations, depending on your constraints:

- Object Storage (e.g. Google Cloud Storage or AWS S3). Managed for you, cheap, and easy to set up. However, it may not be the most performant. Great if your team does not have the capacity to care for a live service. [Setup Instructions](https://bazel.build/remote/caching#cloud-storage).
- A server running nginx, backed by the filesystem. Probably the easiest to set up and quite performant, but hard to ensure properties like high availability, backups, and automatic cleanup. You will have to figure out how to make sure the cache doesn't grow unbounded. [Setup Instructions](https://bazel.build/remote/caching#nginx). You should probably only use this approach for experimentation, and then choose one of the alternatives below.
- A self-hosted OSS implementation (e.g. [bazel-remote](https://bazel.build/remote/caching#bazel-remote) or [Buildbarn](https://github.com/buildbarn)). Works similarly to nginx, but offers nice capabilities such as automated garbage collection. If your team is comfortable self-hosting critical services, and managing their resources (e.g. disk & ram allocation), this is likely a good option. Unless you know you're going to want [Remote Execution](https://bazel.build/remote/rbe) later, we recommend starting with `bazel-remote`.
- Managed cache providers (e.g. [Aspect Build](https://aspect.build/platform/remote-cache), [BuildBuddy](https://www.buildbuddy.io/remote-cache)). There is an ecosystem of companies that will help you set up a remote cache, and either manage it for you, or help you manage it. Different companies offer different products, and we encourage you to book demos with their teams. Furthermore, most companies will offer expert Bazel support in some form, which can be a great way to get you unblocked.

Once you have a remote caching backend, it's time to configure Bazel to use it.
Following [the official documentation](https://bazel.build/remote/caching#run-remote-cache), we see there are two possible flags: `--remote_cache`, and `--remote_upload_local_results`.

Our recommendation is a setup where CI is allowed to read and write from the cache, but local developers are only allowed to read from the cache.
The easiest way to accomplish this is to:

- In your repository's `.bazelrc`, set `--remote_cache=https://yourcache.com...`. This will be consumed by both CI and local developers.
- In that same file, set `--remote_upload_local_results=false`. With this, nobody will be able to write to the cache by default.
- In your CI, set `--remote_upload_local_results=true`, using either a command-line flag, a custom [bazelrc configuration](https://bazel.build/run/bazelrc), or a Bazel enhancer like the [Aspect CLI](https://aspect.build/docs/cli/overview). This will allow CI to write to the cache.
  - You may want to consider only enabling writes for a nightly CI job from `master` or `main`, and keeping them disabled for PR builds.
