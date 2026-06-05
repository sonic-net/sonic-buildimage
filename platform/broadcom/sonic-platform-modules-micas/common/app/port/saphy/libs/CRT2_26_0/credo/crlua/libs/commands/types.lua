local slash = require "slash"

local parse = require "commands.parse"

slash.register_type("fec", parse.fec_desc, parse.fec)
slash.register_type("fec_error", parse.fec_error_desc, parse.fec_error)
slash.register_type("log_level", parse.log_level_desc, parse.log_level)
slash.register_type("log_io", parse.log_io_desc, parse.log_io)
slash.register_type("init", parse.init_type_desc, parse.init_type)
slash.register_type("integer", [[
Integer

Any 32 bit integer. Can use hex, or decimal.

Also add the ability to use lane id which is converted into lane number

Examples:
- 0x8000
- -0x8000
- 8000
- 1000
- A0 (value is dependent on device type)
- B0 (value is dependent on device type)
]], parse.int)

slash.register_type("intlist", [[
Integer List

A list of integers.

Made of a comma separated integer list + integer ranges.

An integer range is defined as `<start>:<stop>[:<step>]` or `<start>-<stop>`.
This creates a range: {start, start + step, start + 2 * step, ... stop}. Step is default 1.

Examples
- 0,1,2,3                # {0,1,2,3}
- 0:6                    # {0,1,2,3,4,5,6}
- 0:6:2                  # {0,2,4,6}
- 0,1,2,4:10:3           # {0,1,2,4,7,10}
- 5:0                    # {5,4,3,2,1,0}
- 0x7000:0x8000:0x400    # {0x7000,0x7400,0x7800,0x7C00,0x8000}
- 5-0                    # {5,4,3,2,1,0}
- 0-3                    # {0,1,2,3}
Invalid:
- 5:0:2             # wrong step direction

]], parse.intlist)

slash.register_type("indexlist", [[
Index List

A list of indexes.

The same as an integer list (intlist). The only addition of the value `all`.
`all` is conditionally converted to a list based upon context.

]], parse.indexlist)
slash.register_type("intspan", [[
Integer Span

A contiguous span of integers.

Defined as `<start>[:<stop>]`.
This creates a span: {start, start + 1, ... stop}

Examples:
- 0:2        # {0,1,2}
- 2:0        # {0,1,2}
- -2:3       # {-2,-1,0,1,2,3}
- 0x0:0x5    # {0,1,2,3,4,5}
- 0x0-0x5    # {0,1,2,3,4,5}
- -0x1-0x3   # {-1,0,1,2,3}

]], parse.intspan)
slash.register_type("lane", [[
Lane

A lane identifier.

Integer defined from [0, max_lane - 1]

]], parse.lane)
slash.register_type("lanelist", [[
Lane List

An integer list bounded between [0, max_lane - 1].

Special:

`all`: returns all lanes (that aren't disabled)
`a`: return all a-side lanes
`b`: return all b-side lanes

]], parse.lanelist)
slash.register_type("lanemode", parse.lanemode_desc, parse.lanemode)
slash.register_type("lanespan", [[
Lane Span

An integer span bounded between [0, max_lane - 1].

]], parse.lanespan)
slash.register_type("lbmode", parse.lbmode_desc, parse.lbmode)
slash.register_type("param_index", parse.param_index_desc, parse.param_index)
slash.register_type("port", [[
Port

A port identifer.

An integer bounded between [0, max_port - 1].

]], parse.port)
slash.register_type("portflags", [[
Port Flags

Port configuration flags.

The flags are combined using '|'.

Available flags: (shorthand is in parenthesis)
- line_optical         (lopt)
- line_anlt            (lanlt)
- line_an              (lan)
- line_lt              (llt)
- sys_optical          (sopt)
- sys_anlt             (sanlt)
- sys_lt               (slt)
- autoneg_override     (anegovr)
- autoneg_disable      (anegdis)
- enable_double_crc    (dblcrc)

Examples:
- line_optical
- line_optical|sys_optical
- lopt|lanlt|sopt

]], parse.portflags)
slash.register_type("portlayer", parse.portlayer_desc, parse.portlayer)
slash.register_type("portside", parse.portside_desc, parse.portside)
slash.register_type("portlist", [[
Port List

An integer list bounded between [0, max_port - 1]
Can also use `all` for the full bounds.

]], parse.portlist)
slash.register_type("portmode", parse.portmode_desc, parse.portmode)
slash.register_type("prbs_direction", [[
PRBS Direction

Direction of prbs configuration
- rx: prbs rx checker
- tx: prbs tx generator
- both: both directions

]], parse.prbs_direction)
slash.register_type("prbs_pattern", parse.prbs_patt_desc, parse.prbs_pattern)
slash.register_type("regbits", [[
Register Bits

Integer span bounded between [0, 15]

]], parse.regbits)
slash.register_type("reg32bits", [[
Register Bits

Integer span bounded between [0, 31]

]], parse.reg32bits)
slash.register_type("speed", [[
Speed

A speed value.

Must use a speed signifier (G,g,M,m).

Examples:
- 20m       # 20m
- 1g        # 1000m
- 20G       # 20000m
- 12.5g     # 12500m
- 500M      # 500m

]], parse.speed)

slash.register_type("speedf", [[
Speed

A speed value.

Must use a speed signifier (G,g,M,m).

Examples:
- 20m       # 20m
- 1g        # 1000m
- 20G       # 20000m
- 12.5g     # 12500m
- 500M      # 500m

]], parse.speedf)

slash.register_type("decibel", [[
Decibel

A decibel value.

Can use a decibel signifier (db,dB).

Examples:
- 20dB   # 20dB
- 12db   # 12dB
- 8      # 8dB

]], parse.decibel)

slash.register_type("time", [[
Time

A time duration value.

Must use a time signifier (s,sec,us,ms).

Examples:
- 2.5s      # 2.5 sec
- 4.2sec    # 4.2 sec
- 500ms     # 0.5 sec
- 500us     # 0.0005 sec

]], parse.time)

slash.register_type("map", [[
Map

[String]=[String] where the string cannot contain ' ', '=', ','

Examples:
- test=cool,other=24      # {test="cool", other=24}
- 0x22=2,-0x1=-12  # {0x22=2,-0x1=-12}

]], parse.map)

slash.register_type("intmap", [[
Integer Map

List of integer to another integer.

Examples:
- 0=2,3=-12      # {0=2, 3=-12}
- 0x22=2,-0x1=-12  # {0x22=2,-0x1=-12}

]], parse.intmap)

slash.register_type("intnum_map", [[
Integer Map

List of integer to number.

Examples:
- 0=2,3=-12.3      # {0=2, 3=-12.3}
- 0x22=2,-0x1=2e5  # {0x22=2,-0x1=2e5}

]], parse.int_to_num_map)

slash.register_type("numbers", [[
Multiple Numbers

List of numbers comma separated

Examples:
- 0,2.2,32,-4       # {0,2.2,32,-4}
- 0,2.2,2e2,1e-8    # {0,2.2,2e2,1e-8}

]], parse.numbers)

return parse
