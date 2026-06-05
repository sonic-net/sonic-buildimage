local slash = require "slash"
local fort = require "fort"

slash.register_chip_command({"pcs", "info"}, credo.FAMILY_OSPREY, [[
Display pcs info for the slice.
]], function(slice, argt)
    credo.display_info_print(slice, "pcs")
end)

slash.register_chip_command({"pcs", "inject"}, credo.FAMILY_OSPREY, [[
Inject packets to pcs port

Arguments:

    <ports>            (portlist)
    <side>             (portside)
    -i,--infinite                                     Send packets infinitely
    -m,--mode          (func|debug default debug)     Packect injection mode
    -d,--data          (optional intlist)             Custom data to send. Max size of 512
    -l,--packet-len     (optional integer)
    -n,--packet-num    (optional integer)
    -v,--verbose                                      Print out packet configuration

]], function(slice, argt)
    local pkt_config = credo.PacketInjectConfig()
    pkt_config.infinite = argt.infinite
    pkt_config.mode = choose(argt.mode == "debug", credo.PKTINJ_DEBUG, credo.PKTINJ_FUNC)
    pkt_config.packet_data = argt.data
    pkt_config.packet_number = argt.packet_num
    pkt_config.packet_len = argt.packet_len
    if argt.verbose then
        print(pkt_config)
    end

    if pkt_config.packet_data ~= nil then
        assert(pkt_config.packet_data_len <= 512, "Packet data too large (> 512)")
    end

    for port in iter(argt.ports) do
        credo.eip_inject_packet(slice, port, argt.side, pkt_config)
    end

end)

slash.register_chip_command({"pcs", "capture"}, credo.FAMILY_OSPREY, [[
Capture packets from pcs port

Arguments:
    <ports>            (portlist)
    <side>             (portside)
    -m,--mode          (optional integer default 0)     Packect capture mode
    -p,--vport         (optional integer default 0)     Vport for conditional capturing
    -s,--stop                                           Stop packet capture
    -v,--verbose                                        Print out packet capture configuration
]], function(slice, argt)
    if argt.stop then
        for port in iter(argt.ports) do
            credo.eip_stop_capture_packet(slice, port, argt.side)
        end
    else
        local pkt_config = credo.PacketCaptureConfig()
        pkt_config.mode = argt.mode
        pkt_config.vport_index = argt.vport

        if argt.verbose then
            print(pkt_config)
        end

        for port in iter(argt.ports) do
            credo.eip_setup_capture_packet(slice, port, argt.side, pkt_config)
        end
    end
end)

slash.register_chip_command({"pcs", "capture", "status"}, credo.FAMILY_OSPREY, [[
Capture packet buffer status

Arguments:
    <side>             (portside)
    -p,--packet                     Pop packet data
    -c,--clear                      Clear capture buffer

]], function(slice, argt)
    local port = 0 -- ignore port number because of co-buffer for all ports
    if argt.clear then
        credo.eip_clear_capture_packet_buffer(slice, port, argt.side)
        print("clear capture buffer")
    elseif argt.packet then
        local pkt_info = credo.eip_get_capture_packet_info(slice, port, argt.side)
        if pkt_info.size ~= 0 then
            local ftable = fort.create_table()
            fort.printf(ftable, "Offset")
            for i = 1, 16 do
                fort.printf(ftable, "0x%02X", i - 1)
            end
            fort.ln(ftable)
            fort.add_separator(ftable)
            for i = 1, pkt_info.size do
                if i % 16 == 1 then
                    fort.printf(ftable, "0x%04X", (i - 1))
                end

                fort.printf(ftable, "0x%02X", pkt_info.packet_data[i])
                if i % 16 == 0 then
                    fort.ln(ftable)
                end
            end
            fort.ln(ftable)
            print(fort.to_string(ftable))
        end
        print("size %d" % {pkt_info.size})
        print(pkt_info.seg_status)
    else
        local status = credo.eip_get_capture_packet_buffer_status(slice, port, argt.side)
        print(status)
    end
end)
