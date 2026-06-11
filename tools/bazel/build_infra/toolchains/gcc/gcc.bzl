GCC = """#!/bin/bash

args=("$@")

EXECROOT="${EXECROOT:-"$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/../../..")"}"

for i in "${!args[@]}"; do
    val="${args[i]}"

    # Make --sysroot flag absolute for GCC.
    if [[ "${val}" == "--sysroot="* ]]; then
        if [[ "${val}" == "--sysroot=/"* ]]; then
            # The path already seems to be absolute.
            continue
        fi
       # args["${i}"]="--sysroot=$(pwd)/${val#--sysroot=}"
    fi
done

exec "${EXECROOT}/${0%%-wrapped}" "${args[@]}"
"""

def _download_gcc(rctx):
    rctx.download_and_extract(
        url = rctx.attr.urls,
        integrity = rctx.attr.integrity,
    )
    rctx.delete("sysroot")
    rctx.file("BUILD.bazel", rctx.read(rctx.attr.build_file))
    rctx.file("bin/x86_64-linux-gcc-wrapped", GCC, executable = True)
    rctx.file("bin/x86_64-linux-g++-wrapped", GCC, executable = True)

fetch_gcc = repository_rule(
    implementation = _download_gcc,
    attrs = {
        "urls": attr.string_list(),
        "integrity": attr.string(),
        "build_file": attr.label(default = "//toolchains/gcc:gcc.BUILD"),
    },
)
