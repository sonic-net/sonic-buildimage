"""Explicit lock maintenance with the plan in its target configuration."""

def _runfile(ctx, file):
    return file.short_path[3:] if file.short_path.startswith("../") else ctx.workspace_name + "/" + file.short_path

def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name)
    tool = ctx.attr._tool[DefaultInfo]
    ctx.actions.write(output, """#!/bin/sh
set -eu
self=$0
case "$self" in /*) ;; *) self="$PWD/$self" ;; esac
runfiles=$self.runfiles
if [ ! -d "$runfiles" ]; then runfiles=${RUNFILES_DIR:?Missing runfiles}; fi
export RUNFILES_DIR="$runfiles"
owner="${BUILD_WORKSPACE_DIRECTORY:?Run this maintenance entry with bazel run}"/%s
exec "$runfiles/%s" update --plan "$runfiles/%s" --requirements "$owner/requirements.in" --lock "$owner/requirements_lock.txt" --inputs "$owner/runtime.inputs.json" "$@"
""" % ("'" + ctx.label.package.replace("'", "'\"'\"'") + "'", _runfile(ctx, tool.files_to_run.executable), _runfile(ctx, ctx.file.plan)), is_executable = True)
    runfiles = ctx.runfiles(files = [ctx.file.plan, tool.files_to_run.executable])
    return [DefaultInfo(executable = output, runfiles = runfiles.merge(tool.default_runfiles).merge(tool.data_runfiles))]

runtime_lock_update = rule(implementation = _impl, executable = True, attrs = {
    "plan": attr.label(mandatory = True, allow_single_file = True),
    "_tool": attr.label(default = Label("@sonic_build_infra//python:runtime_lock"), executable = True, cfg = "exec"),
})
