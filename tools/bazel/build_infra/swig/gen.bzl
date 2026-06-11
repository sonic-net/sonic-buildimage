load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cpp_toolchain", "use_cc_toolchain")

def _swig_gen_cc_impl(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    # Collect all include paths from compilation context of deps and hdrs
    compilation_contexts = []
    for dep in ctx.attr.deps:
        if CcInfo in dep:
            compilation_contexts.append(dep[CcInfo].compilation_context)
    for hdr in ctx.files.hdrs:
        compilation_contexts.append(cc_common.create_compilation_context(
            headers = depset([hdr]),
            includes = depset([hdr.dirname]),
        ))

    # Merge all compilation contexts
    compilation_context = cc_common.merge_compilation_contexts(
        compilation_contexts = compilation_contexts,
    )

    # Output files
    cpp_out = ctx.outputs.cpp_out
    python_out = ctx.outputs.python_out

    # Base name for generated files (from interface file)
    interface_file = ctx.file.interface

    # Get SWIG_LIB path from the swig_lib attribute
    # swig_lib can be either a directory (tree artifact) or individual files
    swig_lib_files = ctx.files.swig_lib
    swig_lib_path = None

    if len(swig_lib_files) == 1 and swig_lib_files[0].is_directory:
        # Tree artifact - the directory itself is the SWIG_LIB path
        swig_lib_path = swig_lib_files[0].path
    else:
        # Individual files - find swig.swg and use its directory
        for f in swig_lib_files:
            if f.path.endswith("/swig.swg") or f.basename == "swig.swg":
                swig_lib_path = f.dirname
                break

    if not swig_lib_path:
        fail("Could not find SWIG library path. Ensure swig_lib points to extracted SWIG library files containing swig.swg.")

    # Arguments for SWIG
    args = ctx.actions.args()
    args.add("-c++")
    args.add("-python")
    args.add("-Wall")
    args.add("-keyword")
    args.add("-DSWIGWORDSIZE64")  # Important for 64-bit

    # Get include flags (-Ipath)
    include_args = []
    for include in compilation_context.includes.to_list():
        args.add("-I" + include)
    for external_include in compilation_context.external_includes.to_list():
        args.add("-I" + external_include)

    args.add("-o", cpp_out.path)
    args.add("-outdir", python_out.dirname)
    args.add(interface_file.path)

    # Inputs: interface + all headers + transitive deps + swig library files
    inputs = depset(
        direct = [interface_file] + ctx.files.hdrs + swig_lib_files,
        transitive = [compilation_context.headers],
    )

    # Tools: SWIG + C++ toolchain (for potential headers)
    tools = [ctx.executable._swig]

    ctx.actions.run(
        executable = ctx.executable._swig,
        arguments = [args],
        inputs = inputs,
        outputs = [cpp_out, python_out],
        tools = tools,
        mnemonic = "SwigGenCC",
        progress_message = "Generating SWIG bindings for %{label}",
        env = {
            "SWIG_LIB": swig_lib_path,
        },
    )

    return [
        DefaultInfo(files = depset([python_out, cpp_out])),
        OutputGroupInfo(
            cpp = depset([cpp_out]),
            python = depset([python_out]),
        ),
    ]

def _swig_gen_go_impl(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)
    cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    # Collect all include paths from compilation context of deps and hdrs
    compilation_contexts = []
    for dep in ctx.attr.deps:
        if CcInfo in dep:
            compilation_contexts.append(dep[CcInfo].compilation_context)
    for hdr in ctx.files.hdrs:
        compilation_contexts.append(cc_common.create_compilation_context(
            headers = depset([hdr]),
            includes = depset([hdr.dirname]),
        ))

    compilation_context = cc_common.merge_compilation_contexts(
        compilation_contexts = compilation_contexts,
    )

    go_out = ctx.outputs.go_out
    cxx_out = ctx.outputs.cxx_out
    hdr_out = ctx.outputs.hdr_out
    interface_file = ctx.file.interface

    swig_lib_files = ctx.files.swig_lib
    swig_lib_path = None

    if len(swig_lib_files) == 1 and swig_lib_files[0].is_directory:
        swig_lib_path = swig_lib_files[0].path
    else:
        for f in swig_lib_files:
            if f.path.endswith("/swig.swg") or f.basename == "swig.swg":
                swig_lib_path = f.dirname
                break

    if not swig_lib_path:
        fail("Could not find SWIG library path. Ensure swig_lib points to extracted SWIG library files containing swig.swg.")

    args = ctx.actions.args()
    args.add("-go")
    args.add("-cgo")
    args.add("-c++")
    args.add("-intgosize")
    args.add("64")
    args.add("-DSWIGWORDSIZE64")

    for include in compilation_context.includes.to_list():
        args.add("-I" + include)
    for external_include in compilation_context.external_includes.to_list():
        args.add("-I" + external_include)

    args.add("-o", cxx_out.path)
    args.add("-oh", hdr_out.path)
    args.add("-outdir", go_out.dirname)
    args.add(interface_file.path)

    inputs = depset(
        direct = [interface_file] + ctx.files.hdrs + swig_lib_files,
        transitive = [compilation_context.headers],
    )

    ctx.actions.run(
        executable = ctx.executable._swig,
        arguments = [args],
        inputs = inputs,
        outputs = [go_out, cxx_out, hdr_out],
        mnemonic = "SwigGenGo",
        progress_message = "Generating Go SWIG bindings for %{label}",
        env = {
            "SWIG_LIB": swig_lib_path,
        },
    )

    return [
        DefaultInfo(files = depset([go_out, cxx_out, hdr_out])),
        OutputGroupInfo(
            go = depset([go_out]),
            cxx = depset([cxx_out]),
            hdr = depset([hdr_out]),
        ),
    ]

swig_gen_go = rule(
    implementation = _swig_gen_go_impl,
    attrs = {
        "interface": attr.label(
            mandatory = True,
            allow_single_file = [".i"],
            doc = "The .i SWIG interface file",
        ),
        "hdrs": attr.label_list(
            allow_files = True,
            doc = "Public headers needed for SWIG processing",
        ),
        "deps": attr.label_list(
            doc = "cc_library dependencies that provide headers/includes",
            providers = [CcInfo],
        ),
        "go_out": attr.output(
            mandatory = True,
            doc = "Output Go module file (e.g. swsscommon.go)",
        ),
        "cxx_out": attr.output(
            mandatory = True,
            doc = "Output C++ wrapper file (e.g. swsscommon_wrap.cxx)",
        ),
        "hdr_out": attr.output(
            mandatory = True,
            doc = "Output C++ wrapper header (e.g. swsscommon_wrap.h)",
        ),
        "_swig": attr.label(
            default = "@swig//:swig",
            executable = True,
            cfg = "exec",
        ),
        "swig_lib": attr.label(
            mandatory = True,
            doc = "SWIG library directory containing swig.swg and language-specific files",
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cc_toolchain(),
)

swig_gen = rule(
    implementation = _swig_gen_cc_impl,
    attrs = {
        "interface": attr.label(
            mandatory = True,
            allow_single_file = [".i"],
            doc = "The .i SWIG interface file",
        ),
        "hdrs": attr.label_list(
            allow_files = True,
            doc = "Public headers needed for SWIG processing",
        ),
        "deps": attr.label_list(
            doc = "cc_library dependencies that provide headers/includes",
            providers = [CcInfo],
        ),
        "cpp_out": attr.output(
            mandatory = True,
            doc = "Output C++ wrapper file (e.g. swsscommon_wrap.cpp)",
        ),
        "python_out": attr.output(
            mandatory = True,
            doc = "Output Python module (e.g. swsscommon.py)",
        ),
        "_swig": attr.label(
            default = "@swig//:swig",
            executable = True,
            cfg = "exec",
        ),
        "swig_lib": attr.label(
            mandatory = True,
            doc = "SWIG library directory containing swig.swg and language-specific files",
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cc_toolchain(),
)
