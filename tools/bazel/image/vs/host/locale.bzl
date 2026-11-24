"""Current build_debian locale-gen/update-locale installation effects."""
def _impl(ctx):
    out=ctx.actions.declare_file(ctx.label.name+".tar")
    audit=ctx.actions.declare_file(ctx.label.name+".audit.json")
    spec=ctx.actions.declare_file(ctx.label.name+".spec.json")
    ctx.actions.write(spec,json.encode({"data":ctx.file.data.path,"control":ctx.file.control.path,"localedef":ctx.executable.localedef.path,"runtime":ctx.file.runtime.path}))
    ctx.actions.run(executable=ctx.executable._tool,arguments=[spec.path,out.path,audit.path],inputs=[spec,ctx.file.data,ctx.file.control,ctx.file.runtime],tools=[ctx.attr.localedef[DefaultInfo].files_to_run],outputs=[out,audit],mnemonic="VsHostLocale")
    return [DefaultInfo(files=depset([out])),OutputGroupInfo(audit=depset([audit]))]
host_locale=rule(implementation=_impl,attrs={"data":attr.label(allow_single_file=True,mandatory=True),"control":attr.label(allow_single_file=True,mandatory=True),"runtime":attr.label(allow_single_file=True,cfg="exec",mandatory=True),"localedef":attr.label(executable=True,cfg="exec",mandatory=True),"_tool":attr.label(default=Label(":locale_tool"),executable=True,cfg="exec")})
