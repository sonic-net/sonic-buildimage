# Bazel for SONiC: A Build Engineer Primer

> [!warning]
> This document is meant for folks actively changing the Bazel build of SONiC.
> If you just want to _use_ the build to generate artifacts,
> please refer to [The `SONIC_BAZEL_DOCKER_IMAGES` target](/README.buildsystem.md) in the regular build.

This document lists general guidelines and patterns we use in the Bazel build for SONiC.
It is meant to be read as a checklist, with each section expanded elsewhere.

## Pre-requisites

Please ensure that you have [installed Bazel](https://bazel.build/install).
We also recommend that you familiarize yourself with the basics of [Bazel terminology](https://bazel.build/reference/glossary), as well as going through [this introductory video course](https://www.youtube.com/playlist?list=PLLU28e_DRwdswrrZaNqnFFm9OawpxN4CB).
Bazel has many complex moving parts, so building a solid foundation will ensure that the rest of this text makes sense.

In general, you should be comfortable with the Bazel terms "target", "label", "rule", "repository rule", "Bazel module", "toolchain", and "action" before continuing.

## Bazel Migration Overview

The status of the Bazel migration can be seen in the [Bazel Migration Dashboard](https://sonic-migration.aspect.dev).
If you are working on a component, please tag @blorente in issues and PRs so that the dashboard reflects it.

When submitting a contribution, please limit the scope of the contribution to _at most_ one container at a time.
Please express clearly if any changes were needed to `sonic-build-infra`, and why.

### TODO(bazel-ready) Comments

Sometimes, it is not practical to write the perfect Bazel-idiomatic build for a target.
For instance, if two submodules depend on each other, and we want to merge them independently,
we'll have to leave some work unfinished.

In those cases, please use a comment that starts with `TODO(bazel-ready): <explanation of the hack and resolution steps>`,
to make sure we can track and fix these issues later.

### Keeping the Bazel Build Green

As per the HLD, we will establish a CI pipeline to ensure the build is green.
Until then, please run the [`./tools/bazel/test_working_targets.sh`](./tools/bazel/test_working_targets.sh) to test that all containers that are supposed to work continue to do so.

## Interaction With the Make-based Build System

Even when building with Bazel, Docker images for SONiC services are driven by the Make build system.
There are two mechanisms for this:

- The `BUILD_WITH_BAZEL_WHEN_AVAILABLE` Flag: A global flag that toggles whether every container that could be built with Bazel should be built with Bazel.
- The `SONIC_BAZEL_DOCKER_IMAGES` Make Target: A new target type that will use `bazel build` to build the containers, instead of Make. [Documentation](/README.buildsystem.md#sonic-bazel-docker-images).

To mark a container as buildable with Bazel, add it to `SONIC_BAZEL_DOCKER_IMAGES` only if `BUILD_WITH_BAZEL_WHEN_AVAILABLE` is enabled:

```makefile
# rules/docker-sysmgr.mk

ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),n)

# Usual Make-based build
...

else

# When BUILD_WITH_BAZEL_WHEN_AVAILABLE is enabled, build this docker with Bazel.
$(DOCKER_SYSMGR)_BAZEL_BASE += $(DOCKER_CONFIG_ENGINE_TRIXIE)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_SYSMGR)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_SYSMGR_DBG)

endif
```

`_BAZEL_BASE` lists the Make-built images the Bazel build consumes as a base layer; `slave.mk` turns those into prerequisites of the Bazel target.

## Bazel Rules Dependencies

SONiC maintains its own Bazel registry, `blorente/sonic-bazel-registry` (soon to be `sonic-net/sonic-bazel-registry`). Everything that isn't a plain upstream BCR dependency lives in that external registry:

- First-party component modules (e.g. `sonic-build-infra`, `sonic-swss-common`, `sonic-sysmgr`), discovered automatically from `src/`.
- Modules we can't get from an upstream registry as-is, via the `OVERLAY_MODULES` list in that script. For instance, `com_github_openconfig_gnoi` is published this way because upstream hasn't migrated to bzlmod yet, and `libnl3` carries our own patch on top of the real upstream archive.
- Rulesets we need to patch from the Bazel Central Registry (e.g. `rules_go`). These are maintained directly in `sonic-bazel-registry` (there's no `sonic-buildimage`-side tooling for them), and are often temporary until the patches have been merged and released upstream.

Please see [Depending on Other Modules](/tools/bazel/docs/patterns-detail.md#depending-on-other-modules) for instructions on how to maintain this registry.

### Unpinned Mode

For development inside `sonic-buildimage`, `sonic-buildimage`'s own `.bazelrc` unconditionally overrides them with
`--override_module` to build from `src/` tree, regardless of what version any consumer's `bazel_dep` declares.
See [`tools/bazel/root-unpinned-modules-config.bazelrc`](/tools/bazel/root-unpinned-modules-config.bazelrc), kept complete by [`tools/bazel/registry/root_config_test.py`](/tools/bazel/registry/root_config_test.py).

Each module still declares a real, externally-meaningful pinned version in its own `MODULE.bazel`, for when it's built standalone (or published) outside of `sonic-buildimage`.

You can find further documentation on how we handle Bazel dependencies in [Depending on Other Modules](/tools/bazel/docs/patterns-detail.md#depending-on-other-modules).

> [!tip]
> Each submodule (e.g. `src/sonic-swss-common`) has its own, similar, but *opt-in* mechanism: `--config=unpinned-<name>` configurations in [`tools/bazel/submodule-config.bazelrc`](/tools/bazel/submodule-config.bazelrc), kept complete by [`tools/bazel/registry/submodule_config_test.py`](/tools/bazel/registry/submodule_config_test.py). See [Unpinned Mode In Submodules](/tools/bazel/docs/patterns-detail.md#unpinned-mode-in-submodules) for more information.

## Debian Dependencies

We manage Debian dependencies through [`rules_distroless`](https://github.com/bazel-contrib/rules_distroless).

All dependencies are declared in [`sonic-build-infra`](/src/sonic-build-infra/MODULE.bazel), under the `apt.install` dependency sets. This is to ensure a centralized resolution we can query.

There are two sets:

- `sysroot`: the C/C++ toolchain's sysroot (`libc6-dev`, `libgcc-12-dev`, `libstdc++-12-dev`, `linux-libc-dev`). Kept separate so the toolchain can resolve without fetching the entire package closure.
- `trixie`: everything else, both runtime and build-time dependencies.

To add a new dependency, add it to that resolution list.
Note that the list is marked `# do not sort` and is grouped by consumer: add your package under the comment for the component that needs it, or start a new group.

```diff
 # src/sonic-build-infra/MODULE.bazel

 apt.install(
     dependency_set = "trixie",
     packages = [
         # do not sort
         ...
         # for docker-sysmgr
         "libdbus-1-3",
         "libdbus-c++-1-0v5",
         "libprotobuf32t64",
+
+        # for docker-mycomponent
+        "libmylib1",
     ],
     suites = [
         "trixie",
         "trixie-updates",
         "trixie-security",
     ],
 )
```

The resolved package set is recorded in `src/sonic-build-infra/MODULE.bazel.lock`, which Bazel updates on the next build.
Commit that change along with the `MODULE.bazel` edit.

There are two ways to use Debian dependencies:

To use them as **runtime dependencies** of containers, add them directly as a layer to the container layer list.
Each package in the hub repo is a `tar` target, so it can be used as a layer as-is:

```python
# dockers/docker-sysmgr/BUILD.bazel

oci_image(
    name = "docker-sysmgr",
    base = ":config_engine_base_layout",
    tars = [
        "@trixie//libdbus-1-3",
        "@trixie//libprotobuf32t64",
    ],
)
```

Every entry in `tars` becomes its own image layer.
If you want several dependencies to go on the same layer, use `flatten()` (see [`dockers/docker-sysmgr/BUILD.bazel`](/dockers/docker-sysmgr/BUILD.bazel) for an example).

Note that `deduplicate = True` is important when packages share files: Without it, `docker load` fails on duplicate paths.

To use them as build-time dependencies (e.g. to link against `uuid`), use the targets defined in the `-dev` packages.
A `-dev` package exposes a `cc_library`-like target named after the library, so the label form is `@trixie//<package>-dev:<library>`:

```python
# src/sonic-swss-common/BUILD.bazel

cc_library(
    ...
    deps = [
        "@trixie//uuid-dev:uuid",
    ],
)
```

Note that the target name is not always a mechanical suffix strip: `libyang-dev` provides `:libyang`, but `python3-dev` provides `:python3`.
If you are unsure, query the package repo:

```sh
bazel query --output=label_kind '@trixie//<package>-dev:all'
```

While fetching **build** dependencies from `apt` is handy, it is not the ideal way of handling external dependencies in Bazel.
 
For one, dependencies fetched from `apt` are hardcoded to a single Debian snapshot may depend on host libraries themselves, such as a particular version of `libstdc++`.

Also note that tools we use at build time should not be fetched as Debian dependencies, for the same reason. For instance, we shouldn't fetch `jq` from a Debian package, because we cannot predict where a given build will run and Debian packages are architecture-locked.

Please see [this document](/tools/bazel/docs/import-external-projects.md) for alternatives and guidance on handling external dependencies.

## Container Assembly

We use `rules_oci` to assemble containers: an `oci_image` target takes a `base` image and a list of `tar` layers, rather than a Dockerfile.
See [`dockers/docker-sysmgr/BUILD.bazel`](/dockers/docker-sysmgr/BUILD.bazel) for a full example, and [Creating Component Containers](/tools/bazel/docs/patterns-detail.md#creating-component-containers) for the details on `oci_image`, `tar`, and `mtree`.

### Debug Containers

Every Bazel-built container has a paired debug image (registered via `SONIC_BAZEL_DBG_DOCKER_IMAGES`, alongside `SONIC_BAZEL_DOCKER_IMAGES` in [Interaction With the Make-based Build System](#interaction-with-the-make-based-build-system)) that layers the binaries' stripped debug symbols, plus tools like `gdb`, on top of the regular image.

Packaging binaries with `sonic_deploy_tar`, instead of a plain `tar`, bundles their debug symbols automatically, which is what lets `sonic-build-infra` recover them later to build the debug image.

See [Debug Images](/tools/bazel/docs/patterns-detail.md#debug-images) for the concrete rules involved.

## C/C++

We use `rules_cc` to write C/C++ targets.

### Hermetic GCC Toolchain

We use a hermetic GCC toolchain, defined in [`sonic-build-infra`](/src/sonic-build-infra/toolchains/gcc/BUILD.bazel).

We should use it whenever we need to use GCC or binutils. Specifically, we should never rely on system-installed utilities (e.g. `/bin/gcc`, `/bin/readelf`).

That toolchain is built by hand, from [blorente/gcc-builds](https://github.com/blorente/gcc-builds).
We can build other versions if needed (e.g. to fit other versions of Debian when they come out).

Each registered toolchain only matches a target platform that declares a sysroot constraint (e.g. [`@sonic_build_infra//platforms:trixie`](/src/sonic-build-infra/platforms/BUILD.bazel)).
Every `.bazelrc` that registers `@sonic_build_infra//toolchains/gcc:all` (root, and each first-party `src/` module's own `.bazelrc`) sets this as the default target platform:

```
common --platforms=@sonic_build_infra//platforms:x86_64_trixie
common:aarch64 --platforms=@sonic_build_infra//platforms:aarch64_trixie
```

> [!warning]
> Without an explicit `--platforms=` selecting a `trixie` platform, *no* hermetic toolchain is compatible, and Bazel silently falls back to whatever C++ toolchain it auto-detects on the host instead of failing loudly.
> If you're invoking `bazel build`/`bazel test` directly (bypassing the checked-in `.bazelrc`), make sure `--platforms` is still set.

### aarch64 & Cross-Compilation
 
To build for aarch64, we must pass `--config=aarch64`.

Our hermetic toolchains are *host* toolchains (`exec == target == $cpu`), not real cross-compilation, so `--config=aarch64` only resolves a toolchain when Bazel itself is running on an aarch64 machine.

There is no fundametal reason why we can't have cross-compilation, we just haven't implemented it yet.

### Compiler and linker flags

Avoid hand-writing compiler and linker flags that reference files, such as `-L`, `-I`, `-isystem`, etc.
The paths you observe in a Bazel sandbox are not not stable, and it's easy to incorrecly rely on files being on disk when an action executes.

Either Bazel, `rules_distroless`, or `rules_cc` should add them automatically, since they have more context about where the files will be. If you find yourself reaching for one of these flags, please consider generalizing your solution into the appropriate ruleset.

### asan, tsan, and others

`sonic-build-infra` exposes each sanitizer (`asan`, `tsan`, `usan`) and `gcov` as a boolean build flag. Set the corresponding flag on the command line to enable it for a build:

```sh
bazel build --@sonic_build_infra//:asan=true //src/sonic-sysmgr/...
```

> [!note]
> Only `asan` is actually wired into a component today (in `sonic-sysmgr`). `tsan`, `usan`, and `gcov` are defined but not yet consumed anywhere.

See [Sanitizers and Other Build Toggles](/tools/bazel/docs/patterns-detail.md#sanitizers-and-other-build-toggles) for how this is implemented, and how to wire up a toggle for your component.

### Debug Builds

Pass `-c dbg` to build with `@sonic_build_infra//:debug_enabled` set, which components can use to select more debug-friendly flags (e.g. `-ggdb`) instead of their normal optimized ones:

```sh
bazel build -c dbg //src/sonic-swss-common/...
```

See [Debug Builds](/tools/bazel/docs/patterns-detail.md#debug-builds) for an example.

## Python

We use `rules_python` to handle Python targets, and `aspect_rules_py`'s `uv` extension to resolve pip dependencies.
As with [C/C++](#cc), we use a hermetic toolchain for Python, registered in the root [`MODULE.bazel`](/MODULE.bazel) (`python.toolchain(python_version = "3.11.6")`).
We never depend on the system's Python installation.

### Python Dependencies

Python dependencies are listed in [`sonic-build-infra//deps:pyproject.toml`](/src/sonic-build-infra/deps/pyproject.toml).
We should only use those dependencies.

They are fetched, at build time, via the `uv` extension block in `sonic-build-infra`'s [`MODULE.bazel`](/src/sonic-build-infra/MODULE.bazel).

The lockfile for dependencies, `uv.lock`, is generated manually by calling `deps/generate_lockfile.sh` in `@sonic-build-infra`. It does require to have `uv` installed.

Please see [`tools/bazel/registry/BUILD.bazel`](/tools/bazel/registry/BUILD.bazel) for an example usage of a Python dependency.

### Installing Python Dependencies Into Images

To install a Python dependency into an OCI image for a SONiC container,
we need to re-package files from the format they were packaged in (usually a `.whl`),
into the `site_packages` format.

To do that, `sonic-build-infra` defines a [`site_packages`](/src/sonic-build-infra/python/site_packages.bzl) macro you can use:

```starlark
site_packages(
    name = "site-packages",
    srcs = [
        "@pypi//j2cli",
        "@pypi//supervisor",
        "@pypi//supervisord_dependent_startup",
    ],
    visibility = [":__subpackages__"],
)
```

## Swig

`sonic-build-infra` wraps SWIG in three rules, in [`swig/defs.bzl`](/src/sonic-build-infra/swig/defs.bzl):

- `swig_lib_deb` extracts SWIG's own library files out of the `swig` apt package, so generation doesn't depend on
 a system-installed SWIG.
- `swig_gen` / `swig_gen_go` run `swig` against a `.i` interface file and a library's headers, producing a C++ wr
apper plus a Python or Go module. Both derive their `-I` include paths automatically from `deps`/`hdrs`, so you d
on't hand-write them.

The generated files are consumed like any other target: compile the C++ wrapper into a `cc_binary`/`cc_library`,
then wrap it for the target language (`py_native_library` for Python, `go_library` with `cdeps` for Go).

See [`src/sonic-swss-common/pyext/BUILD.bazel`](/src/sonic-swss-common/pyext/BUILD.bazel) and [`src/sonic-swss-common/goext/BUILD.bazel`](/src/sonic-swss-common/goext/BUILD.bazel) for full examples, and [Generating SWIG Bindings](/tools/bazel/docs/patterns-detail.md#generating-swig-bindings) for the details.

## Perl, Doxygen, and Other Tools

Sometimes, we need to execute binary tools as part of a build. For instance, the SAI headers are extracted with Doxygen.

These are the patterns we prefer to use when reaching for a tool, in this order:

1. An existing ruleset from the the Bazel Central Registry (e.g. `rules_m4` for libnl3, see [`src/libnl3/libnl3.BUILD`](/src/libnl3/libnl3.BUILD)).
2. An exsiting BCR module that serves that binary, wrapped in either a custom rule or a `genrule`.
3. Migrate the tool to build from source, and then switch to (2).
4. Build a hermetic version of the tool out of band and import it to the Bazel build, and then switch to (2).

This is similar to the way we handle library dependencies, see [Importing External Projects](/tools/bazel/docs/import-external-projects.md).

For a full example, see:
- How libnl3 uses `rules_m4`, `rules_bison`, and `rules_flex`, in [`src/libnl3/libnl3.BUILD`](/src/libnl3/libnl3.BUILD).
- How we handle Doxygen and Perl in SAI, in [`meta/BUILD.bazel`](https://github.com/thesayyn/SAI/blob/master/meta/BUILD.bazel).

## Tar

We use `tar.bzl` to build and manipulate `tar` files.
Usually, we'll use it one of two ways:

- `mutate`: useful to bulk-relocate a group of files into a specific directory layout, by stripping and re-rooting a path prefix. See [`libnl3-dev_headers`](/src/libnl3/libnl3.BUILD) for an example: it strips `include/` off every public header and re-roots them under `./usr/include/libnl3`.
- `mtree`: explicitly list, per file, where it lands and with what permissions. Useful when assembling complex layers, or when dealing with loose files like binaries or configuration files. See [Creating Component Containers](/tools/bazel/docs/patterns-detail.md#creating-component-containers) for examples.

## Generating `.deb` Packages

`sonic-build-infra` defines a [`sonic_deb`](/src/sonic-build-infra/deb/sonic_deb.bzl) macro that assembles a `.deb` from a pre-packaged data `tar`, deriving the target architecture from the target platform. It only supports the dpkg-required control fields plus `Depends`. We should extend the macro if a migrated package needs more (e.g. `Homepage`).

```starlark
# src/sonic-sysmgr/BUILD.bazel

sonic_deb(
    name = "sysmgr_deb",
    data = ":sysmgr_pkg",
    depends = ["libswsscommon"],
    description = "This package contains sysmgr service.",
    maintainer = "Runming Wu <runmingwu@google.com>",
    package = "sysmgr",
    version = "1.0.0",
    visibility = ["//visibility:public"],
)
```
