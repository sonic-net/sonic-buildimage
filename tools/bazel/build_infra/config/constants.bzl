# All possible values for SONiC ASIC vendors,
# from @sonic-buildimage//README.md
ASIC_MANUFACTURERS = [
    "barefoot",
    "broadcom",
    "marvell-prestera",
    "marvell-teralynx",
    "mellanox",
    "centec",
    "nephos",
    "nvidia-bluefield",
    "vs",
    # A special value we use to default to an invalid vendor.
    # sonic-buildimage doesn't default PLATFORM, so we shouldn't either.
    "_incompatible",
]

# All possible values for SONiC device manufacturers,
# from @sonic-buildimage//device/*
DEVICES = [
    "accton",
    "alphanetworks",
    "arista",
    "barefoot",
    "broadcom",
    "celestica",
    "centec",
    "cig",
    "common",
    "dell",
    "delta",
    "facebook",
    "fs",
    "ingrasys",
    "inventec",
    "juniper",
    "marvell",
    "mellanox",
    "micas",
    "mitac",
    "netberg",
    "nexthop",
    "nokia",
    "nvidia-bluefield",
    "pegatron",
    "pensando",
    "quanta",
    "ragile",
    "ruijie",
    "supermicro",
    "tencent",
    "ufispace",
    "virtual",
    "wistron",
    "wnc",
    # A special value we use to default to an invalid vendor.
    # sonic-buildimage doesn't default PLATFORM, so we shouldn't either.
    "_incompatible",
]

BOOLEAN_TOGGLES = [
    "asan",
    "tsan",
    "usan",
    "gcov",
]
