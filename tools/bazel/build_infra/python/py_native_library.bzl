"""Rule to create a Python library that wraps native C++ extensions."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_python//python:py_library.bzl", "py_library")

_INIT_PY_TEMPLATE = '''"""Auto-generated wrapper for native extension."""
try:
    from . import _{module_name}
except ImportError:
    import atexit, ctypes, importlib.util, os, sys

    def _find_runfiles():
        runfiles = os.environ.get("RUNFILES_DIR", "")
        if not runfiles:
            path = os.path.abspath(__file__)
            if ".runfiles" in path:
                runfiles = path[:path.find(".runfiles") + len(".runfiles")]
        return runfiles

    def _preload_libs():
        runfiles = _find_runfiles()
        if not runfiles:
            return False
        loaded = []
        pending = []
        for paths in {lib_paths}.values():
            for rel_path in paths:
                for prefix in ("", "_main/"):
                    full_path = os.path.join(runfiles, prefix + rel_path)
                    if os.path.exists(full_path):
                        pending.append(full_path)
                        break
                else:
                    continue
                break
        for _ in range(10):
            if not pending:
                break
            retry = []
            for path in pending:
                try:
                    loaded.append(ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL))
                except OSError as e:
                    if "invalid ELF header" not in str(e):
                        retry.append(path)
            pending = retry
        if loaded:
            atexit.register(os._exit, 0)
        return bool(loaded)

    def _load_extension():
        so_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_{module_name}.so")
        spec = importlib.util.spec_from_file_location("{package_name}._{module_name}", so_path)
        if spec is None or spec.loader is None:
            raise ImportError(f"Cannot load native extension from {{so_path}}")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        return module

    _preload_libs()
    _{module_name} = _load_extension()

from .{module_name} import *
'''

_SKIP_LIB_PREFIXES = ("libasan", "libtsan", "libmsan", "libubsan", "liblsan", "libmemusage", "libpcprofile")

def _py_native_library_impl(ctx):
    pkg = ctx.attr.package_name
    module = ctx.attr.module_name
    outputs = []

    # Collect and filter dynamic libraries from cc_deps
    lib_paths = {}
    dynamic_libs = []
    for dep in ctx.attr.cc_deps:
        if CcInfo not in dep:
            continue
        cc_info = dep[CcInfo]
        if not cc_info.linking_context:
            continue
        for linker_input in cc_info.linking_context.linker_inputs.to_list():
            for lib in linker_input.libraries:
                if not lib.dynamic_library:
                    continue
                basename = lib.dynamic_library.basename
                if any([basename.startswith(p) for p in _SKIP_LIB_PREFIXES]):
                    continue
                if "gconv" in lib.dynamic_library.short_path:
                    continue
                dynamic_libs.append(lib.dynamic_library)
                base = basename.split(".so")[0]
                if base not in lib_paths:
                    lib_paths[base] = []
                lib_paths[base].append(lib.dynamic_library.short_path)

    # Generate __init__.py
    init_py = ctx.actions.declare_file(pkg + "/__init__.py")
    ctx.actions.write(
        output = init_py,
        content = _INIT_PY_TEMPLATE.format(
            module_name = module,
            package_name = pkg,
            lib_paths = repr(lib_paths),
        ),
    )
    outputs.append(init_py)

    # Symlink SWIG wrapper .py if provided
    if ctx.file.native_py:
        py_out = ctx.actions.declare_file(pkg + "/" + module + ".py")
        ctx.actions.symlink(output = py_out, target_file = ctx.file.native_py)
        outputs.append(py_out)

    # Symlink native .so into package directory
    so_out = ctx.actions.declare_file(pkg + "/_" + module + ".so")
    ctx.actions.symlink(output = so_out, target_file = ctx.file.native_so)
    outputs.append(so_out)

    # Build runfiles with dynamic libraries
    runfiles = ctx.runfiles(files = dynamic_libs)
    for dep in ctx.attr.cc_deps:
        runfiles = runfiles.merge(dep[DefaultInfo].default_runfiles)

    return [DefaultInfo(files = depset(outputs), runfiles = runfiles)]

_py_native_library_gen = rule(
    implementation = _py_native_library_impl,
    attrs = {
        "package_name": attr.string(mandatory = True),
        "module_name": attr.string(mandatory = True),
        "native_so": attr.label(mandatory = True, allow_single_file = True),
        "native_py": attr.label(allow_single_file = [".py"]),
        "cc_deps": attr.label_list(providers = [CcInfo]),
    },
)

def py_native_library(
        name,
        native_so,
        native_py = None,
        cc_deps = [],
        srcs = [],
        deps = [],
        data = [],
        imports = [],
        visibility = None,
        **kwargs):
    """Create a Python library wrapping a native C++ extension.

    Args:
        name: Target name (also used as Python package name)
        native_so: Label of native .so (cc_binary with linkshared=1)
        native_py: Optional SWIG-generated .py wrapper
        cc_deps: cc_library deps for runtime dynamic libraries
        srcs: Additional Python sources
        deps: Python library dependencies
        data: Additional data files
        imports: Import path modifications ("." always included)
        visibility: Target visibility
        **kwargs: Additional py_library arguments
    """
    gen_name = name + "_gen"
    _py_native_library_gen(
        name = gen_name,
        package_name = name,
        module_name = name,
        native_so = native_so,
        native_py = native_py,
        cc_deps = cc_deps,
    )

    py_library(
        name = name,
        srcs = [":" + gen_name] + list(srcs),
        data = [":" + gen_name] + list(data),
        imports = ["."] + list(imports),
        deps = deps,
        visibility = visibility,
        **kwargs
    )
