local slash = require "slash"

slash.register_chip_command({"eip", "topology"}, credo.FAMILY_OSPREY, [[
Get / set the eip topology for the slice

Arguments:

    <topology>    (optional top1|top2|top3)    Topology to set as

Examples:

    > eip topology         # get topology
    > eip topology top1    # set eip to topology 1

]], function(slice, argt)
    if argt.topology ~= nil then
        local topology_num = string.sub(argt.topology, 4)
        local topology = "topology_%s" % {topology_num}
        credo.eip_set_topology(slice, topology)
    end
    print("EIP Toplogy: %s" % {credo.eip_get_topology(slice)})
end)
