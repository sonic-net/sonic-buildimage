"""protoc, but fetched from Debian archives, like Make does.

protoc is dynamically linked against libprotoc.so.32 and libprotobuf.so.32,
so running it needs those two `.deb`s unpacked alongside the binary and put on the
loader path.

This is necessary to dynamically link binaries to the protobuf runtime shipped in the Make base layers,
which is different from any protobuf runtime offered in any BCR module.
"""

PROTOC = str(Label("//:protoc"))

PROTOC_RUNTIME_LIBS = [
    str(Label(label))
    for label in [
        "//:libprotoc_soname",
        "//:libprotoc_library",
        "//:libprotobuf_soname",
        "//:libprotobuf_library",
    ]
]

PROTOC_TOOLS = [PROTOC] + PROTOC_RUNTIME_LIBS

_DESCRIPTOR_PROTO = str(Label("//:descriptor_proto"))

# The well-known types, plus one file in the tree used to locate their include root.
# Add both to the srcs of any genrule compiling a proto that imports them.
WELL_KNOWN_PROTOS = [
    str(Label("//:well_known_protos")),
    _DESCRIPTOR_PROTO,
]

# usr/include, reached by climbing out of google/protobuf/descriptor.proto.
WELL_KNOWN_PROTO_PATH = "$$(dirname $$(dirname $$(dirname $(execpath {}))))".format(_DESCRIPTOR_PROTO)

def protoc_cmd(args):
    """Build a command to run protoc in a genrule.

    Note that `PROTOC_TOOLS` needs to be in the genrule's `tools`.

    Args:
        args: the protoc command line, as a single string.

    Returns:
        A `cmd` string. Add `PROTOC_TOOLS` to the genrule's `tools`.
    """
    lib_dirs = ":".join([
        "$$(dirname $(execpath {label}))".format(label = label)
        for label in PROTOC_RUNTIME_LIBS
    ])
    return "LD_LIBRARY_PATH={lib_dirs} $(execpath {protoc}) {args}".format(
        args = args,
        lib_dirs = lib_dirs,
        protoc = PROTOC,
    )

def protoc_genrule(name, outs, args, srcs = [], setup = "", **kwargs):
    """A genrule that runs Debian's protoc, with its tools and inputs wired up.

    The well-known protos are always available.
    Please use the `WELL_KNOWN_PROTO_PATH` constant to refer to them in args.

    Args:
        name: the genrule to define.
        outs: the files protoc generates.
        args: the protoc command line, as a single string.
        srcs: the `.proto` inputs, on top of the well-known protos.
        setup: shell to run before protoc, e.g. for setting up env vars.
        **kwargs: passed through to the genrule.
    """
    native.genrule(
        name = name,
        srcs = srcs + WELL_KNOWN_PROTOS,
        outs = outs,
        cmd = setup + protoc_cmd(args),
        tools = PROTOC_TOOLS,
        **kwargs
    )
