"""
Custom sdist_build rule that works with already-extracted source and supports a CcInfo dependency.
Similar to sdist_build_with_deps but takes extracted source instead of a tarball.
"""

load("@aspect_rules_py//py/private/py_venv:types.bzl", "VirtualenvInfo")
load("@aspect_rules_py//py/private/toolchain:types.bzl", "PY_TOOLCHAIN", "UNPACK_TOOLCHAIN")
load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain", "use_cc_toolchain")
load("@rules_python//python:defs.bzl", "PyInfo")

def _sdist_build_from_src(ctx):
    py_toolchain = ctx.toolchains[PY_TOOLCHAIN].py3_runtime
    cc_toolchain = find_cc_toolchain(ctx)

    # Get all source files
    src_files = ctx.attr.src[DefaultInfo].files.to_list()

    wheel_dir = ctx.actions.declare_directory("build")
    venv = ctx.attr.venv

    # Collect include and library paths from deps
    include_paths = []
    library_path = ""
    cc_files = []

    # Get headers from either headers attr or dep
    headers_target = ctx.attr.headers if ctx.attr.headers else ctx.attr.dep
    dep = ctx.attr.dep
    if CcInfo in headers_target:
        cc_info = headers_target[CcInfo]

        # Get include directories from compilation context
        for inc in cc_info.compilation_context.includes.to_list():
            if inc and inc not in include_paths:
                include_paths.append(inc)

        # Get headers as input files
        for hdr in cc_info.compilation_context.headers.to_list():
            cc_files.append(hdr)

            # Extract include path from header location
            if "/usr/include/" in hdr.path and not include_paths:
                idx = hdr.path.find("/usr/include/")
                include_paths.append(hdr.path[:idx + len("/usr/include")])
                # For Bazel-built libraries, look for libyang/ in path

            elif "/libyang/" in hdr.path and hdr.path.endswith(".h"):
                # Get parent of libyang dir as include path
                idx = hdr.path.find("/libyang/")
                inc_path = hdr.dirname[:idx + 1] if idx >= 0 else hdr.dirname
                if inc_path not in include_paths:
                    include_paths.append(inc_path)

        # Get library dirs from linking context
        if cc_info.linking_context:
            for linker_input in cc_info.linking_context.linker_inputs.to_list():
                for lib in linker_input.libraries:
                    if lib.dynamic_library:
                        lib_dir = lib.dynamic_library.dirname
                        if "/usr/lib" in lib_dir and not library_path:
                            idx = lib_dir.find("/usr/lib")
                            library_path = lib_dir[:idx] + "/usr/lib"
                        elif not library_path:
                            library_path = lib_dir

                    # Also check static library
                    if lib.static_library and not library_path:
                        library_path = lib.static_library.dirname

    # Also collect DefaultInfo files (includes shared libraries)
    for f in dep[DefaultInfo].files.to_list():
        cc_files.append(f)
        if "/usr/lib/" in f.path and f.path.endswith(".so") and not library_path:
            idx = f.path.find("/usr/lib")
            library_path = f.dirname
        elif f.path.endswith(".a") and not library_path:
            library_path = f.dirname

    # Join include paths with colons for multiple paths
    # Prefix each path with $EXECROOT since we'll be in a different directory
    include_path = ":".join(["$EXECROOT/" + p if not p.startswith("/") else p for p in include_paths]) if include_paths else ""

    # Get CC toolchain files for linker
    cc_toolchain_files = cc_toolchain.all_files.to_list()

    # Find the root directory of the source files
    # We need to find a common prefix for all source files
    src_dir = ""
    for f in src_files:
        if f.path.endswith("setup.py") or f.path.endswith("pyproject.toml"):
            src_dir = f.dirname
            break

    # Prefix library path with $EXECROOT as well
    if library_path and not library_path.startswith("/"):
        library_path_full = "$EXECROOT/" + library_path
    else:
        library_path_full = library_path

    # Get shared library path if available
    shared_lib = None
    shared_lib_dir = ""
    shared_lib_file = ""
    for f in dep[DefaultInfo].files.to_list():
        if f.path.endswith(".so"):
            shared_lib = f
            shared_lib_dir = f.dirname
            shared_lib_file = f.path
            break

    # Use run_shell to build the wheel from extracted source
    # Save PWD before cd since paths are relative to execroot
    command = """
set -e
EXECROOT="$PWD"
export LIBYANG_HEADERS="{headers}"
export LIBYANG_LIBRARIES="{libraries}"
export PATH="$PATH:/usr/bin"

# Set up library path to find our libyang3 shared library first
if [ -n "{shared_lib_dir}" ]; then
    export LIBRARY_PATH="$EXECROOT/{shared_lib_dir}:$LIBRARY_PATH"
    export LD_LIBRARY_PATH="$EXECROOT/{shared_lib_dir}:$LD_LIBRARY_PATH"
    # Create symlink so linker finds libyang.so.3
    ln -sf "$EXECROOT/{shared_lib_file}" "$EXECROOT/{shared_lib_dir}/libyang.so.3"
    ln -sf "$EXECROOT/{shared_lib_dir}/libyang.so.3" "$EXECROOT/{shared_lib_dir}/libyang.so"
    export LIBYANG_LIBRARIES="$EXECROOT/{shared_lib_dir}"
fi

# Create output directory
mkdir -p "$EXECROOT/{outdir}"

# Build the wheel (--no-isolation since venv already has deps)
cd "$EXECROOT/{srcdir}"
"$EXECROOT/{python}" -m build --wheel --no-isolation --outdir "$EXECROOT/{outdir}"
""".format(
        headers = include_path,
        libraries = library_path_full,
        shared_lib_dir = shared_lib_dir,
        shared_lib_file = shared_lib_file,
        python = venv[VirtualenvInfo].home.path + "/bin/python3",
        srcdir = src_dir,
        outdir = wheel_dir.path,
    )

    # Collect all input files including headers
    headers_files = []
    if ctx.attr.headers:
        headers_files = ctx.attr.headers[DefaultInfo].files.to_list()

    ctx.actions.run_shell(
        command = command,
        inputs = src_files + [
            venv[VirtualenvInfo].home,
        ] + py_toolchain.files.to_list() + ctx.attr.venv[DefaultInfo].files.to_list() + cc_files + cc_toolchain_files + headers_files,
        outputs = [wheel_dir],
    )

    # Install the wheel to provide PyInfo
    install_dir = ctx.actions.declare_directory("install")

    # Use unpack tool to install the wheel
    unpack = ctx.attr._unpack[platform_common.ToolchainInfo].bin.bin

    install_command = """
WHEEL=$(ls "{wheel_dir}"/*.whl 2>/dev/null | head -1)
if [ -z "$WHEEL" ]; then
    echo "No wheel found in {wheel_dir}" >&2
    exit 1
fi

"{unpack}" --into "{install_dir}" --wheel "$WHEEL" --python-version-major {py_major} --python-version-minor {py_minor}

# Create yang.py compatibility shim (sonic_yang expects 'yang' module, libyang 3.x provides 'libyang')
cat > "{install_dir}/lib/python{py_major}.{py_minor}/site-packages/yang.py" << 'EOF'
# Compatibility shim for libyang 3.x
# sonic_yang.py expects to import 'yang' but libyang 3.x provides 'libyang'
from libyang import *
EOF

# Materialize the libyang.so that we have built from source, and add it to the rpath.
# TODO: There must be a better way to do this.
EXECROOT="$PWD"
root="{install_dir}/lib/python{py_major}.{py_minor}/site-packages/"
wheel_so=$(find ${{root}} -name "*.so")

cp "$EXECROOT/{shared_library_file}" "${{root}}/libyang.so"
{patchelf} --set-rpath '$ORIGIN' "${{wheel_so}}"
""".format(
        wheel_dir = wheel_dir.path,
        unpack = unpack.path,
        install_dir = install_dir.path,
        py_major = py_toolchain.interpreter_version_info.major,
        py_minor = py_toolchain.interpreter_version_info.minor,
        shared_library_file = shared_lib_file,
        shared_lib_dir = shared_lib_dir,
        patchelf = ctx.executable._patchelf.path,
    )

    ctx.actions.run_shell(
        command = install_command,
        inputs = [
            wheel_dir,
            unpack,
            shared_lib,
            ctx.executable._patchelf,
        ],
        outputs = [install_dir],
    )

    repo_name = ctx.label.repo_name

    return [
        DefaultInfo(
            files = depset([install_dir]),
            runfiles = ctx.runfiles(files = [install_dir]),
        ),
        PyInfo(
            transitive_sources = depset([install_dir]),
            imports = depset([
                ctx.label.package + "{}/install/lib/python{}.{}/site-packages".format(
                    repo_name,
                    py_toolchain.interpreter_version_info.major,
                    py_toolchain.interpreter_version_info.minor,
                ),
            ]),
            has_py2_only_sources = False,
            has_py3_only_sources = True,
            uses_shared_libraries = True,  # libyang has native extensions
        ),
    ]

sdist_build_from_src = rule(
    implementation = _sdist_build_from_src,
    doc = """Build a Python wheel from extracted source with CcInfo dependency support.

Similar to sdist_build_with_deps but takes already-extracted source files
instead of a tarball. Useful when source is fetched via http_archive with patches.
""",
    attrs = {
        "src": attr.label(doc = "The extracted source files (filegroup)"),
        "venv": attr.label(doc = "The Python venv with build dependencies"),
        "dep": attr.label(doc = "Library dependency (shared library for linking)"),
        "headers": attr.label(doc = "Optional separate target providing CcInfo with headers", default = None),
        "_cc_toolchain": attr.label(
            default = Label("@rules_cc//cc:current_cc_toolchain"),
        ),
        "_unpack": attr.label(
            default = "@aspect_rules_py//py/private/toolchain:resolved_unpack_toolchain",
            cfg = "exec",
        ),
        "_patchelf": attr.label(
            default = "@patchelf//:patchelf",
            cfg = "exec",
            executable = True,
        ),
    },
    toolchains = [
        PY_TOOLCHAIN,
        UNPACK_TOOLCHAIN,
    ] + use_cc_toolchain(),
    provides = [
        DefaultInfo,
        PyInfo,
    ],
)
