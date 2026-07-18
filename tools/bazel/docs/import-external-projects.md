# How to import external projects to the SONiC Build

In this document, we'll explain how to work with external projects in the SONiC Bazel build. This is useful when you want to make an external library work with Bazel, and it becomes vital if we need to maintain a series of patches to it (e.g. if we need to patch OpenSSL in response to a CVE).

There are four methods to integrate an external libraries into the build. They are presented from most maintainable to least maintainable.
## Method 1: Pull from the Bazel Central Registry (BCR)

Some of the libraries we need are very widely used. It's entirely possible that someone has already gone through the trouble of writing the Bazel build for that library. For instance, there exist Bazel wrappers for [`m4`](https://registry.bazel.build/modules/rules_m4), [`bison`](https://registry.bazel.build/modules/rules_bison) and [`flex`](https://registry.bazel.build/modules/rules_flex), as well as [OpenSSL](https://registry.bazel.build/modules/openssl).

When this is the case, we usually just need to add the appropriate dependency to a `MODULE.bazel` file:

```
bazel_dep(name = "openssl", version = "3.5.5.bcr.4")
```

Bazel will then look into the BCR to do dependency resolution, just like npm looks into the npm registry, and pip looks into PyPi. This is the best way to import a library, since we're usually building the resource from source, following Bazel best-principles.

If we need to apply custom patches to the dependency, we can do so in SONiC's Bazel Registry. This registry works exactly like the BCR, but it lives in in `<sonic-buildimage>/tools/bazel/registry` and is unique to the SONiC project. All SONiC projects are configured to look here before looking into the BCR, so we can override dependency versions here if we wish.

To do that, we create a new module in the SONiC Bazel Registry, and we give it a unique version:

```
# Example: patched version of rules_go v0.60.0
➜ pwd
/home/.../sonic-net/sonic-buildimage

➜ ls tools/bazel/registry/modules/rules_go
0.60.0.sonic-patched
metadata.json
```

Then, we can copy the contents of the BCR entry we want to start from. For instance, [this](https://github.com/bazelbuild/bazel-central-registry/tree/main/modules/rules_go/0.60.0) is the BCR entry for `rules_go` version 0.60.0, so we copy it here:

```
➜ ls tools/bazel/registry/modules/rules_go/
tools/bazel/registry/modules/rules_go/0.60.0.sonic-patched
├── MODULE.bazel
├── patches
└── source.json
# We don't need presumbit.yml, that's for BCR's own CI.
```

Then, we add the patches we need:

```
➜ ls tools/bazel/registry/modules/rules_go/
tools/bazel/registry/modules/rules_go/0.60.0.sonic-patched
├── MODULE.bazel
├── patches
│   ├── 0001_patch_forward_to_base_of_4574.patch
│   └── 0002_import_pr_4574.patch
└── source.json
```

And modify `source.json` to load the patches:

```
➜ cat tools/bazel/registry/modules/rules_go/0.60.0.sonic-patched/source.json
{
    "integrity": "<integrity>",
    "strip_prefix": "rules_go-0.60.0",
    "url": "https://github.com/bazel-contrib/rules_go/archive/refs/tags/v0.60.0.tar.gz",
    "patches": {
        "0001_patch_forward_to_base_of_4574.patch": "<integrity>",
        "0002_import_pr_4574.patch": "<integrity>"
    },
    "patch_strip": 1
}
```

> [!tip]
> You can generate integrity values with:
> `openssl dgst -sha256 -binary $FILE | openssl base64 -A`

Please read the [full documentation on Bazel Modules](https://bazel.build/external/registry) to learn more about these files.
### Tip: Use `--override_module` to make development easier

The workflow above explains how to integrate patches, but it doesn't explain how to _generate_ them.

We recommend the flag `--override_module` to make this easier ([docs](https://bazel.build/reference/command-line-reference#common_options-flag--override_module)):

- Clone the repository of the dependency you want, by looking into its `source.json`. For instance, for `rules_go`: [`source.json`](https://github.com/bazelbuild/bazel-central-registry/blob/main/modules/rules_go/0.60.0/source.json).
- Then, switch back to `sonic-buildimage`, and develop as normal. When you're ready to build a target, use the `--override_module` flag to tell Bazel to load your local copy of the dependency: `--override_module=rules_go=/home/.../path/to/rules_go`.
- Make changes in your local clone of the dependency as needed, until the build passes. Every time you make a change, Bazel will pick it up.
- Once your build passes in `sonic-buildimage`, you can head back to your local clone of the ruleset and extract the diff: `cd /home/.../path/to/rules_go && git diff > my_patch.patch`.

That will yield patch files ready to be copied into the SONiC Bazel Registry.
## Method 2: Pull a Debian archive using `rules_distroless`

This one is simple: If the dependency is available from Debian repositories, and you don't want to patch it, you should be able to import the `deb` directly through [`rules_distroless`](https://github.com/thesayyn/sonic-buildimage/blob/9033053ebfee08c63fc22ef414b9f8cd2b82c766/dockers/docker-base-bookworm/base_bookworm.MODULE.bazel#L1-L160).

Please see [`base_bookworm.MODULE.bazel`](dockers/docker-base-bookworm/base_bookworm.MODULE.bazel) for an example.
## Method 3: Port the dependency into Bazel

If you need to patch the dependency, but it is _not_ in the BCR (and therefore nobody has ported it to Bazel yet), then it may pay off to do it ourselves.

This method works best with small, well-understood libraries that don't change very often, such as [`libnl3`](/src/libnl3). The more complicated the build process for a particular dependency, the harder it will be to port it to Bazel.

To import a dependency into Bazel, we're going to work in two stages:
### First, we make the project build with Bazel as a standalone

We're going to forget about SONiC for a moment, and just try to make the specific library work with Bazel.

If we follow the example of `libnl3` this would entail:

1. Downloading the source code for `libnl3` into a local checkout.
2. Applying the changes that we want to the source code. Please also keep patch files of the changes, as we'll need them later.
3. Creating a new Bazel build. For most libraries, a single `MODULE.bazel` and `BUILD.bazel` files at the root of the repository should suffice. If you think you need to break down the `BUILD.bazel` into more parts, it's probably a sign that [Method 4](#method-4-build-the-dependency-out-of-band-and-import-it-into-bazel-as-an-opaque-archive) might be better suited.
4. Filling out the `BUILD.bazel` with the appropriate targets. For most libraries, one or more `cc_library` and `cc_binary` targets is all you'll need.

Of course, step 4 is an oversimplification -- every project is different, and their builds will require more or less care. In addition, this is the point where you may decide the project is not worth the hassle, and move onto [Method 4](#method-4-build-the-dependency-out-of-band-and-import-it-into-bazel-as-an-opaque-archive).

LLMs do a decent job at generating these `BUILD.bazel` files, but please double-check their output. Often, old projects that rely on e.g. autotools will have configuration flags that affect what compiler flags a target is built with, and the LLMs may not be aware of which configuration you want. If necessary, compare the flags produced by the current SONiC build with the ones Bazel uses, using [`--subcommands`](https://bazel.build/reference/command-line-reference#build-flag--subcommands).
### Then, we import the project into the SONiC build

Once we have a working Bazel build, we have to import it to SONiC. To do that, we're going to recreate the process we've just done (clone, apply the patches, then overlay the Bazel build) but within Bazel. For an example, please refer to [`src/libnl3`](/src/libnl3).

The structure boils down to three files:

- A file that holds the Bazel build for the dependency. If you're following the process above, it's the one BUILD file we had. This is just a regular file, and you can name it whatever you want. For `libnl3`, we named it `libnl3.BUILD`, and we got it by doing:

```starlark
$ cd sonic-buildimage
$ cp /home/../../path/to/libnl3/BUILD.bazel src/libnl3/libnl3.BUILD
```

- A `MODULE.bazel` file that imports the source code of the dependency via `http_archive`, applies whatever patches we need to the source, and overlays the above file. For `libnl3`:

```starlark
$ cd sonic-buildimage
$ cd src/libnl3
$ cat MODULE.bazel

... 
LIBNL_3_VERSION = "3.7.0"

# This is a custom repository rule because libnl3.BUILD needs access to the repository name.
# Usually, an `http_archive` is enough.

libnl3_src = use_repo_rule("//:libnl3_src.bzl", "libnl3_src")
libnl3_src(
    name = "libnl3_src",
    build_file_template = "//:libnl3.BUILD",
    patches = [
        "//:patch/0003-Adding-support-for-RTA_NH_ID-attribute.patch",
        "//:bazel_patches/fix-icmp6-mib-max-assert.patch",
    ],
    sha256 = "9fe43ccbeeea72c653bdcf8c93332583135cda46a79507bfd0a483bb57f65939",
    strip_prefix = "libnl-{}".format(LIBNL_3_VERSION),
    urls = ["http://debian-archive.trafficmanager.net/debian/pool/main/libn/libnl3/libnl3_{}.orig.tar.gz".format(LIBNL_3_VERSION)],
)

...
```

This will build the dependency correctly as the `@libnl_src` module. Because it imports it as an external module it will not be possible for anything outside of `src/libnl3` to consume it. To do that, we need the third piece:

- A `BUILD.bazel` file that exports the required targets. This file is just a list of `alias` targets that re-exports whatever we need from the build. For `libnl3`:
```starlark
alias(
    name = "libnl_3",
    actual = "@libnl3_src//:libnl_3",
    visibility = ["//visibility:public"],
)

alias(
    name = "libnl_genl_3",
    actual = "@libnl3_src//:libnl_genl_3",
    visibility = ["//visibility:public"],
)

... # Other targets
```

Now we have fully implemented `@libnl3`, a working Bazel build that downloads some source code, applies some patches, rebuilds, and surfaces the result in a useful way. But how does one depend on `src/libnl3` from other parts of `sonic-buildimage`?

To make it discoverable, we need to add it to the SONiC Bazel Registry:

```
$ cd sonic-buildimage
$ tree tools/bazel/registry/modules/libnl3/
tools/bazel/registry/modules/libnl3/
├── 3.7.0
│   ├── MODULE.bazel -> ../../../../../../src/libnl3/MODULE.bazel
│   └── source.json
└── metadata.json

$ cat tools/bazel/registry/modules/libnl3/3.7.0/source.json
{
  "type": "local_path",
  "path": "src/libnl3"
}
```

As you can see, `MODULE.bazel`is just a symlink to the `src/libnl3/MODULE.bazel` we just created, and `source.json` is just telling Bazel to look in a local directory for the actual implementation of the module.

Now, anything that needs `libnl3` can depend on it with `bazel_dep(name = "libnl3", version = "3.7.0")`, and use `@libnl3//:libnl_3` in its build, just like any other package from the BCR. 
## Method 4: Build the dependency out of band, and import it into Bazel as an opaque archive.

Sometimes, a dependency's build process is too convoluted, and it's not worth porting to Bazel. For instance, we may need to patch Python itself, a project famous for being hard to compile in the best of times.

In those cases, we should treat the dependency as opaque artifacts, and download them like we would download any other prebuilt version of it. Specifically:

1. Patch and build the dependency somewhere outside of the Bazel build, following the project's own build instructions. For instance, this could mean spinning up a development container to build the appropriate version of Python.
2. Capture the outputs in a way that is consumable by Bazel. Most of the cases, the outputs of this build will be a series of headers and `.so` files that you can bundle into a tar archive. Or, in the case of Python, it may just produce the archive itself as a product of the build.
3. Place the outputs somewhere reachable, like a cloud bucket or a public GitHub release (e.g. [`gcc-builds`](https://github.com/f0rmiga/gcc-builds) publishes repacks of the gcc toolchain).
4. Add it to the SONiC Bazel Registry, to make the dependency reachable.

> [!warning]
> As you may have noticed, this method is the worst of them all, because it means we cannot build from source. If we ever have to make a change to our patches, or upgrade the versions of that dependency, we'll have to figure out how to build another artifact, and push it to Artifactory all over again.

### Add it to the SONiC Bazel Registry

To make the dependency reachable by every project, we should add it to the SONiC Bazel Registry.

To do that, we're going to follow the same pattern as in [Method 3](#then-we-import-the-project-into-the-sonic-build), except this time the `<dependency>.BUILD` file does not need to contain targets to _build_ the dependency, just to _use_ it. We don't have an actual example in the project yet, so we'll walk through a made-up scenario. 

Imagine we're building our own Python distribution, and we only care about the interpreter's binary. We have already built the Python distribution we want, and have uploaded it to `https://my.artifactory.xyz/bazel/prebuilt/python-prebuilt.tar.gz`.

First, we would write `src/pyhon/python.BUILD` like this:

```starlark
$ cat src/python/python.BUILD

alias(
	name = "python",
	target = ":bin/python3", # The actual path in python-prebuilt.tar.gz where the python binary lives.
)
```

Then, we'd have `src/python/MODULE.bazel` download the binary and overlay the file like this:

```starlark
$ cat src/python/MODULE.bazel
... 

http_archive(
    name = "python_prebuilt",
    build_file_template = "//:python.BUILD",
    patches = [
	    # No patches! We've already applied those when we built the acrhive
    ],
    urls = ["https://my.artifactory.xyz/bazel/prebuilt/python-prebuilt.tar.gz"],
    sha256 = "<integrity>",
)
```

Then, we add `src/python/BUILD.bazel` to surface the target:

```starlark
$ cat src/python/BUILD.bazel
alias(
	name = "python",
	target = "@python_prebuilt//:python",
)
```

And we add it to the SONiC Bazel Registry:

```
$ ls tools/bazel/registry/modules
...
python
```

