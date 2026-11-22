"""Original CLI completion generation with declared Python runfiles and APT."""
load("@rules_python//python:py_info.bzl", "PyInfo")

def _runfile(ctx, file):
    return file.short_path[3:] if file.short_path.startswith("../") else ctx.workspace_name + "/" + file.short_path

def _generator_impl(ctx):
    for tree in ctx.files.source_trees:
        if not tree.is_directory:
            fail("source_trees requires installation directories: " + tree.path)
    executable = ctx.actions.declare_file(ctx.label.name)
    filesystem = ctx.actions.declare_file(ctx.label.name + ".filesystem.json")
    imports = depset(transitive = [target[PyInfo].imports for target in ctx.attr.python_deps]).to_list()
    arguments = ['"$runfiles/' + _runfile(ctx, ctx.file._runner) + '"', '--generator="$runfiles/' + _runfile(ctx, ctx.file.generator) + '"', '--apt-runtime="$runfiles/' + _runfile(ctx, ctx.file.apt_runtime) + '"', '--filesystem="$runfiles/' + _runfile(ctx, filesystem) + '"', '--bwrap="$runfiles/' + _runfile(ctx, ctx.executable.bwrap) + '"', '--hosts="$runfiles/' + _runfile(ctx, ctx.file.hosts) + '"', '--hostname="$runfiles/' + _runfile(ctx, ctx.file.hostname) + '"', '--ca-bundle="$runfiles/' + _runfile(ctx, ctx.file.ca_bundle) + '"', '--cfggen="$runfiles/' + _runfile(ctx, ctx.file.cfggen) + '"', '--database-config="$runfiles/' + _runfile(ctx, ctx.file.database_config) + '"']
    arguments += ['--source-tree="$runfiles/' + _runfile(ctx, file) + '"' for file in ctx.files.source_trees]
    arguments += ['--python-import="$runfiles/' + path + '"' for path in sorted(imports)]
    ctx.actions.write(executable, '''#!/bin/sh
set -eu
self=$0
case "$self" in /*) ;; *) self="$PWD/$self" ;; esac
runfiles=${RUNFILES_DIR:-$self.runfiles}
RUNFILES_DIR="$runfiles"
PYTHONHOME="$runfiles/%s/usr"
PYTHONDONTWRITEBYTECODE=1
export RUNFILES_DIR PYTHONHOME PYTHONDONTWRITEBYTECODE
exec "$runfiles/%s" %s --output "$@"
''' % (_runfile(ctx, ctx.file.apt_runtime), _runfile(ctx, ctx.executable.python), " ".join(arguments)), is_executable = True)
    runfiles = ctx.runfiles(files = [ctx.file._runner, ctx.file.generator, ctx.file.apt_runtime, ctx.executable.python, ctx.executable.bwrap, ctx.file.cfggen, ctx.file.database_config, ctx.file.ca_bundle, ctx.file.hosts, ctx.file.hostname] + ctx.files.source_trees)
    for target in ctx.attr.python_deps + [ctx.attr.python, ctx.attr.bwrap]:
        runfiles = runfiles.merge(target[DefaultInfo].default_runfiles).merge(target[DefaultInfo].data_runfiles)
    namespace = ctx.runfiles(files = [ctx.file._runner, ctx.file.generator, ctx.file.apt_runtime, ctx.file.cfggen, ctx.file.database_config, ctx.file.ca_bundle, ctx.file.hosts, ctx.file.hostname] + ctx.files.source_trees)
    for target in ctx.attr.python_deps:
        namespace = namespace.merge(target[DefaultInfo].default_runfiles).merge(target[DefaultInfo].data_runfiles)
    names = {_runfile(ctx, file): file.is_directory for file in namespace.files.to_list()}
    for entry in namespace.symlinks.to_list():
        names[ctx.workspace_name + "/" + entry.path] = entry.target_file.is_directory
    for entry in namespace.root_symlinks.to_list():
        names[entry.path] = entry.target_file.is_directory
    names[_runfile(ctx, filesystem)] = False
    ctx.actions.write(filesystem, json.encode(names))
    runfiles = runfiles.merge(ctx.runfiles(files = [filesystem]))
    return [DefaultInfo(executable = executable, runfiles = runfiles)]

utilities_generator = rule(implementation = _generator_impl, executable = True, attrs = {
    "generator": attr.label(allow_single_file = True, mandatory = True),
    "source_trees": attr.label_list(allow_files = True),
    "apt_runtime": attr.label(allow_single_file = True, mandatory = True, cfg = "exec"),
    "python": attr.label(executable = True, mandatory = True, cfg = "exec"),
    "python_deps": attr.label_list(providers = [PyInfo]),
    "bwrap": attr.label(executable = True, mandatory = True, cfg = "exec"),
    "cfggen": attr.label(allow_single_file = True, mandatory = True),
    "hosts": attr.label(allow_single_file = True, mandatory = True),
    "hostname": attr.label(allow_single_file = True, mandatory = True),
    "ca_bundle": attr.label(allow_single_file = True, mandatory = True),
    "database_config": attr.label(allow_single_file = True, mandatory = True),
    "_runner": attr.label(default = Label(":generate.py"), allow_single_file = True),
})

def _impl(ctx):
    completions = ctx.actions.declare_directory(ctx.label.name + ".completions")
    mtree = ctx.actions.declare_file(ctx.label.name + ".mtree")
    ctx.actions.run(
        executable = ctx.executable.generator,
        tools = [ctx.attr.generator[DefaultInfo].files_to_run],
        arguments = [completions.path],
        inputs = ctx.files.validation,
        outputs = [completions],
        env = {"LANG": "C", "LC_ALL": "C", "PATH": ""},
        mnemonic = "SonicUtilityCompletions",
    )
    docs = [file for target in ctx.attr.docs.values() for file in target[DefaultInfo].files.to_list()]
    ctx.actions.run(
        executable = ctx.executable._mtree_tool,
        arguments = ["--completions", completions.path, "--output", mtree.path, "--template-root", ctx.file.source_generator.dirname + "/templates"] + ["--doc=" + name + "=" + target[DefaultInfo].files.to_list()[0].path for name, target in ctx.attr.docs.items()] + [file.path for file in ctx.files.templates],
        inputs = [completions] + ctx.files.templates + docs,
        outputs = [mtree],
        mnemonic = "SonicUtilityDataMtree",
    )
    return [DefaultInfo(files = depset([completions])), OutputGroupInfo(mtree = depset([mtree]))]

utilities_data = rule(implementation = _impl, attrs = {
    "generator": attr.label(executable = True, cfg = "exec", mandatory = True),
    "source_generator": attr.label(allow_single_file = True, mandatory = True),
    "templates": attr.label_list(allow_files = True),
    "validation": attr.label_list(allow_files = True),
    "docs": attr.string_keyed_label_dict(allow_files = True),
    "_mtree_tool": attr.label(default = Label(":mtree_tool"), executable = True, cfg = "exec"),
})
