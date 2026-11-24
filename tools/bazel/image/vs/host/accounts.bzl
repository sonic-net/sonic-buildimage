"""Native account database generation from declared package inputs."""
def _accounts_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.json")
    spec = ctx.actions.declare_file(ctx.label.name + ".spec.json")
    data = {}
    controls = {}
    inputs = [spec, ctx.file.bashrc, ctx.file.skel_profile, ctx.file.skel_bash_logout]
    for targets, result in [(ctx.attr.data, data), (ctx.attr.controls, controls)]:
        for target, name in targets.items():
            files = target[DefaultInfo].files.to_list()
            if len(files) != 1:
                fail("account input must have one TAR: " + str(target.label))
            result[name] = files[0].path
            inputs.append(files[0])
    ctx.actions.write(spec, json.encode({
        "data": data,
        "controls": controls,
        "installed": ctx.attr.installed,
        "configuration": {
            "USERNAME": ctx.attr.username,
            "PASSWORD": ctx.attr.password,
            "DEFAULT_USERINFO": ctx.attr.user_info,
            "CHANGE_DEFAULT_PASSWORD": "y" if ctx.attr.change_default_password else "n",
        },
        "bashrc": ctx.file.bashrc.path,
        "skel_profile": ctx.file.skel_profile.path,
        "skel_bash_logout": ctx.file.skel_bash_logout.path,
        "lastchg_day": ctx.attr.lastchg_day,
        "mkpasswd": ctx.executable.mkpasswd.path,
    }))
    ctx.actions.run(
        executable = ctx.executable._tool,
        arguments = [spec.path, output.path, audit.path],
        inputs = inputs,
        tools = [ctx.attr.mkpasswd[DefaultInfo].files_to_run],
        outputs = [output, audit],
        env = {"LC_ALL": "C", "TZ": "UTC"},
        mnemonic = "VsHostAccounts",
    )
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]))]

host_accounts = rule(
    implementation = _accounts_impl,
    attrs = {
        "data": attr.label_keyed_string_dict(allow_files = True),
        "controls": attr.label_keyed_string_dict(allow_files = True),
        "installed": attr.string_list(mandatory = True),
        "username": attr.string(mandatory = True),
        "password": attr.string(mandatory = True),
        "user_info": attr.string(default = "Default admin user,,,"),
        "change_default_password": attr.bool(),
        "lastchg_day": attr.int(mandatory = True),
        "bashrc": attr.label(default = Label("//files/image_config:bash/bash.bashrc"), allow_single_file = True),
        "skel_profile": attr.label(default = Label("@sonic_bash//:skel_profile"), allow_single_file = True),
        "skel_bash_logout": attr.label(default = Label("@sonic_bash//:skel_bash_logout"), allow_single_file = True),
        "mkpasswd": attr.label(mandatory = True, executable = True, cfg = "exec"),
        "_tool": attr.label(default = Label(":accounts_tool"), executable = True, cfg = "exec"),
    },
)
