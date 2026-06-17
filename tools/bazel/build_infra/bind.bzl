def _alias_impl(rctx):
    rctx.file("BUILD.bazel", "".join([
        """
alias(
    name = "{}",
    actual = "{}",
    visibility = ["//visibility:public"]
)
""".format(name, actual)
        for (name, actual) in rctx.attr.aliases.items()
    ]))

_alias = repository_rule(
    implementation = _alias_impl,
    attrs = {
        "aliases": attr.string_keyed_label_dict(mandatory = True),
    },
)

def _bind_impl(mctx):
    names = []
    for module in mctx.modules:
        for tag in module.tags.alias:
            names.append(tag.name)
            _alias(
                name = tag.name,
                aliases = tag.aliases,
            )

alias = tag_class(
    attrs = {
        "name": attr.string(mandatory = True),
        "aliases": attr.string_keyed_label_dict(mandatory = True),
    },
)

bind = module_extension(
    implementation = _bind_impl,
    tag_classes = {
        "alias": alias,
    },
)
