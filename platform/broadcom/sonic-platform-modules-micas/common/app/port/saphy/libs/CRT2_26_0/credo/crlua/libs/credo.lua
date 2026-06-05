local enum = require "enum"
local _credo = require "_credo"
local shlex = require "shlex"

---@module 'credo'
local credo = {}

---@param slice integer
---@param lane integer
---@param pageId integer
---@param page integer
function credo.autoneg_set_pages(slice, lane, pageId, page)
    _credo.autoneg_set_pages(slice, lane, pageId, page)
end

---@param slice integer
---@param lane integer
---@return integer page_count
---@return integer[] tx_pages
---@return integer[] rx_pages
function credo.autoneg_get_exchanged_pages(slice, lane)
    local page_count, tx_pages, rx_pages = _credo.autoneg_get_exchanged_pages(slice, lane)
    return page_count, tx_pages, rx_pages
end

---@class credo.AutoNegState: enum.IntEnum
---@class credo.AutoNegStateEnum : enum.IntEnumMeta
---@field public UNKNOWN credo.AutoNegState
---@field public OFF credo.AutoNegState
---@field public ENABLE credo.AutoNegState
---@field public TX_DISABLE credo.AutoNegState
---@field public ABILITY_DETECT credo.AutoNegState
---@field public ACK_DETECT credo.AutoNegState
---@field public COMPLETE_ACK credo.AutoNegState
---@field public NP_WAIT credo.AutoNegState
---@field public GOOD_CHECK credo.AutoNegState
---@field public LINK_STATUS_CHECK credo.AutoNegState
---@field public PARALLEL_DET_FAULT credo.AutoNegState
---@field public GOOD credo.AutoNegState

---@type credo.AutoNegStateEnum
credo.AutoNegState = enum.IntEnum("AutoNegState", {
    UNKNOWN = _credo.AUTONEG_STATE_UNKNOWN,
    OFF = _credo.AUTONEG_STATE_OFF,
    ENABLE = _credo.AUTONEG_STATE_ENABLE,
    TX_DISABLE = _credo.AUTONEG_STATE_TX_DISABLE,
    ABILITY_DETECT = _credo.AUTONEG_STATE_ABILITY_DETECT,
    ACK_DETECT = _credo.AUTONEG_STATE_ACK_DETECT,
    COMPLETE_ACK = _credo.AUTONEG_STATE_COMPLETE_ACK,
    NP_WAIT = _credo.AUTONEG_STATE_NP_WAIT,
    GOOD_CHECK = _credo.AUTONEG_STATE_GOOD_CHECK,
    LINK_STATUS_CHECK = _credo.AUTONEG_STATE_LINK_STATUS_CHECK,
    PARALLEL_DET_FAULT = _credo.AUTONEG_STATE_PARALLEL_DET_FAULT,
    GOOD = _credo.AUTONEG_STATE_GOOD
})

---@param slice integer
---@param lane integer
---@return credo.AutoNegState
function credo.autoneg_get_state(slice, lane)
    return credo.AutoNegState(_credo.autoneg_get_state(slice, lane))
end

---@param slice integer
---@param lane integer
---@return integer
function credo.autoneg_get_restart_count(slice, lane)
    return _credo.autoneg_get_restart_count(slice, lane)
end

---@param slice integer
---@param clock_output integer
function credo.clockout_disable(slice, clock_output)
    _credo.clockout_disable(slice, clock_output)
end

---@param slice integer
function credo.clockout_disable_all(slice)
    _credo.clockout_disable_all(slice)
end

---@param slice integer
---@param clock_output integer
---@param lane integer
---@param divider integer
function credo.clockout_enable(slice, clock_output, lane, divider)
    _credo.clockout_enable(slice, clock_output, lane, divider)
end

---@param slice integer
---@param command string
---@param args table<string, integer> arguments to pass
---@return string captured_data
function credo.data_capture(slice, command, args)
    return _credo.data_capture_buffer(slice, command, args)
end

---@param slice integer
---@param command string
---@param file string
---@param args table<string, integer> arguments to pass
function credo.data_capture_file(slice, command, file, args)
    _credo.data_capture_file(slice, command, args, file)
end

---@param slice integer
---@param commands string[]|string
function credo.display_info_log(slice, commands)
    if type(commands) == "string" then
        commands = shlex.split(commands)
    end
    _credo.display_info_log(slice, commands)
end

---comment
---@param slice integer
---@param commands string[]|string
---@return string output
---@return string|nil error_info error code information
function credo.display_info_catch(slice, commands)
    if type(commands) == "string" then
        commands = shlex.split(commands)
    end
    local error, output = _credo.display_info_buffer(slice, commands)
    return output, error
end

---@param slice integer
---@param commands string[]|string
---@return string
function credo.display_info(slice, commands)
    local output, error = credo.display_info_catch(slice, commands)
    if error ~= nil then
        output = output .. error
    end
    return output
end

---@param slice integer
---@param commands string[]|string
function credo.display_info_print(slice, commands)
    local output = credo.display_info(slice, commands)
    -- only print if something to display
    if stringx.strip(output) ~= "" then
        print(output)
    end
end

---@param slice integer
---@param commands string[]|string
---@param file string
function credo.display_info_file(slice, commands, file)
    if type(commands) == "string" then
        commands = shlex.split(commands)
    end
    _credo.display_info_file(slice, commands, file)
end

---@param slice integer
---@param bank integer
---@return integer
function credo.efuse_read(slice, bank)
    return _credo.efuse_read(slice, bank)
end

---@param slice integer
---@return integer[]
function credo.efuse_read_ecid(slice)
    return _credo.efuse_read_ecid(slice)
end

---@param slice integer
---@return string
function credo.efuse_read_ecid_str(slice)
    local vals = credo.efuse_read_ecid(slice)
    return "0x%08X%08X" % {vals[1], vals[2]}
end

---@param slice integer
---@param portId integer
function credo.eip_configure(slice, portId)
    _credo.eip_configure(slice, portId)
end

---@param slice integer
function credo.eip_display_topologies(slice)
    _credo.eip_display_topologies(slice)
end

---@param slice integer
---@param portId integer
---@return integer
function credo.eip_get_channel_id(slice, portId)
    return _credo.eip_get_channel_id(slice, portId)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@return integer
function credo.eip_get_client_id(slice, portId, side)
    return _credo.eip_get_client_id(slice, portId, side.value)
end

---@param slice integer
---@param portId integer
---@param direction credo.MACsecDirection
---@return boolean
function credo.eip_get_low_latency_bypass(slice, portId, direction)
    return _credo.eip_get_low_latency_bypass(slice, portId, direction.value) ~= 0
end

---@class credo.MACsecDataPath
---@field public start_offset1 integer
---@field public end_offset1 integer
---@field public start_offset2 integer
---@field public end_offset2 integer
---@overload fun(): credo.MACsecDataPath
credo.MACsecDataPath = _credo.MACsecDataPath

---@param slice integer
---@param direction integer
---@return credo.MACsecDataPath
function credo.eip_get_macsec_datapath(slice, direction)
    local datapath = credo.MACsecDataPath()
    _credo.eip_get_macsec_datapath(slice, direction, datapath)
    return datapath
end

---@param slice integer
---@return string
function credo.eip_get_topology(slice)
    return _credo.eip_get_topology(slice)
end

---@class credo.PacketInjectConfig
---@field public packet_data integer[]|nil
---@field public mode credo.PacketInjectMode|nil
---@field public packet_data_len number
---@field public packet_len integer|nil
---@field public infinite boolean|nil
---@field public packet_number integer|nil
---@overload fun():credo.PacketInjectConfig
credo.PacketInjectConfig = _credo.PacketInjectConfig

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@param packet_config credo.PacketInjectConfig
function credo.eip_inject_packet(slice, port_id, side, packet_config)
    _credo.eip_inject_packet(slice, port_id, side.value, packet_config)
end

---@class credo.TcamPacketData
---@field public dst_macaddr integer[]
---@field public ethertype integer[]
---@field public extension integer[]
credo.TcamPacketData = _credo.TcamPacketData

---@class credo.PacketCaptureConfig
---@field public mode integer
---@field public vport_index integer
---@field public rule_index integer
---@field public key credo.TcamPacketData
---@field public mask credo.TcamPacketData
---@overload fun(): credo.PacketCaptureConfig
credo.PacketCaptureConfig = _credo.PacketCaptureConfig

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param pkt_config credo.PacketCaptureConfig
function credo.eip_setup_capture_packet(slice, portId, side, pkt_config)
    _credo.eip_setup_capture_packet(slice, portId, side.value, pkt_config)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
function credo.eip_stop_capture_packet(slice, portId, side)
    _credo.eip_stop_capture_packet(slice, portId, side.value)
end

---@class credo.PacketCaptureBufferStatus
---@field public captured_packets integer
---@field public filled_segments integer
---@field public dropped_packets integer
---@overload fun(): credo.PacketCaptureBufferStatus
credo.PacketCaptureBufferStatus = _credo.PacketCaptureBufferStatus

---@param slice integer
---@param portId integer
---@param side credo.Side
---@return credo.PacketCaptureBufferStatus
function credo.eip_get_capture_packet_buffer_status(slice, portId, side)
    local status = credo.PacketCaptureBufferStatus()
    _credo.eip_get_capture_packet_buffer_status(slice, portId, side.value, status)
    return status
end

---@param slice integer
---@param portId integer
---@param side credo.Side
function credo.eip_clear_capture_packet_buffer(slice, portId, side)
    _credo.eip_clear_capture_packet_buffer(slice, portId, side.value)
end

---@class credo.PacketCaptureSegmentStatus
---@field public truncated boolean
---@field public port_id integer
---@field public vport_id integer
---@overload fun(): credo.PacketCaptureSegmentStatus
credo.PacketCaptureSegmentStatus = _credo.PacketCaptureSegmentStatus

---@class credo.PacketCaptureInfo
---@field public packet_data integer[]
---@field public size integer
---@field public seg_status credo.PacketCaptureSegmentStatus
---@overload fun(): credo.PacketCaptureInfo
credo.PacketCaptureInfo = _credo.PacketCaptureInfo

---@param slice integer
---@param portId integer
---@param side credo.Side
---@return credo.PacketCaptureInfo
function credo.eip_get_capture_packet_info(slice, portId, side)
    local pkt_info = credo.PacketCaptureInfo()
    _credo.eip_get_capture_packet_info(slice, portId, side.value, pkt_info)
    return pkt_info
end

---@param slice integer
---@param portId integer
---@param direction credo.MACsecDirection
---@param enable boolean
function credo.eip_set_low_latency_bypass(slice, portId, direction, enable)
    _credo.eip_set_low_latency_bypass(slice, portId, direction.value, int(enable))
end

---@param slice integer
---@param topology string
function credo.eip_set_topology(slice, topology)
    _credo.eip_set_topology(slice, topology)
end

---@param slice integer
---@param portId integer
function credo.eip_stop(slice, portId)
    _credo.eip_stop(slice, portId)
end

---@class credo.FastRecoverStatus: enum.IntEnum
---@class credo.FastRecoverStatusEnum : enum.IntEnumMeta
---@field public DISABLED credo.FastRecoverStatus
---@field public ENABLED credo.FastRecoverStatus
---@field public ARMED credo.FastRecoverStatus

---@type credo.FastRecoverStatusEnum
credo.FastRecoverStatus = enum.IntEnum("FastRecoverStatus", {
    DISABLED = _credo.FRECOV_DISABLED,
    ENABLED = _credo.FRECOV_ENABLED,
    ARMED = _credo.FRECOV_ARMED
})

---@param slice integer
---@param lane integer
---@param timeout_ms integer
function credo.frecov_configure(slice, lane, timeout_ms)
    _credo.frecov_configure(slice, lane, timeout_ms)
end

---@param slice integer
---@param lane integer
---@return integer timeout_ms
---@return credo.FastRecoverStatus status
function credo.frecov_get_status(slice, lane)
    local timeout_ms, status = _credo.frecov_get_status(slice, lane)
    return timeout_ms, credo.FastRecoverStatus(status)
end

---@param slice integer
---@param lane integer
---@return integer count
function credo.frecov_get_recover_count(slice, lane)
    return _credo.frecov_get_recover_count(slice, lane)
end

---@class credo.FecAnalyzerConfig
---@field public codeword_size integer
---@field public symbol_size integer
---@field public threshold integer
---@field public error_type credo.FecErrorType
---@overload fun():credo.FecAnalyzerConfig
credo.FecAnalyzerConfig = _credo.FecAnalyzerConfig

---@param slice integer
---@param lane integer
---@param enable boolean
---@param config credo.FecAnalyzerConfig
function credo.fecana_configure(slice, lane, enable, config)
    _credo.fecana_configure(slice, lane, int(enable), config)
end

---@param slice integer
---@param lane integer
function credo.fecana_reset(slice, lane)
    _credo.fecana_reset(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer, integer
function credo.fecana_get_counter(slice, lane)
    return _credo.fecana_get_counter(slice, lane)
end

---@param slice integer
---@param lane integer
---@return number
function credo.fecana_get_duration(slice, lane)
    return _credo.fecana_get_duration(slice, lane)
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.fecana_get_autosync(slice, lane)
    return _credo.fecana_get_autosync(slice, lane)
end

---@param slice integer
---@param lane integer
---@param counter_sel integer
---@param duration_ms integer
---@return number
function credo.fecana_get_error_rate(slice, lane, counter_sel, duration_ms)
    return _credo.fecana_get_error_rate(slice, lane, counter_sel, duration_ms)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.fecana_get_hist_counter(slice, lane)
    return _credo.fecana_get_hist_counter(slice, lane)
end

---@param slice integer
---@param lane integer
---@param counter_sel integer
---@return integer
function credo.fecana_get_raw_counter(slice, lane, counter_sel)
    return _credo.fecana_get_raw_counter(slice, lane, counter_sel)
end

---@param slice integer
---@param lane integer
---@return boolean, credo.FecAnalyzerConfig
function credo.fecana_query(slice, lane)
    local fec_config = credo.FecAnalyzerConfig()
    local enable = _credo.fecana_query(slice, lane, fec_config)
    return enable ~= 0, fec_config
end

---@param slice integer
---@param lane integer
---@param group integer
function credo.fecana_set_hist_group(slice, lane, group)
    _credo.fecana_set_hist_group(slice, lane, group)
end

---@param slice integer
---@param lane integer
---@return integer group
function credo.fecana_get_hist_group(slice, lane)
    return _credo.fecana_get_hist_group(slice, lane)
end

---@param slice integer
---@param cmd integer
---@param param integer
---@param silent? boolean no log on error, defaults to false
---@return integer, integer
function credo.firmware_cmd(slice, cmd, param, silent)
    silent = silent or false
    return _credo.firmware_cmd(slice, cmd, param, silent)
end

---@param slice integer
---@param cmd integer
---@param param1 integer
---@param param2 integer
---@param silent? boolean no log on error, defaults to false
---@return integer, integer, integer
function credo.firmware_cmd_ex(slice, cmd, param1, param2, silent)
    silent = silent or false
    return _credo.firmware_cmd_ex(slice, cmd, param1, param2, silent)
end

---@param slice integer
---@return integer
function credo.firmware_crc(slice)
    return _credo.firmware_crc(slice)
end

---@param slice integer
---@return integer
function credo.firmware_date(slice)
    return _credo.firmware_date(slice)
end

---@param slice integer
---@param lane integer
---@param section integer
---@param index integer
---@param silent? boolean no log on error, defaults to false
---@return integer
function credo.firmware_debug_cmd(slice, lane, section, index, silent)
    silent = silent or false
    return _credo.firmware_debug_cmd(slice, lane, section, index, silent)
end

---@param slice integer
---@param lane integer
---@param section integer
---@param index integer
---@param silent? boolean no log on error, defaults to false
---@return integer, integer
function credo.firmware_debug_cmd_ex(slice, lane, section, index, silent)
    silent = silent or false
    return _credo.firmware_debug_cmd_ex(slice, lane, section, index, silent)
end

---@param slice integer
---@return integer
function credo.firmware_get_status(slice)
    return _credo.firmware_get_status(slice)
end

---@param slice integer
---@return integer
function credo.firmware_hash(slice)
    return _credo.firmware_hash(slice)
end

---@param slice integer
---@param image_file string
---@param force? boolean default is not force
---@return any
function credo.firmware_load(slice, image_file, force)
    force = force or false
    local force_int = int(force)

    return _credo.firmware_load(slice, image_file, force_int)
end

---@param slices integer[]
---@param firmware string
---@param delay_us? integer default is 5us
---@param force? boolean default is not force
function credo.firmware_load_broadcast(slices, firmware, delay_us, force)
    delay_us = delay_us or 5
    force = force or false
    _credo.firmware_load_broadcast(slices, firmware, delay_us, int(force))
end

---@param slice integer
---@return integer
function credo.firmware_magic(slice)
    return _credo.firmware_magic(slice)
end

---@param slice integer
---@param addr integer
---@return integer
function credo.firmware_reg_rd(slice, addr)
    return _credo.firmware_reg_rd(slice, addr)
end

---@param slice integer
---@param addr integer
---@param section integer
---@return integer
function credo.firmware_reg_rd_ex(slice, addr, section)
    return _credo.firmware_reg_rd_ex(slice, addr, section)
end

---@param slice integer
---@param addr integer
---@param value integer
function credo.firmware_reg_wr(slice, addr, value)
    _credo.firmware_reg_wr(slice, addr, value)
end

---@param slice integer
---@param addr integer
---@param section integer
---@param value integer
function credo.firmware_reg_wr_ex(slice, addr, section, value)
    _credo.firmware_reg_wr_ex(slice, addr, section, value)
end

---@param slice integer
function credo.firmware_unload(slice)
    _credo.firmware_unload(slice)
end

---@param slice integer
---@return integer
function credo.firmware_version(slice)
    return _credo.firmware_version(slice)
end

---@param slice integer
---@return string
function credo.firmware_version_str(slice)
    local fw_ver = string.rep(" ", 32, "")
    _credo.firmware_version_str(slice, fw_ver)
    fw_ver = stringx.rstrip(fw_ver, " \0")
    return fw_ver
end

---@param slice integer
---@param lane integer
---@param lane_mode credo.LaneMode
---@param speed integer
function credo.lane_configure_mode(slice, lane, lane_mode, speed)
    _credo.lane_configure_mode(slice, lane, lane_mode.value, speed)
end

---@param slice integer
---@param lane integer
---@param speed integer
function credo.lane_configure_mode_loopback(slice, lane, speed)
    _credo.lane_configure_mode_loopback(slice, lane, speed)
end

---@param slice integer
---@param lane integer
function credo.lane_destroy_mode(slice, lane)
    _credo.lane_destroy_mode(slice, lane)
end

---@param slice integer
---@param lane integer
function credo.lane_disable(slice, lane)
    _credo.lane_disable(slice, lane)
end

---@param slice integer
---@return integer host_lanes
---@return integer line_lanes
function credo.lane_get_count(slice)
    local host_count, line_count = _credo.lane_get_count(slice)
    return host_count, line_count
end

---@param slice integer
---@param lane integer
---@return credo.LaneMode
function credo.lane_get_mode(slice, lane)
    local mode = _credo.lane_get_mode(slice, lane)
    return credo.LaneMode(mode)
end

---@param slice integer
---@param lane integer
---@return credo.LaneMode
function credo.lane_get_tx_mode(slice, lane)
    local mode = _credo.lane_get_tx_mode(slice, lane)
    return credo.LaneMode(mode)
end

---@param slice integer
---@param lane integer
---@param option_name string
---@return integer
function credo.lane_get_option(slice, lane, option_name)
    return _credo.lane_get_option(slice, lane, option_name)
end

---@class credo.LaneOption
---@field public name string
---@field public description string
---@overload fun():credo.LaneOption
credo.LaneOption = _credo.LaneOption

---@param slice integer
---@return credo.LaneOption[]
function credo.lane_get_option_list(slice)
    ---@type credo.LaneOption[]
    local option_list = {}
    local count = _credo.lane_get_option_count(slice)
    for index = 0, count - 1 do
        local option = credo.LaneOption()
        ---@type boolean
        _credo.lane_index_option_list(slice, index, option)
        list.append(option_list, option)
    end
    return option_list
end

---@param slice integer
---@param lane integer
---@return integer
function credo.lane_get_speed(slice, lane)
    return _credo.lane_get_speed(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.lane_get_tx_speed(slice, lane)
    return _credo.lane_get_tx_speed(slice, lane)
end

---@param slice integer
---@param option string
---@return boolean
function credo.lane_is_option_supported(slice, option)
    local status = pcall(_credo.lane_is_option_supported, slice, option)
    if not status then
        return false
    else
        return true
    end
end

---@param slice integer
---@param lane integer
function credo.lane_logic_reset(slice, lane)
    _credo.lane_logic_reset(slice, lane)
end

---@param slice integer
---@param lane integer
function credo.lane_reg_reset(slice, lane)
    _credo.lane_reg_reset(slice, lane)
end

---@param slice integer
---@param lane integer
---@param enable boolean
function credo.lane_rx_force_squelch(slice, lane, enable)
    _credo.lane_rx_force_squelch(slice, lane, enable)
end

---@param slice integer
---@param lane integer
function credo.lane_rx_reset(slice, lane)
    _credo.lane_rx_reset(slice, lane)
end

---@param slice integer
---@param lane integer
---@param mode credo.LaneLoopbackMode
function credo.lane_set_loopback_mode(slice, lane, mode)
    _credo.lane_set_loopback_mode(slice, lane, mode.value)
end

---@param slice integer
---@param lane integer
---@return credo.LaneLoopbackMode
function credo.lane_get_loopback_mode(slice, lane)
    local lb_mode = _credo.lane_get_loopback_mode(slice, lane)
    return credo.LaneLoopbackMode(lb_mode)
end

---@param slice integer
---@param lane integer
---@param mode credo.LaneMode
function credo.lane_set_mode(slice, lane, mode)
    _credo.lane_set_mode(slice, lane, mode.value)
end

---@param slice integer
---@param lane integer
---@param option_name string
---@param value integer
function credo.lane_set_option(slice, lane, option_name, value)
    _credo.lane_set_option(slice, lane, option_name, value.value)
end

---@param slice integer
---@param lane integer
function credo.lane_rx_disable(slice, lane)
    _credo.lane_rx_disable(slice, lane)
end

---@param slice integer
---@param lane integer
function credo.lane_rx_no_disable(slice, lane)
    _credo.lane_rx_no_disable(slice, lane)
end

---@param slice integer
---@param lane integer
function credo.lane_tx_disable(slice, lane)
    _credo.lane_tx_disable(slice, lane)
end

---@param slice integer
---@param lane integer
---@param enable boolean
function credo.lane_tx_force_squelch(slice, lane, enable)
    _credo.lane_tx_force_squelch(slice, lane, enable)
end

---@param slice integer
---@param lane integer
---@return credo.LaneTxState
function credo.lane_tx_get_status(slice, lane)
    local tx_state = _credo.lane_tx_get_status(slice, lane)
    return credo.LaneTxState(tx_state)
end

---@param slice integer
---@param lane integer
function credo.lane_tx_no_disable(slice, lane)
    _credo.lane_tx_no_disable(slice, lane)
end

---@param slice integer
---@param lane integer
function credo.lane_update_mode(slice, lane)
    _credo.lane_update_mode(slice, lane)
end

---@param slice integer
---@param portId integer
function credo.mac_configure(slice, portId)
    _credo.mac_configure(slice, portId)
end

---@param slice integer
---@param portId integer
---@return integer
function credo.mac_get_faultprop(slice, portId)
    local fault_prop = _credo.mac_get_faultprop(slice, portId)
    return credo.FaultPropagation(fault_prop)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@return integer
function credo.mac_read(slice, portId, side, offset)
    return _credo.mac_read(slice, portId, side.value, offset)
end

---@param slice integer
---@param portId integer
---@param fault_prop credo.FaultPropagation
function credo.mac_set_faultprop(slice, portId, fault_prop)
    _credo.mac_set_faultprop(slice, portId, fault_prop.value)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
function credo.mac_stats_clear_counters(slice, portId, side)
    _credo.mac_stats_clear_counters(slice, portId, side.value)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@return integer
function credo.mac_stats_read(slice, portId, side, offset)
    return _credo.mac_stats_read(slice, portId, side.value, offset)
end

---@class credo.MACStatistics
---@field public etherStatsTxPkts128to255Octets number
---@field public aCBFCPAUSEFramesReceived_7 number
---@field public aPAUSEMACCtrlFramesTransmitted number
---@field public etherStatsRxOctets number
---@field public aInRangeLengthErrors number
---@field public aCBFCPAUSEFramesTransmitted_7 number
---@field public aCBFCPAUSEFramesReceived_6 number
---@field public aCBFCPAUSEFramesTransmitted_0 number
---@field public aCBFCPAUSEFramesReceived_4 number
---@field public etherStatsRxPkts256to511Octets number
---@field public ifOutMulticastPkts number
---@field public aCBFCPAUSEFramesReceived_2 number
---@field public aCBFCPAUSEFramesTransmitted_5 number
---@field public etherStatsRxPkts1024to1518Octets number
---@field public etherStatsRxPkts512to1023Octets number
---@field public etherStatsRxPkts1519toMaxOctets number
---@field public etherStatsTxOctets number
---@field public aFramesReceivedOK number
---@field public aMACControlFramesTransmitted number
---@field public aCBFCPAUSEFramesReceived_0 number
---@field public ifInErrors number
---@field public ifOutBroadcastPkts number
---@field public aCBFCPAUSEFramesTransmitted_3 number
---@field public etherStatsRxPkts64Octets number
---@field public aCBFCPAUSEFramesTransmitted_2 number
---@field public etherStatsRxPkts number
---@field public etherStatsRxPkts65to127Octets number
---@field public etherStatsRxPkts128to255Octets number
---@field public aPAUSEMACCtrlFramesReceived number
---@field public ifOutUcastPkts number
---@field public OctetsReceivedOK number
---@field public aAlignmentErrors number
---@field public ifInUcastPkts number
---@field public etherStatsUndersizePkts number
---@field public aFramesTransmittedOK number
---@field public etherStatsTxPkts64Octets number
---@field public ifOutErrors number
---@field public VLANTransmittedOK number
---@field public aFrameTooLongErrors number
---@field public aMACControlFramesReceived number
---@field public aCBFCPAUSEFramesReceived_5 number
---@field public aFrameCheckSequenceErrors number
---@field public etherStatsFragments number
---@field public etherStatsTxPkts512to1023Octets number
---@field public etherStatsTxPkts1519toMaxOctets number
---@field public OctetsTransmittedOK number
---@field public aCBFCPAUSEFramesTransmitted_4 number
---@field public aCBFCPAUSEFramesTransmitted_6 number
---@field public ifInMulticastPkts number
---@field public etherStatsTxPkts1024to1518Octets number
---@field public etherStatsJabbers number
---@field public aCBFCPAUSEFramesReceived_3 number
---@field public etherStatsTxPkts256to511Octets number
---@field public aCBFCPAUSEFramesReceived_1 number
---@field public etherStatsTxPkts number
---@field public aCBFCPAUSEFramesTransmitted_1 number
---@field public VLANReceivedOK number
---@field public etherStatsOversizePkts number
---@field public etherStatsDropEvents number
---@field public ifInBroadcastPkts number
---@field public etherStatsTxPkts65to127Octets number
---@overload fun():credo.MACStatistics
credo.MACStatistics = _credo.MACStatistics

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return credo.MACStatistics
function credo.mac_stats_read_counters(slice, port_id, side)
    local mac_stats = credo.MACStatistics()
    _credo.mac_stats_read_counters(slice, port_id, side.value, mac_stats)
    return mac_stats
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@param val integer
function credo.mac_stats_write(slice, portId, side, offset, val)
    _credo.mac_stats_write(slice, portId, side.value, offset, val)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@return integer
function credo.mac_status_read(slice, portId, side)
    return _credo.mac_status_read(slice, portId, side.value)
end

---@param slice integer
---@param portId integer
function credo.mac_stop(slice, portId)
    _credo.mac_stop(slice, portId)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@param val integer
function credo.mac_write(slice, portId, side, offset, val)
    _credo.mac_write(slice, portId, side.value, offset, val)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@return integer
function credo.pcs_read(slice, portId, side, offset)
    return _credo.pcs_read(slice, portId, side.value, offset)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@return integer
function credo.pcs_status_read(slice, portId, side)
    return _credo.pcs_status_read(slice, portId, side.value)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@param val integer
function credo.pcs_write(slice, portId, side, offset, val)
    _credo.pcs_write(slice, portId, side.value, offset, val)
end

---@class credo.PortConfig
---@field public host_start_lane integer
---@field public port_id credo.PortID
---@field public line_fec_type credo.FecType
---@field public port_mode credo.PortMode
---@field public line_start_lane integer
---@field public speed number
---@field public line_no_of_lanes integer
---@field public flags credo.PortFlags|integer
---@field public host_no_of_lanes integer
---@field public host_fec_type credo.FecType
---@field public port_type credo.PortType
---@overload fun():credo.PortConfig
credo.PortConfig = _credo.PortConfig

---@class credo.PortSetup
---@field public mode credo.PortType
---@field public speed integer
---@field public host_lanes integer[]
---@field public line_lanes integer[]
---@overload fun():credo.PortSetup
credo.PortSetup = _credo.PortSetup

---@param slice integer
---@param port_config credo.PortConfig
---@param force? boolean
---@return credo.PortConfig
function credo.port_configure(slice, port_config, force)
    local force_int = 0
    if not (not force) then
        force_int = 1
    end

    _credo.port_configure(slice, port_config, force_int)
    return port_config
end

---@param slice integer
---@param portId integer
function credo.port_destroy(slice, portId)
    _credo.port_destroy(slice, portId)
end

---@param slice integer
function credo.port_destroy_all(slice)
    _credo.port_destroy_all(slice)
end

---@param slice integer
---@param port_id integer
---@return credo.PortConfig
function credo.port_query(slice, port_id)
    local port_config = credo.PortConfig()
    _credo.port_query(slice, port_id, port_config)
    return port_config
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return boolean
function credo.port_is_link_up(slice, port_id, side)
    return _credo.port_is_link_up(slice, port_id, side.value)
end

---@param slice integer
---@param lane integer
function credo.prbs_generate_tx_error(slice, lane)
    _credo.prbs_generate_tx_error(slice, lane)
end

---@param slice integer
---@param lane integer
---@param time_sec? number
---@return number
function credo.prbs_get_rx_ber(slice, lane, time_sec)
    time_sec = time_sec or 0

    return _credo.prbs_get_rx_ber(slice, lane, int(time_sec * 1000))
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.prbs_get_rx_autosync(slice, lane)
    return _credo.prbs_get_rx_autosync(slice, lane)
end

---@param slice integer
---@param lanes integer[]
---@param time_sec? number
---@return number
function credo.prbs_get_rx_ber_all(slice, lanes, time_sec)
    local ber = range(#lanes)

    return _credo.cri_prbs_get_rx_ber_all(slice, lanes, int(time_sec * 1000), ber)
end

---@param slice integer
---@param lane integer
---@return boolean, credo.PrbsPattern
function credo.prbs_get_rx_checker(slice, lane)
    local enable, mode = _credo.prbs_get_rx_checker(slice, lane)
    return enable ~= 0, credo.PrbsPattern(mode)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.prbs_get_rx_count(slice, lane)
    return _credo.prbs_get_rx_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.prbs_get_rx_duration(slice, lane)
    return _credo.prbs_get_rx_duration(slice, lane)
end

---@param slice integer
---@param lane integer
---@return credo.PrbsLockStatus
function credo.prbs_get_rx_lock(slice, lane)
    local lock = _credo.prbs_get_rx_lock(slice, lane)
    return credo.PrbsLockStatus(lock)
end

---@param slice integer
---@param lane integer
---@return boolean, credo.PrbsPattern
function credo.prbs_get_tx_generator(slice, lane)
    local enable, mode = _credo.prbs_get_tx_generator(slice, lane)
    return enable ~= 0, credo.PrbsPattern(mode)
end

---@param slice integer
---@param lane integer
function credo.prbs_reset_rx_count(slice, lane)
    _credo.prbs_reset_rx_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@param enable boolean
---@param mode credo.PrbsPattern
function credo.prbs_set_rx_checker(slice, lane, enable, mode)
    _credo.prbs_set_rx_checker(slice, lane, int(enable), mode.value)
end

---@param slice integer
---@param lane integer
---@param enable boolean
---@param mode credo.PrbsPattern
function credo.prbs_set_tx_generator(slice, lane, enable, mode)
    _credo.prbs_set_tx_generator(slice, lane, int(enable), mode.value)
end

---@param slice integer
---@param lane integer
---@param time_sec? number
---@return number, number
function credo.prbs_get_tx_ber(slice, lane, time_sec)
    time_sec = time_sec or 0

    return _credo.prbs_get_tx_ber(slice, lane, int(time_sec * 1000))
end

---@param slice integer
---@param lane integer
---@return boolean, credo.PrbsPattern
function credo.prbs_get_tx_checker(slice, lane)
    local enable, mode = _credo.prbs_get_tx_checker(slice, lane)
    return enable ~= 0, credo.PrbsPattern(mode)
end

---@param slice integer
---@param lane integer
---@return integer, integer
function credo.prbs_get_tx_count(slice, lane)
    return _credo.prbs_get_tx_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.prbs_get_tx_duration(slice, lane)
    return _credo.prbs_get_tx_duration(slice, lane)
end

---@param slice integer
---@param lane integer
function credo.prbs_reset_tx_count(slice, lane)
    _credo.prbs_reset_tx_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@param enable boolean
---@param mode credo.PrbsPattern
function credo.prbs_set_tx_checker(slice, lane, enable, mode)
    _credo.prbs_set_tx_checker(slice, lane, int(enable), mode.value)
end

---@param slice integer
---@param lane integer
---@return credo.PrbsTrainingStatus
function credo.prbs_training_rx_get_status(slice, lane)
    return credo.PrbsTrainingStatus(_credo.prbs_training_rx_get_status(slice, lane))
end

---@param slice integer
---@param lane integer
function credo.prbs_training_rx_relink(slice, lane)
    _credo.prbs_training_rx_relink(slice, lane)
end

---@param slice integer
---@param lane integer
---@param enable boolean
function credo.prbs_training_tx_enable(slice, lane, enable)
    _credo.prbs_training_tx_enable(slice, lane, enable)
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.prbs_training_tx_is_enabled(slice, lane)
    return _credo.prbs_training_tx_is_enabled(slice, lane)
end

function credo.prbs_pattchecker_set_symbol(slice, lane, en, symbol)
    _credo.prbs_pattchecker_set_symbol(slice, lane, en, symbol)
end

function credo.prbs_pattchecker_reset_count(slice, lane)
    _credo.prbs_pattchecker_reset_count(slice, lane)
end

function credo.prbs_pattchecker_get_count(slice, lane)
    return _credo.prbs_pattchecker_get_count(slice, lane)
end

function credo.prbs_pattchecker_set_phase(slice, lane, phase)
    return _credo.prbs_pattchecker_set_phase(slice, lane, phase)
end

---@class credo.RSFECStatus
---@field public fec_aligned boolean
---@field public pcs_aligned boolean
---@field public AM_locked boolean[]
---@overload fun():credo.RSFECStatus
credo.RSFECStatus = _credo.RSFECStatus

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return credo.RSFECStatus
function credo.rsfec_get_align_status(slice, port_id, side)
    local fec_status = credo.RSFECStatus()
    _credo.rsfec_get_align_status(slice, port_id, side.value, fec_status)
    return fec_status
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return number
function credo.rsfec_get_corrected_bits(slice, port_id, side)
    return _credo.rsfec_get_corrected_bits(slice, port_id, side.value)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return number
function credo.rsfec_get_corrected_codeword(slice, port_id, side)
    return _credo.rsfec_get_corrected_codeword(slice, port_id, side.value)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return boolean
function credo.rsfec_get_count_freeze(slice, port_id, side)
    return _credo.rsfec_get_count_freeze(slice, port_id, side.value)
end

---@class credo.RSFECFifo
---@field public rx_min number
---@field public rx_max number
---@field public tx_min number
---@field public tx_max number
---@field public tx_cur number
---@field public rx_cur number
---@overload fun():credo.RSFECFifo
credo.RSFECFifo = _credo.RSFECFifo

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return credo.RSFECFifo
function credo.rsfec_get_fifo(slice, port_id, side)
    local fec_fifo = credo.RSFECFifo()
    _credo.rsfec_get_fifo(slice, port_id, side.value, fec_fifo)
    return fec_fifo
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@param hist_bin integer
---@return number
function credo.rsfec_get_histogram(slice, port_id, side, hist_bin)
    return _credo.rsfec_get_histogram(slice, port_id, side.value, hist_bin)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return integer
function credo.rsfec_get_lane_mapping(slice, port_id, side)
    return _credo.rsfec_get_lane_mapping(slice, port_id, side.value)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@param fec_lane integer
---@return integer
function credo.rsfec_get_symbol_error(slice, port_id, side, fec_lane)
    return _credo.rsfec_get_symbol_error(slice, port_id, side.value, fec_lane)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return number
function credo.rsfec_get_total_codeword(slice, port_id, side)
    return _credo.rsfec_get_total_codeword(slice, port_id, side.value)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@return integer
function credo.rsfec_get_uncorrected_codeword(slice, port_id, side)
    return _credo.rsfec_get_uncorrected_codeword(slice, port_id, side.value)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@return integer
function credo.rsfec_read(slice, portId, side, offset)
    return _credo.rsfec_read(slice, portId, side.value, offset)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
function credo.rsfec_reset_count(slice, port_id, side)
    _credo.rsfec_reset_count(slice, port_id, side.value)
end

---@param slice integer
---@param port_id integer
---@param side credo.Side
---@param enable boolean
function credo.rsfec_set_count_freeze(slice, port_id, side, enable)
    _credo.rsfec_set_count_freeze(slice, port_id, side.value, enable)
end

---@param slice integer
---@param portId integer
---@param side credo.Side
---@param offset integer
---@param val integer
function credo.rsfec_write(slice, portId, side, offset, val)
    _credo.rsfec_write(slice, portId, side.value, offset, val)
end

---@class credo.RegVerifyStats
---@field reg_count integer
---@field fail_count integer
---@field duration_sec number
---@overload fun():credo.RegVerifyStats
credo.RegVerifyStats = _credo.RegVerifyStats

---@class credo.RegVerifyConfig
---@field flags credo.RegVerifyFlags
---@field test_count integer
---@field duration_sec number
---@field address integer
---@field burst_width integer
---@overload fun():credo.RegVerifyConfig
credo.RegVerifyConfig = _credo.RegVerifyConfig

---@class credo.RegVerifyFlags: enum.IntEnum
---@class credo.RegVerifyFlagsEnum : enum.IntEnumMeta
---@field public BURST credo.RegVerifyFlags
---@field public USE_DURATION credo.RegVerifyFlags
---@field public OVERRIDE_ADDRESS credo.RegVerifyFlags

---@type credo.RegVerifyFlagsEnum
credo.RegVerifyFlags = enum.IntEnum("RegVerifyFlags", {
    BURST = _credo.REG_VERIFY_BURST,
    USE_DURATION = _credo.REG_VERIFY_USE_DURATION,
    OVERRIDE_ADDRESS = _credo.REG_VERIFY_OVERRIDE_ADDRESS
}, true)

---@param slice integer
---@param config credo.RegVerifyConfig
---@return credo.RegVerifyStats
function credo.reg_verify(slice, config)
    local stats = credo.RegVerifyStats()
    _credo.reg_verify(slice, config, stats)
    return stats
end

---@return integer
function credo.sdk_version()
    return _credo.sdk_version()
end

---@return string
function credo.sdk_version_str()
    return _credo.sdk_version_str()
end

---@param slice integer
---@param lane integer
function credo.serdes_cal_top_pll(slice, lane)
    _credo.serdes_cal_top_pll(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_adapt_count(slice, lane)
    return _credo.serdes_get_adapt_count(slice, lane)
end

---@param slice integer
---@return integer
function credo.serdes_get_all_phy_ready(slice)
    return _credo.serdes_get_all_phy_ready(slice)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_link_lost_count(slice, lane)
    return _credo.serdes_get_link_lost_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_los_count(slice, lane)
    return _credo.serdes_get_los_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_phy_ready(slice, lane)
    return _credo.serdes_get_phy_ready(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_readapt_count(slice, lane)
    return _credo.serdes_get_readapt_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_rx_ready(slice, lane)
    return _credo.serdes_get_rx_ready(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_rx_signal_detect(slice, lane)
    return _credo.serdes_get_rx_signal_detect(slice, lane) ~= 0
end

---@param slice integer
function credo.serdes_init_top_pll(slice)
    _credo.serdes_init_top_pll(slice)
end

---comment
---@param slices any
---@param command any
---@return string
---@return string|nil
function credo.slash_command_catch(slices, command)
    if type(slices) == "number" then
        slices = {slices}
    end
    local err, data = _credo.slash_command_buffer(slices, command)
    return data, err
end

---@param slices integer[]|integer
---@param command string
---@return string
function credo.slash_command(slices, command)
    local output, error_str = credo.slash_command_catch(slices, command)
    if error_str then
        error(error_str)
    end
    return output
end

---@param slice integer
---@param command string
function credo.slice_display_info(slice, command)
    -- use print to help with data capture
    if _credo.display_info_implemented(slice) then
        local output = credo.display_info(slice, command)
        if stringx.strip(output) ~= "" then -- prevent non output commands from printing a new line
            print(output)
        end
    else
        _credo.slice_display_info(slice, command)
    end

end

---@param slice integer
---@return credo.DeviceType
function credo.slice_get_device_type(slice)
    local dev_type = _credo.slice_get_device_type(slice)
    return credo.DeviceType(dev_type)
end

---@class credo.SliceLimit
---@field public max_ports number
---@field public max_vsensors number
---@field public max_lanes number
---@overload fun():credo.SliceLimit
credo.SliceLimit = _credo.SliceLimit

---@param slice integer
---@return credo.SliceLimit
function credo.slice_get_limits(slice)
    local limits = credo.SliceLimit()
    _credo.slice_get_limits(slice, limits)
    return limits
end

---@param slice integer
---@return integer
function credo.slice_get_model_number(slice)
    return _credo.slice_get_model_number(slice)
end

---@param slice integer
---@param option_name string
---@return integer
function credo.slice_get_option(slice, option_name)
    return _credo.slice_get_option(slice, option_name)
end

---@class credo.SliceOption
---@field public name string
---@field public description string
---@overload fun():credo.SliceOption
credo.SliceOption = _credo.SliceOption

---@param slice integer
---@return credo.SliceOption[]
function credo.slice_get_option_list(slice)
    ---@type credo.LaneOption[]
    local option_list = {}
    local count = _credo.slice_get_option_count(slice)
    for index = 0, count - 1 do
        local option = credo.SliceOption()
        ---@type boolean
        _credo.slice_index_option_list(slice, index, option)
        list.append(option_list, option)
    end
    return option_list
end

---@param slice integer
---@return integer
function credo.slice_get_oui(slice)
    return _credo.slice_get_oui(slice)
end

---@param slice integer
---@return integer
function credo.slice_get_revision_number(slice)
    return _credo.slice_get_revision_number(slice)
end

---@param slice integer
---@return number
function credo.slice_get_temperature(slice)
    return _credo.slice_get_temperature(slice)
end

---@param slice integer
---@return credo.SliceType
function credo.slice_get_type(slice)
    local slice_type = _credo.slice_get_type(slice)
    return credo.SliceType(slice_type)
end

---@param slice integer
---@return credo.Family
function credo.slice_get_family(slice)
    local slice_type = _credo.slice_get_family(slice)
    return credo.Family(slice_type)
end

---@param slice integer
---@return number
function credo.slice_get_vsensor(slice)
    return _credo.slice_get_vsensor(slice)
end

---@param slice integer
---@param sel_vin integer
---@return number
function credo.slice_get_vsensor_ex(slice, sel_vin)
    return _credo.slice_get_vsensor_ex(slice, sel_vin)
end

---@param slice integer
---@param option string
---@return boolean
function credo.slice_is_option_supported(slice, option)
    local status = pcall(_credo.slice_is_option_supported, slice, option)
    if not status then
        return false
    else
        return true
    end
end

---@param slice integer
---@param file_path string
function credo.slice_load_setup(slice, file_path)
    _credo.slice_load_setup(slice, file_path)
end

---@param slice integer
function credo.slice_logic_reset(slice)
    _credo.slice_logic_reset(slice)
end

---@param slice integer
function credo.slice_mcu_reset(slice)
    _credo.slice_mcu_reset(slice)
end

---@param slice integer
function credo.slice_mcu_reset_hold(slice)
    _credo.slice_mcu_reset_hold(slice)
end

---@param slice integer
---@param address integer
---@param msb? integer
---@param lsb? integer
---@return integer
function credo.slice_read(slice, address, msb, lsb)
    lsb = lsb or msb or 0
    msb = msb or 15

    -- allow lsb and msb to be in the wrong order
    if lsb > msb then
        msb, lsb = lsb, msb
    end

    local val = _credo.slice_read(slice, address)
    return (val >> lsb) & (((1 << (msb + 1)) - 1) >> lsb)
end

---read slice and print as hex string
---@param slice integer
---@param address integer
---@param msb? integer
---@param lsb? integer
---@return string hex of register
function credo.slice_readh(slice, address, msb, lsb)
    return hex(credo.slice_read(slice, address, msb, lsb))
end

---@param slice integer
---@param start_address integer
---@param count integer
---@return integer[]
function credo.slice_burst_read(slice, start_address, count)
    local vals = range(count)
    return _credo.slice_burst_read(slice, start_address, vals)
end

---@param slice integer
function credo.slice_reg_reset(slice)
    _credo.slice_reg_reset(slice)
end

---@param slice integer
---@param init credo.SliceInitType
---@param firmware? string
function credo.slice_reinit(slice, init, firmware)
    firmware = firmware or ""
    _credo.slice_reinit(slice, init.value, firmware)
end

---@param slice integer
---@param file_path string
function credo.slice_save_setup(slice, file_path)
    _credo.slice_save_setup(slice, file_path)
end

---@param slice integer
---@param option_name string
---@param value integer
function credo.slice_set_option(slice, option_name, value)
    _credo.slice_set_option(slice, option_name, value.value)
end

---@param slice integer
function credo.slice_soft_reset(slice)
    _credo.slice_soft_reset(slice)
end

---@param slice integer
---@param address integer
---@param value integer
---@param msb? integer
---@param lsb? integer
function credo.slice_write(slice, address, value, msb, lsb)
    lsb = lsb or msb or 0
    msb = msb or 15

    -- allow lsb and msb to be in the wrong order
    if lsb > msb then
        msb, lsb = lsb, msb
    end

    if msb == 15 and lsb == 0 then
        _credo.slice_write(slice, address, value)
    else
        local val = _credo.slice_read(slice, address)
        local mask = ((1 << (msb + 1)) - 1) - ((1 << lsb) - 1)
        val = (val & ~mask) | ((value << lsb) & mask)
        _credo.slice_write(slice, address, val)
    end

end

---@param slice integer
---@param start_address integer
---@param data integer[]
function credo.slice_burst_write(slice, start_address, data)
    _credo.slice_burst_write(slice, start_address, data)
end

---@param slice integer
---@param sram_status credo.SramStatus
function credo.sram_generate_error(slice, sram_status)
    _credo.sram_generate_error(slice, sram_status.value)
end

---@param slice integer
---@return credo.SramStatus
function credo.sram_get_status(slice)
    local status = _credo.sram_get_status(slice)
    return credo.SramStatus(status)
end

---@param slice integer
---@param addr integer
---@return integer
function credo.tcm_read(slice, addr)
    return _credo.tcm_read(slice, addr)
end

---@param slice integer
---@param addr integer
---@param data integer
function credo.tcm_write(slice, addr, data)
    _credo.tcm_write(slice, addr, data)
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.testpatt_get_tx_enable(slice, lane)
    return _credo.testpatt_get_tx_enable(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.testpatt_get_tx_memory(slice, lane)
    return _credo.testpatt_get_tx_memory(slice, lane)
end

---@param slice integer
---@param lane integer
---@return credo.TestPatternMode
function credo.testpatt_get_tx_mode(slice, lane)
    local mode = _credo.testpatt_get_tx_mode(slice, lane)
    return credo.TestPatternMode(mode)
end

---@param slice integer
---@param lane integer
---@param enable boolean
function credo.testpatt_set_tx_enable(slice, lane, enable)
    _credo.testpatt_set_tx_enable(slice, lane, enable)
end

---@param slice integer
---@param lane integer
---@param pattern integer
function credo.testpatt_set_tx_memory(slice, lane, pattern)
    _credo.testpatt_set_tx_memory(slice, lane, pattern)
end

---@param slice integer
---@param lane integer
---@param mode credo.TestPatternMode
function credo.testpatt_set_tx_mode(slice, lane, mode)
    _credo.testpatt_set_tx_mode(slice, lane, mode.value)
end

---@class credo.Param
---@field public has_setter boolean
---@field public description string
---@field public name string
---@field public type string
---@field public index_type credo.ParamIndex
---@field public has_getter boolean
---@field public val_type credo.ParamValue
---@field public count number
---@field public flags number
---@overload fun():credo.Param
credo.Param = _credo.Param

local Domain = {SLICE = 0, PORT = 1, LANE = 2, SERDES = 3}

---@param slice integer
---@param domain integer
local function get_param_deflist(slice, domain)
    local param_list = {}
    local param_count = _credo.param_get_param_count(slice, domain)
    for i = 1, param_count do
        local param = credo.Param()
        _credo.param_index_param_def(slice, domain, i - 1, param)
        param_list[i] = param
    end
    return list.sort(param_list, function(a, b)
        return a.name < b.name
    end)
end

---@param slice integer
---@param domain integer
---@param name string
---@return credo.Param|nil
local function find_param_def(slice, domain, name)
    local param = credo.Param()

    local found = _credo.param_find_param_def(slice, domain, name, param)

    if found then
        return param
    else
        return nil
    end
end

---@param slice integer
---@param domain integer
---@param name string
---@param index integer
---@return integer
local function param_get_param_val_count(slice, domain, name, index)
    return _credo.param_get_param_val_count(slice, domain, name, index)
end

---@param slice integer
---@param domain integer
---@param name string
---@param index integer
---@return integer
local function param_get_param_val_set_count(slice, domain, name, index)
    return _credo.param_get_param_val_set_count(slice, domain, name, index)
end

---@param slice integer
---@param domain integer
---@param param_name string
---@param index integer
---@return number|number[]|integer|integer[]
local function get_param(slice, domain, param_name, index)
    local param = find_param_def(slice, domain, param_name)
    assert(param ~= nil, "Invalid parameter '%s'" % {param_name})
    local data = _credo.param_get_paramh(slice, domain, param_name, index)
    if param.count == 1 then
        return data[1]
    end
    return data
end

---@param slice integer
---@param domain integer
---@param param_name string
---@param index integer
---@param val number|number[]|integer|integer[]
local function set_param(slice, domain, param_name, index, val)
    local param = find_param_def(slice, domain, param_name)
    assert(param ~= nil, "Invalid parameter %s" % {param_name})
    if type(val) ~= "table" then
        val = {val}
    end
    local data = {type = param.val_type, data = val}
    _credo.param_set_param(slice, domain, param_name, index, data)
end

---@param slice integer
---@param param_name string
---@return credo.Param|nil
function credo.serdes_find_param_def(slice, param_name)
    return find_param_def(slice, Domain.SERDES, param_name)
end

---@param slice integer
---@param param_name string
---@param index integer
---@return integer
function credo.serdes_get_param_val_count(slice, param_name, index)
    local count = param_get_param_val_count(slice, Domain.SERDES, param_name, index)
    local set_count = param_get_param_val_set_count(slice, Domain.SERDES, param_name, index)
    if count > set_count then
        return count
    else
        return set_count
    end
end

---@param slice integer
---@param index integer
---@return credo.Param
function credo.serdes_index_param_def(slice, index)
    local param = credo.Param()
    _credo.serdes_index_param_def(slice, index, param)
    return param
end

---@param slice integer
---@param param_name string
---@param index integer
---@return any
function credo.serdes_get_param(slice, param_name, index)
    return get_param(slice, Domain.SERDES, param_name, index)
end

---@param slice integer
---@param param_name string
---@param index integer
---@param val number|number[]|integer|integer[]
function credo.serdes_set_param(slice, param_name, index, val)
    set_param(slice, Domain.SERDES, param_name, index, val)
end

---@param slice integer
---@return credo.Param[]
function credo.serdes_get_param_deflist(slice)
    return get_param_deflist(slice, Domain.SERDES)
end

---@param slice integer
---@return credo.Param[]
function credo.port_get_param_deflist(slice)
    return get_param_deflist(slice, Domain.PORT)
end

---@param slice integer
---@param param_name string
---@param index integer
---@return number|number[]|integer|integer[]
function credo.port_get_param(slice, param_name, index)
    return get_param(slice, Domain.PORT, param_name, index)
end

---@param slice integer
---@param param_name string
---@param index integer
---@param val number|number[]|integer|integer[]
function credo.port_set_param(slice, param_name, index, val)
    return set_param(slice, Domain.PORT, param_name, index, val)
end

---@param slice integer
---@return credo.Param[]
function credo.lane_get_param_deflist(slice)
    return get_param_deflist(slice, Domain.LANE)
end

---@param slice integer
---@param param_name string
---@param index integer
---@return number|number[]|integer|integer[]
function credo.lane_get_param(slice, param_name, index)
    return get_param(slice, Domain.LANE, param_name, index)
end

---@param slice integer
---@param param_name string
---@param index integer
---@param val number|number[]|integer|integer[]
function credo.lane_set_param(slice, param_name, index, val)
    return set_param(slice, Domain.LANE, param_name, index, val)
end

---@param slice integer
---@return credo.Param[]
function credo.slice_get_param_deflist(slice)
    return get_param_deflist(slice, Domain.SLICE)
end

---@param slice integer
---@param param_name string
---@param index integer
---@return number|number[]|integer|integer[]
function credo.slice_get_param(slice, param_name, index)
    return get_param(slice, Domain.SLICE, param_name, index)
end

---@param slice integer
---@param param_name string
---@param index integer
---@param val number|number[]|integer|integer[]
function credo.slice_set_param(slice, param_name, index, val)
    return set_param(slice, Domain.SLICE, param_name, index, val)
end

---@param slice integer
---@return lightuserdata
function credo.slice_get_device(slice)
    return _credo.slice_get_device(slice)
end

---@param slice integer
---@param lane integer
---@param taps integer[]
function credo.serdes_set_tx_taps(slice, lane, taps)
    local array_size = _credo.serdes_get_tx_ffe_range(slice, lane)
    assert(array_size == #taps, "array size is %d, expected %d" % {#taps, array_size})
    _credo.serdes_set_tx_taps(slice, lane, taps)
end

---@class credo.ChannelDesc
---@field public mode credo.LaneMode
---@field public speed integer
---@field public optical boolean
---@field public opt_vswing integer
---@field public channel_loss integer
---@overload fun():credo.ChannelDesc
credo.ChannelDesc = _credo.ChannelDesc

---@param slice integer
---@param lane integer
---@param desc any
function credo.serdes_preset_tx_taps(slice, lane, desc)
    _credo.serdes_preset_tx_taps(slice, lane, desc)
end

---@param slice integer
---@param lane integer
function credo.serdes_reset_tx_taps(slice, lane)
    _credo.cri_serdes_reset_tx_taps(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer base
---@return integer extended
function credo.serdes_get_tx_ffe_range(slice, lane)
    return _credo.serdes_get_tx_ffe_range(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_tx_taps(slice, lane)
    local array_size = credo.serdes_get_tx_ffe_range(slice, lane)
    return _credo.serdes_get_tx_taps(slice, lane, array_size)
end

---@param slice integer
---@param lane integer
---@param taps integer[]
function credo.serdes_set_tx_taps_scale(slice, lane, taps)
    local array_size = _credo.serdes_get_tx_ffe_range(slice, lane)
    assert(array_size == #taps, "array size is %d, expected %d" % {#taps, array_size})
    _credo.serdes_set_tx_taps_scale(slice, lane, taps)
end

function credo.serdes_get_tx_taps_scale(slice, lane)
    local array_size = _credo.serdes_get_tx_ffe_range(slice, lane)
    return _credo.serdes_get_tx_taps_scale(slice, lane, array_size)
end

---@param slice integer
---@param lane integer
---@param pol boolean
function credo.serdes_set_rx_polarity(slice, lane, pol)
    _credo.serdes_set_rx_polarity(slice, lane, int(pol))
end

---@param slice integer
---@param lane integer
---@param pol boolean
function credo.serdes_set_tx_polarity(slice, lane, pol)
    _credo.serdes_set_tx_polarity(slice, lane, int(pol))
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_rx_polarity(slice, lane)
    return _credo.serdes_get_rx_polarity(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_tx_polarity(slice, lane)
    return _credo.serdes_get_tx_polarity(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@param coupling credo.LaneCoupling
function credo.serdes_set_rx_coupling(slice, lane, coupling)
    _credo.serdes_set_rx_coupling(slice, lane, coupling.value)
end

---@param slice integer
---@param lane integer
---@return credo.LaneCoupling
function credo.serdes_get_rx_coupling(slice, lane)
    local value = _credo.serdes_get_rx_coupling(slice, lane)
    return credo.LaneCoupling(value)
end

---@param slice integer
---@param lane integer
---@param gc boolean
function credo.serdes_set_rx_gray_code(slice, lane, gc)
    _credo.serdes_set_rx_gray_code(slice, lane, int(gc))
end

---@param slice integer
---@param lane integer
---@param gc boolean
function credo.serdes_set_tx_gray_code(slice, lane, gc)
    _credo.serdes_set_tx_gray_code(slice, lane, int(gc))
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_rx_gray_code(slice, lane)
    return _credo.serdes_get_rx_gray_code(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_tx_gray_code(slice, lane)
    return _credo.serdes_get_tx_gray_code(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@param pc boolean
function credo.serdes_set_rx_precoder(slice, lane, pc)
    _credo.serdes_set_rx_precoder(slice, lane, int(pc))
end

---@param slice integer
---@param lane integer
---@param pc boolean
function credo.serdes_set_tx_precoder(slice, lane, pc)
    _credo.serdes_set_tx_precoder(slice, lane, int(pc))
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_rx_precoder(slice, lane)
    return _credo.serdes_get_rx_precoder(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_tx_precoder(slice, lane)
    return _credo.serdes_get_tx_precoder(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@param msb boolean
function credo.serdes_set_rx_msb(slice, lane, msb)
    _credo.serdes_set_rx_msb(slice, lane, int(msb))
end

---@param slice integer
---@param lane integer
---@param msb boolean
function credo.serdes_set_tx_msb(slice, lane, msb)
    _credo.serdes_set_tx_msb(slice, lane, int(msb))
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_rx_msb(slice, lane)
    return _credo.serdes_get_rx_msb(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@return boolean
function credo.serdes_get_tx_msb(slice, lane)
    return _credo.serdes_get_tx_msb(slice, lane) ~= 0
end

---@param slice integer
---@param lane integer
---@param ber_exp integer
---@param flag credo.EyeMonitorFlags
function credo.eye_monitor_start(slice, lane, ber_exp, flag)
    _credo.eye_monitor_start(slice, lane, ber_exp, flag.value)
end

---@param slice integer
---@param lane integer
function credo.eye_monitor_stop(slice, lane)
    _credo.eye_monitor_stop(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer progress
function credo.eye_monitor_get_progress(slice, lane)
    return _credo.eye_monitor_get_progress(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[][] data
---@return integer extent_mv
function credo.eye_monitor_get_data(slice, lane)
    return _credo.eye_monitor_get_data(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer vstep_side
---@return integer hstep_side
function credo.eye_monitor_get_range(slice, lane)
    return _credo.eye_monitor_get_range(slice, lane)
end

---@param slice integer
---@return integer[]
function credo.eye_monitor_get_separator(slice)
    return _credo.eye_monitor_get_separator(slice)
end

---@class credo.Option
---@field public name string
---@field public description string
---@overload fun():credo.Option
credo.Option = _credo.Option

---@param slice integer
---@param port_id integer
---@param option string
---@return integer
function credo.port_get_option(slice, port_id, option)
    return _credo.port_get_option(slice, port_id.value, option)
end

---@param slice integer
---@param port_id integer
---@param option string
---@param value integer
function credo.port_set_option(slice, port_id, option, value)
    _credo.port_set_option(slice, port_id.value, option, value.value)
end

---@param slice integer
---@param index integer
---@return credo.Option
function credo.port_index_option_def(slice, index)
    local option = credo.Option()
    _credo.port_index_option_def(slice, index, option)
    return option
end

---@param slice integer
---@return credo.Option[]
function credo.port_get_option_list(slice)
    local count = _credo.port_get_option_count(slice);
    local options = {}
    for i = 0, count - 1 do
        list.append(options, credo.port_index_option_def(slice, i))
    end
    return options
end

---@param slice integer
---@return integer
function credo.serdes_get_top_pll_cap(slice)
    return _credo.serdes_get_top_pll_cap(slice)
end

---@param slice integer
---@param lane integer
---@return integer taps_len
---@return integer sum_len
function credo.serdes_get_rx_ffe_range(slice, lane)
    return _credo.serdes_get_rx_ffe_range(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer row
---@return integer col
function credo.serdes_get_rx_ffe_weighting_table_range(slice, lane)
    return _credo.serdes_get_rx_ffe_weighting_table_range(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_rx_dfe_range(slice, lane)
    return _credo.serdes_get_rx_dfe_range(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_rx_isi_range(slice, lane)
    return _credo.serdes_get_rx_isi_range(slice, lane)
end

---@param slice integer
---@param lane integer
---@param tx_cap integer
function credo.serdes_set_tx_cap(slice, lane, tx_cap)
    _credo.serdes_set_tx_cap(slice, lane, tx_cap)
end

---@param slice integer
---@param lane integer
---@param rx_cap integer
function credo.serdes_set_rx_cap(slice, lane, rx_cap)
    _credo.serdes_set_rx_cap(slice, lane, rx_cap)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_tx_cap(slice, lane)
    return _credo.serdes_get_tx_cap(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_rx_cap(slice, lane)
    return _credo.serdes_get_rx_cap(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_rx_ppm(slice, lane)
    -- CredoErrorCodes_t cr_serdes_get_rx_ppm(CredoSlice_t* slice, int lane, int* OUTPUT);
    return _credo.serdes_get_rx_ppm(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer enable
---@return integer degen
---@return integer addcap
---@return integer gain
function credo.serdes_get_rx_skef(slice, lane)
    return _credo.serdes_get_rx_skef(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_rx_dac(slice, lane)
    return _credo.serdes_get_rx_dac(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer passive
---@return integer gain
---@return integer termtune
function credo.serdes_get_rx_attenuator(slice, lane)
    return _credo.serdes_get_rx_attenuator(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_ffe_taps(slice, lane)
    local taps_len, sum_len = credo.serdes_get_rx_ffe_range(slice, lane)
    return _credo.serdes_get_ffe_taps(slice, lane, taps_len + sum_len)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_ffe_taps_fine(slice, lane)
    local taps_len, sum_len = credo.serdes_get_rx_ffe_range(slice, lane)
    -- CredoErrorCodes_t cr_serdes_get_ffe_taps_fine(CredoSlice_t* slice, int lane, int GET_FFE_TAPS_FINE[]);
    return _credo.serdes_get_ffe_taps_fine(slice, lane, taps_len + sum_len)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_f1over3(slice, lane)
    return _credo.serdes_get_f1over3(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_agcgain_count(slice, lane)
    return _credo.serdes_get_agcgain_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_agcgain(slice, lane)
    local count = credo.serdes_get_agcgain_count(slice, lane)
    return _credo.serdes_get_agcgain(slice, lane, count)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_ctle_count(slice, lane)
    return _credo.serdes_get_ctle_count(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_ctle(slice, lane)
    local count = credo.serdes_get_ctle_count(slice, lane)
    return _credo.serdes_get_ctle(slice, lane, count)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_delta_phase(slice, lane)
    return _credo.serdes_get_delta_phase(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_edge(slice, lane)
    return _credo.serdes_get_edge(slice, lane)
end

---@param slice integer
---@param lane integer
---@return number[]
function credo.serdes_get_dfe(slice, lane)
    local count = credo.serdes_get_rx_dfe_range(slice, lane)
    return _credo.serdes_get_dfe(slice, lane, count)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_raw_eye(slice, lane)
    return _credo.serdes_get_raw_eye(slice, lane)
end

---@param slice integer
---@param lane integer
---@param enable integer
---@param degen integer
---@param addcap integer
---@param gain integer
function credo.serdes_set_rx_skef(slice, lane, enable, degen, addcap, gain)
    _credo.serdes_set_rx_skef(slice, lane, enable, degen, addcap, gain)
end

---@param slice integer
---@param lane integer
---@param rx_dac integer
function credo.serdes_set_rx_dac(slice, lane, rx_dac)
    _credo.serdes_set_rx_dac(slice, lane, rx_dac)
end

---@param slice integer
---@param lane integer
---@param passive integer
---@param gain integer
---@param termtune integer
function credo.serdes_set_rx_attenuator(slice, lane, passive, gain, termtune)
    _credo.serdes_set_rx_attenuator(slice, lane, passive, gain, termtune)
end

---@param slice integer
---@param lane integer
---@param ffe_taps integer[]
function credo.serdes_set_ffe_taps(slice, lane, ffe_taps)
    local taps_len, sum_len = credo.serdes_get_rx_ffe_range(slice, lane)
    assert(#ffe_taps == taps_len + sum_len, "Expected list of length %d, got %d" % {taps_len + sum_len, #ffe_taps})
    _credo.serdes_set_ffe_taps(slice, lane, ffe_taps)
end

---@param slice integer
---@param lane integer
---@param ffe_taps_fine integer[]
function credo.serdes_set_ffe_taps_fine(slice, lane, ffe_taps_fine)
    local taps_len, sum_len = credo.serdes_get_rx_ffe_range(slice, lane)
    assert(#ffe_taps_fine == taps_len + sum_len,
           "Expected list of length %d, got %d" % {taps_len + sum_len, #ffe_taps_fine})
    -- CredoErrorCodes_t cr_serdes_set_ffe_taps_fine(CredoSlice_t* slice, int lane, const int SET_FFE_TAPS[]);
    _credo.serdes_set_ffe_taps_fine(slice, lane, ffe_taps_fine)
end

---@param slice integer
---@param lane integer
---@param f1over3 integer
function credo.serdes_set_f1over3(slice, lane, f1over3)
    _credo.serdes_set_f1over3(slice, lane, f1over3)
end

---@param slice integer
---@param lane integer
---@param agcgain integer[]
function credo.serdes_set_agcgain(slice, lane, agcgain)
    local count = credo.serdes_get_agcgain_count(slice, lane)
    assert(#agcgain == count, "Expected list of length %d, got %d" % {count, #agcgain})
    _credo.serdes_set_agcgain(slice, lane, agcgain)
end

---@param slice integer
---@param lane integer
---@param ctle integer[]
function credo.serdes_set_ctle(slice, lane, ctle)
    local count = credo.serdes_get_ctle_count(slice, lane)
    assert(#ctle == count, "Expected list of length %d, got %d" % {count, #ctle})
    _credo.serdes_set_ctle(slice, lane, ctle)
end

---@param slice integer
---@param lane integer
---@param phase integer
function credo.serdes_set_delta_phase(slice, lane, phase)
    _credo.serdes_set_delta_phase(slice, lane, phase)
end

---@param slice integer
---@param lane integer
---@param edge integer
function credo.serdes_set_edge(slice, lane, edge)
    _credo.serdes_set_edge(slice, lane, edge)
end

---@param slice integer
---@param lane integer
---@param taps integer[]
function credo.serdes_set_tx_taps_extended(slice, lane, taps)
    local _, extended = credo.serdes_get_tx_ffe_range(slice, lane)
    assert(#taps == extended, "Expected list of length %d, got %d" % {extended, #taps})
    _credo.serdes_set_tx_taps_extended(slice, lane, taps)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_tx_taps_extended(slice, lane)
    local _, extended = credo.serdes_get_tx_ffe_range(slice, lane)
    return _credo.serdes_get_tx_taps_extended(slice, lane, extended)
end

---@param slice integer
---@param lane integer
---@return number
function credo.serdes_get_channel_estimate(slice, lane)
    return _credo.serdes_get_channel_estimate(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_of(slice, lane)
    return _credo.serdes_get_of(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer
function credo.serdes_get_hf(slice, lane)
    return _credo.serdes_get_hf(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_eye(slice, lane)
    return _credo.serdes_get_eye(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_isi(slice, lane)
    local count = credo.serdes_get_rx_isi_range(slice, lane)
    return _credo.serdes_get_isi(slice, lane, count)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_rx_ffe(slice, lane)
    local taps_len, sum_len = credo.serdes_get_rx_ffe_range(slice, lane)
    return _credo.serdes_get_rx_ffe(slice, lane, taps_len + sum_len)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_rx_ffe_nbias(slice, lane)
    local taps_len, sum_len = credo.serdes_get_rx_ffe_range(slice, lane)
    return _credo.serdes_get_rx_ffe_nbias(slice, lane, taps_len + sum_len)
end

---@param slice integer
---@param lane integer
---@return number[]
function credo.serdes_get_rx_ffe_kaccu(slice, lane)
    local taps_len = credo.serdes_get_rx_ffe_range(slice, lane)
    return _credo.serdes_get_rx_ffe_kaccu(slice, lane, taps_len)
end

---@param slice integer
---@param lane integer
---@return number[][]
function credo.serdes_get_rx_ffe_weighting_table(slice, lane)
    return _credo.serdes_get_rx_ffe_weighting_table(slice, lane)
end

---@param slice integer
---@param lane integer
---@return integer[]
function credo.serdes_get_rx_ffe_flip_counter(slice, lane)
    local taps_len = credo.serdes_get_rx_ffe_range(slice, lane)
    return _credo.serdes_get_rx_ffe_flip_counter(slice, lane, taps_len)
end

---@param slice integer
---@param port_id integer
---@param setup credo.PortSetup
function credo.port_build(slice, port_id, setup)
    _credo.port_build(slice, port_id.value, setup)
end

---@param slice integer
---@param port_id integer
---@param force? boolean
function credo.port_start(slice, port_id, force)
    if force == nil then
        force = false
    end
    _credo.port_start(slice, port_id.value, force)
end

---@param slice integer
---@param port_id integer
---@return boolean started
---@return credo.PortSetup|nil setup
function credo.port_get_setup(slice, port_id)
    local setup = credo.PortSetup()
    local started = _credo.port_get_setup(slice, port_id.value, setup)
    if not started then
        return false
    end
    return true, setup
end

---@param slice integer
---@return integer
function credo.port_assign_id(slice)
    return _credo.port_assign_id(slice)
end

--- End of Functions ---

---@param slice integer
---@param lane integer
---@param mode credo.LaneMode
---@param speed integer
---@param flags? integer
function credo.phy_configure(slice, lane, mode, speed, flags)

    if speed >= 1e9 then
        if flags & credo.LFLAG_FLEXSPEED ~= 0 then
            speed = speed / 1e3
        else
            speed = speed / 1e6
        end
    end
    if flags == nil then
        flags = 0
    end
    speed = int(speed)
    print(speed, flags)
    _credo.phy_configure(slice, lane, mode.value, speed.value, flags.value)
end

---@param slice integer
---@param lane integer
---@param mode credo.LaneMode
---@param speed integer
---@param flags? integer
function credo.phy_configure_shallow_retimer(slice, lane, mode, speed, flags)
    if flags == nil then
        flags = 0
    end
    _credo.phy_configure_shallow_retimer(slice, lane, mode.value, speed.value, flags.value)
end

---@param slice integer
---@param lane integer
function credo.phy_destroy(slice, lane)
    _credo.phy_destroy(slice, lane)
end

---@param slice integer
---@param lane integer
---@return credo.LinkTrainingStatus
function credo.link_training_get_status(slice, lane)
    return credo.LinkTrainingStatus(_credo.link_training_get_status(slice, lane))
end

---@param slice integer
---@param lane integer
---@return credo.LinkTrainingState
function credo.link_training_get_state(slice, lane)
    return credo.LinkTrainingState(_credo.link_training_get_state(slice, lane))
end

---@param slice integer
---@param lane integer
---@return integer
function credo.link_training_get_restart_count(slice, lane)
    return _credo.link_training_get_restart_count(slice, lane)
end

---@param slice integer
---@param partition integer
function credo.spiflash_load_firmware(slice, partition)
    _credo.spiflash_load_firmware(slice, partition)
end

---@param slice integer
function credo.spiflash_display_mbr(slice)
    _credo.spiflash_display_mbr(slice)
end

---@param slice integer
---@param flash_size_kb integer
---@param force? boolean
function credo.spiflash_format_mbr(slice, flash_size_kb, force)
    if force == nil then
        force = false
    end
    _credo.spiflash_format_mbr(slice, flash_size_kb, force)
end

---@param slice integer
---@param firmware_out_path string
---@param partition_num integer
function credo.spiflash_read_firmware(slice, firmware_out_path, partition_num)
    _credo.spiflash_read_firmware(slice, firmware_out_path, partition_num)
end

---@param slice integer
---@param firmware_out_path string
---@param partition_num integer
---@param force? boolean
function credo.spiflash_write_firmware(slice, firmware_out_path, partition_num, force)
    if force == nil then
        force = false
    end
    _credo.spiflash_write_firmware(slice, firmware_out_path, partition_num, force)
end

---@param slice integer
---@param partition integer
function credo.spiflash_set_partition(slice, partition)
    _credo.spiflash_set_partition(slice, partition)
end

---@param timeout_usec integer
function credo.lock_set_timeout(timeout_usec)
    _credo.lock_set_timeout(timeout_usec)
end

---@return integer
function credo.lock_get_timeout()
    return _credo.lock_get_timeout()
end

---@param slice integer
---@return number
function credo.time_start(slice)
    return _credo.time_start(slice)
end

---@param slice integer
---@return number
function credo.time_system(slice)
    return _credo.time_system(slice)
end

---@class credo.DeviceType: enum.IntEnum
---@class credo.DeviceTypeEnum : enum.IntEnumMeta
---@field public HERON_P3 credo.DeviceType
---@field public OWL_400 credo.DeviceType
---@field public HERON_P1 credo.DeviceType
---@field public BLACKHAWK_800 credo.DeviceType
---@field public OSPREY_400 credo.DeviceType
---@field public FAKE_800 credo.DeviceType
---@field public BALDEAGLE_400 credo.DeviceType
---@field public OWL_800 credo.DeviceType
---@field public BLACKHAWK_400 credo.DeviceType
---@field public KITE_TEST credo.DeviceType
---@field public OSPREY_AEC credo.DeviceType
---@field public OSPREY_800 credo.DeviceType
---@field public HERON_1P0 credo.DeviceType
---@field public HERON_MR credo.DeviceType
---@field public FAKE_400 credo.DeviceType
---@field public FAKE_32 credo.DeviceType
---@field public ADMIRAL_TEST credo.DeviceType
---@field public BALDEAGLE_800 credo.DeviceType
---@field public NUTCRACKER_32 credo.DeviceType
---@field public SEAHAWK credo.DeviceType
---@field public OSTRICH_1P1 credo.DeviceType
---@field public SCREAMING_EAGLE credo.DeviceType
---@field public SCREAMING_EAGLE_AEC credo.DeviceType
---@field public VICEROY_IN_NC credo.DeviceType
---@field public VICEROY_TEST credo.DeviceType
---@field public RAVEN_TEST credo.DeviceType
---@field public NIGHTHAWK_1P0 credo.DeviceType
---@field public NIGHTHAWK credo.DeviceType
---@field public SKIPPER_TEST credo.DeviceType
---@field public SKIPPER credo.DeviceType
---@field public SPARROW_800 credo.DeviceType
---@field public BLUEJAY credo.DeviceType
---@field public MONARCH2P1_TEST credo.DeviceType
---@field public MONARCH2P1 credo.DeviceType

---@type credo.DeviceTypeEnum
credo.DeviceType = enum.IntEnum("DeviceType", {
    NUTCRACKER_32 = _credo.DEV_NUTCRACKER_32,
    OSTRICH_1P1 = _credo.DEV_OSTRICH_1P1,
    VICEROY_IN_NC = _credo.DEV_VICEROY_IN_NC,
    HERON_MR = _credo.DEV_HERON_MR,
    OWL_400 = _credo.DEV_OWL_400,
    OSPREY_400 = _credo.DEV_OSPREY_400,
    FAKE_400 = _credo.DEV_FAKE_400,
    FAKE_32 = _credo.DEV_FAKE_32,
    KITE_TEST = _credo.DEV_KITE_TEST,
    BLACKHAWK_800 = _credo.DEV_BLACKHAWK_800,
    BALDEAGLE_800 = _credo.DEV_BALDEAGLE_800,
    ADMIRAL_TEST = _credo.DEV_ADMIRAL_TEST,
    OWL_800 = _credo.DEV_OWL_800,
    HERON_1P0 = _credo.DEV_HERON_1P0,
    OSPREY_AEC = _credo.DEV_OSPREY_AEC,
    HERON_P3 = _credo.DEV_HERON_P3,
    FAKE_800 = _credo.DEV_FAKE_800,
    SEAHAWK = _credo.DEV_SEAHAWK,
    SCREAMING_EAGLE = _credo.DEV_SCREAMING_EAGLE,
    SCREAMING_EAGLE_AEC = _credo.DEV_SCREAMING_EAGLE_AEC,
    BALDEAGLE_400 = _credo.DEV_BALDEAGLE_400,
    VICEROY_TEST = _credo.DEV_VICEROY_TEST,
    HERON_P1 = _credo.DEV_HERON_P1,
    OSPREY_800 = _credo.DEV_OSPREY_800,
    BLACKHAWK_400 = _credo.DEV_BLACKHAWK_400,
    RAVEN_TEST = _credo.DEV_RAVEN_TEST,
    NIGHTHAWK_1P0 = _credo.DEV_NIGHTHAWK_1P0,
    NIGHTHAWK = _credo.DEV_NIGHTHAWK,
    PEREGRINE_EVB = _credo.DEV_PEREGRINE_EVB,
    PEREGRINE = _credo.DEV_PEREGRINE,
    CROW = _credo.DEV_CROW,
    SKIPPER_TEST = _credo.DEV_SKIPPER_TEST,
    SKIPPER = _credo.DEV_SKIPPER,
    SPARROW_800 = _credo.DEV_SPARROW_800,
    BLUEJAY = _credo.DEV_BLUEJAY,
    MONARCH2P1_TEST = _credo.DEV_MONARCH2P1_TEST,
    MONARCH2P1 = _credo.DEV_MONARCH2P1
})
---@class credo.SliceType: enum.IntEnum
---@class credo.SliceTypeEnum : enum.IntEnumMeta
---@field public BLACKHAWK_DC credo.SliceType
---@field public OSPREY credo.SliceType
---@field public OWL_A0 credo.SliceType
---@field public HERON credo.SliceType
---@field public OWL credo.SliceType
---@field public OWL_B0 credo.SliceType
---@field public ADMIRAL credo.SliceType
---@field public OWL_B0B credo.SliceType
---@field public FAKE credo.SliceType
---@field public BLACKHAWK_AC credo.SliceType
---@field public BLACKHAWK credo.SliceType
---@field public BALDEAGLE credo.SliceType
---@field public NUTCRACKER credo.SliceType
---@field public OWL_B1 credo.SliceType
---@field public KITE credo.SliceType
---@field public SEAHAWK credo.SliceType
---@field public OSTRICH credo.SliceType
---@field public SCREAMING_EAGLE credo.SliceType
---@field public VICEROY_IN_NC credo.SliceType
---@field public VICEROY_TEST credo.SliceType
---@field public RAVEN credo.SliceType
---@field public NIGHTHAWK credo.SliceType
---@field public SKIPPER_TEST credo.SliceType
---@field public SPARROW credo.SliceType
---@field public BLUEJAY credo.SliceType
---@field public MONARCH2P1_TEST credo.SliceType

---@type credo.SliceTypeEnum
credo.SliceType = enum.IntEnum("SliceType", {
    SCREAMING_EAGLE = _credo.SLC_SCREAMING_EAGLE,
    OWL_A0 = _credo.SLC_OWL_A0,
    BALDEAGLE = _credo.SLC_BALDEAGLE,
    BLACKHAWK_AC = _credo.SLC_BLACKHAWK_AC,
    ADMIRAL = _credo.SLC_ADMIRAL,
    NUTCRACKER = _credo.SLC_NUTCRACKER,
    OWL_B0B = _credo.SLC_OWL_B0B,
    BLACKHAWK = _credo.SLC_BLACKHAWK,
    HERON = _credo.SLC_HERON,
    FAKE = _credo.SLC_FAKE,
    OWL_B0 = _credo.SLC_OWL_B0,
    OWL = _credo.SLC_OWL,
    SEAHAWK = _credo.SLC_SEAHAWK,
    VICEROY_TEST = _credo.SLC_VICEROY_TEST,
    VICEROY_IN_NC = _credo.SLC_VICEROY_IN_NC,
    KITE = _credo.SLC_KITE,
    OSTRICH = _credo.SLC_OSTRICH,
    OSPREY = _credo.SLC_OSPREY,
    OWL_B1 = _credo.SLC_OWL_B1,
    BLACKHAWK_DC = _credo.SLC_BLACKHAWK_DC,
    RAVEN = _credo.SLC_RAVEN,
    NIGHTHAWK = _credo.SLC_NIGHTHAWK,
    PEREGRINE_EVB = _credo.SLC_PEREGRINE_EVB,
    PEREGRINE = _credo.SLC_PEREGRINE,
    CROW = _credo.SLC_CROW,
    SKIPPER_TEST = _credo.SLC_SKIPPER_TEST,
    SPARROW = _credo.SLC_SPARROW,
    BLUEJAY = _credo.SLC_BLUEJAY,
    MONARCH2P1_TEST = _credo.SLC_MONARCH2P1_TEST
})

---@class credo.Family: enum.IntEnum
---@class credo.FamilyEnum : enum.IntEnumMeta
---@field public BALDEAGLE credo.Family
---@field public OWL credo.Family
---@field public BLACKHAWK credo.Family
---@field public HERON credo.Family
---@field public OSPREY credo.Family
---@field public NUTCRACKER credo.Family
---@field public KITE credo.Family
---@field public ADMIRAL credo.Family
---@field public SEAHAWK credo.Family
---@field public OSTRICH credo.Family
---@field public SCREAMING_EAGLE credo.Family
---@field public VICEROY credo.Family
---@field public NIGHTHAWK credo.Family
---@field public SPARROW credo.Family
---@field public BLUEJAY credo.Family
---@field public RAVEN credo.Family
---@field public FAKE credo.Family
---@field public MONARCH2P1_TEST credo.Family
---@field public SKIPPER_TEST credo.Family

---@type credo.FamilyEnum
credo.Family = enum.IntEnum("Family", {
    BALDEAGLE = _credo.FAMILY_BALDEAGLE,
    OWL = _credo.FAMILY_OWL,
    BLACKHAWK = _credo.FAMILY_BLACKHAWK,
    HERON = _credo.FAMILY_HERON,
    OSPREY = _credo.FAMILY_OSPREY,
    NUTCRACKER = _credo.FAMILY_NUTCRACKER,
    KITE = _credo.FAMILY_KITE,
    ADMIRAL = _credo.FAMILY_ADMIRAL,
    SEAHAWK = _credo.FAMILY_SEAHAWK,
    OSTRICH = _credo.FAMILY_OSTRICH,
    SCREAMING_EAGLE = _credo.FAMILY_SCREAMING_EAGLE,
    VICEROY = _credo.FAMILY_VICEROY,
    NIGHTHAWK = _credo.FAMILY_NIGHTHAWK,
    RAVEN = _credo.FAMILY_RAVEN,
    BLUEJAY = _credo.FAMILY_BLUEJAY,
    SPARROW = _credo.FAMILY_SPARROW,
    MONARCH2P1_TEST = _credo.FAMILY_MONARCH2P1_TEST,
    SKIPPER_TEST = _credo.FAMILY_SKIPPER_TEST,
    FAKE = _credo.FAMILY_FAKE
})

---@class credo.SliceInitType: enum.IntEnum
---@class credo.SliceInitTypeEnum : enum.IntEnumMeta
---@field public NONE credo.SliceInitType
---@field public WARM credo.SliceInitType
---@field public FULL credo.SliceInitType
---@field public SPIFLASH credo.SliceInitType
---@field public NO_FIRMWARE credo.SliceInitType
---@type credo.SliceInitTypeEnum
credo.SliceInitType = enum.IntEnum("SliceInitType", {
    WARM = _credo.INIT_WARM,
    FULL = _credo.INIT_FULL,
    NO_FIRMWARE = _credo.INIT_NO_FIRMWARE,
    NONE = _credo.INIT_NONE,
    SPIFLASH = _credo.INIT_SPIFLASH
})
---@class credo.FecErrorType: enum.IntEnum
---@class credo.FecErrorTypeEnum : enum.IntEnumMeta
---@field public SYMBOL credo.FecErrorType
---@field public BIT credo.FecErrorType
---@field public FRAME credo.FecErrorType
---@type credo.FecErrorTypeEnum
credo.FecErrorType = enum.IntEnum("FecErrorType", {
    BIT = _credo.FEC_ERROR_BIT,
    SYMBOL = _credo.FEC_ERROR_SYMBOL,
    FRAME = _credo.FEC_ERROR_FRAME
})
---@class credo.EyeMonitorFlags: enum.IntEnum
---@class credo.EyeMonitorFlagsEnum
---@field public BATHTUB credo.EyeMonitorFlags
---@field public DESTRUCTIVE credo.EyeMonitorFlags
---@field public PHASE_BASE credo.EyeMonitorFlags
---@type credo.EyeMonitorFlagsEnum
credo.EyeMonitorFlags = enum.IntEnum("EyeMonitorFlags", {
    BATHTUB = _credo.EYE_MONITOR_BATHTUB,
    DESTRUCTIVE = _credo.EYE_MONITOR_DESTRUCTIVE
}, true)
---@class credo.PortType: enum.IntEnum
---@class credo.PortTypeEnum : enum.IntEnumMeta
---@field public BITMUX credo.PortType
---@field public GEARBOX credo.PortType
---@field public RETIMER credo.PortType
---@field public SWITCHOVER_RETIMER credo.PortType
---@type credo.PortTypeEnum
credo.PortType = enum.IntEnum("PortType", {
    RETIMER = _credo.PORT_RETIMER,
    BITMUX = _credo.PORT_BITMUX,
    GEARBOX = _credo.PORT_GEARBOX,
    SWITCHOVER_RETIMER = _credo.PORT_SWITCHOVER_RETIMER
})
---@class credo.PortDirection: enum.IntEnum
---@class credo.PortDirectionEnum : enum.IntEnumMeta
---@field public BIDIRECTIONAL credo.PortDirection
---@field public INGRESS credo.PortDirection
---@field public EGRESS credo.PortDirection
---@field public HOST_TO_LINE credo.PortDirection
---@field public LINE_TO_HOST credo.PortDirection
---@type credo.PortDirectionEnum
credo.PortDirection = enum.IntEnum("PortDirection", {
    BIDIRECTIONAL = _credo.PORT_DIR_BIDIRECTIONAL,
    INGRESS = _credo.PORT_DIR_INGRESS,
    EGRESS = _credo.PORT_DIR_EGRESS,
    HOST_TO_LINE = _credo.PORT_DIR_HOST_TO_LINE,
    LINE_TO_HOST = _credo.PORT_DIR_LINE_TO_HOST
})
---@class credo.PortMode: enum.IntEnum
---@class credo.PortModeEnum : enum.IntEnumMeta
---@field public PCS credo.PortMode
---@field public MACSEC credo.PortMode
---@field public SERDES credo.PortMode
---@type credo.PortModeEnum
credo.PortMode = enum.IntEnum("PortMode",
                              {SERDES = _credo.PMODE_SERDES, MACSEC = _credo.PMODE_MACSEC, PCS = _credo.PMODE_PCS})
---@class credo.LaneCoupling: enum.IntEnum
---@class credo.LaneCouplingEnum : enum.IntEnumMeta
---@field public AC credo.LaneCoupling
---@field public DC credo.LaneCoupling
---@type credo.LaneCouplingEnum
credo.LaneCoupling = enum.IntEnum("LaneCoupling", {AC = _credo.COUPLING_AC, DC = _credo.COUPLING_DC})
---@class credo.LaneMode: enum.IntEnum
---@class credo.LaneModeEnum : enum.IntEnumMeta
---@field public OFF credo.LaneMode
---@field public NRZ credo.LaneMode
---@field public DISABLE credo.LaneMode
---@field public PAM4 credo.LaneMode
---@field public AN credo.LaneMode
---@field public PAM3 credo.LaneMode
---@type credo.LaneModeEnum
credo.LaneMode = enum.IntEnum("LaneMode", {
    PAM3 = _credo.LMODE_PAM3,
    DISABLE = _credo.LMODE_DISABLE,
    OFF = _credo.LMODE_OFF,
    PAM4 = _credo.LMODE_PAM4,
    NRZ = _credo.LMODE_NRZ,
    AN = _credo.LMODE_AN
})
---@class credo.FecType: enum.IntEnum
---@class credo.FecTypeEnum : enum.IntEnumMeta
---@field public RS_544 credo.FecType
---@field public RS_528 credo.FecType
---@field public NONE credo.FecType
---@field public FIRE_CODE credo.FecType
---@type credo.FecTypeEnum
credo.FecType = enum.IntEnum("FecType", {
    RS_528 = _credo.FEC_RS_528,
    FIRE_CODE = _credo.FEC_FIRE_CODE,
    RS_544 = _credo.FEC_RS_544,
    NONE = _credo.FEC_NONE
})
---@class credo.SramStatus: enum.IntEnum
---@class credo.SramStatusEnum : enum.IntEnumMeta
---@field public CORR_ERROR credo.SramStatus
---@field public UNCORR_ERROR credo.SramStatus
---@field public NO_ERROR credo.SramStatus
---@type credo.SramStatusEnum
credo.SramStatus = enum.IntEnum("SramStatus", {
    UNCORR_ERROR = _credo.SRAM_UNCORR_ERROR,
    CORR_ERROR = _credo.SRAM_CORR_ERROR,
    NO_ERROR = _credo.SRAM_NO_ERROR
})
---@class credo.Side: enum.IntEnum
---@class credo.SideEnum : enum.IntEnumMeta
---@field public HOST credo.Side
---@field public SYSTEM credo.Side
---@field public LINE credo.Side
---@type credo.SideEnum
credo.Side = enum.IntEnum("Side", {HOST = _credo.SIDE_HOST, LINE = _credo.SIDE_LINE})
credo.Side.SYSTEM = credo.Side.HOST -- alias of HOST
---@class credo.MACsecDirection: enum.IntEnum
---@class credo.MACsecDirectionEnum : enum.IntEnumMeta
---@field public INGRESS credo.MACsecDirection
---@field public EGRESS credo.MACsecDirection
---@type credo.MACsecDirectionEnum
credo.MACsecDirection =
    enum.IntEnum("MACsecDirection", {EGRESS = _credo.MACSEC_EGRESS, INGRESS = _credo.MACSEC_INGRESS})
---@class credo.PrbsPattern: enum.IntEnum
---@class credo.PrbsPatternEnum : enum.IntEnumMeta
---@field public PRBS31 credo.PrbsPattern
---@field public PRBS9 credo.PrbsPattern
---@field public PRBS23 credo.PrbsPattern
---@field public PRBS11 credo.PrbsPattern
---@field public PRBS13 credo.PrbsPattern
---@field public PRBS7 credo.PrbsPattern
---@field public PRBS15 credo.PrbsPattern
---@field public PRBS19 credo.PrbsPattern
---@field public UNKNOWN credo.PrbsPattern
---@type credo.PrbsPatternEnum
credo.PrbsPattern = enum.IntEnum("PrbsPattern", {
    PRBS9 = _credo.PRBS9,
    PRBS7 = _credo.PRBS7,
    PRBS15 = _credo.PRBS15,
    PRBS19 = _credo.PRBS19,
    PRBS31 = _credo.PRBS31,
    UNKNOWN = _credo.PRBS_UNKNOWN,
    PRBS13 = _credo.PRBS13,
    PRBS11 = _credo.PRBS11,
    PRBS23 = _credo.PRBS23
})
credo.LanePrbsPattern = credo.PrbsPattern -- alias for backwards compatibility

---@class credo.TestPatternMode: enum.IntEnum
---@class credo.TestPatternModeEnum : enum.IntEnumMeta
---@field public CUSTOM credo.TestPatternMode
---@field public JP03A credo.TestPatternMode
---@field public UNKNOWN credo.TestPatternMode
---@field public LINEAR credo.TestPatternMode
---@field public JP03B credo.TestPatternMode
---@type credo.TestPatternModeEnum
credo.TestPatternMode = enum.IntEnum("TestPatternMode", {
    CUSTOM = _credo.TESTPATT_CUSTOM,
    JP03A = _credo.TESTPATT_JP03A,
    UNKNOWN = _credo.TESTPATT_UNKNOWN,
    JP03B = _credo.TESTPATT_JP03B,
    LINEAR = _credo.TESTPATT_LINEAR
})

credo.LaneTxTestPatternMode = credo.TestPatternMode

---@class credo.LaneTxState: enum.IntEnum
---@class credo.LaneTxStateEnum : enum.IntEnumMeta
---@field public TRAFFIC credo.LaneTxState
---@field public PRBS_PAM4 credo.LaneTxState
---@field public PRBS_NRZ credo.LaneTxState
---@field public SQUELCH credo.LaneTxState
---@field public FORCE_TRAFFIC credo.LaneTxState
---@field public FORCE_PRBS_NRZ credo.LaneTxState
---@field public FORCE_PRBSS_PAM4 credo.LaneTxState
---@field public FORCE_DISABLE credo.LaneTxState
---@field public LOWPOWER credo.LaneTxState
---@field public FORCE_TEST_PATT credo.LaneTxState
---@field public UNKNOWN credo.LaneTxState
---@type credo.LaneTxStateEnum
credo.LaneTxState = enum.IntEnum("LaneTxState", {
    FORCE_PRBS_NRZ = _credo.TX_FORCE_PRBS_NRZ,
    PRBS_NRZ = _credo.TX_PRBS_NRZ,
    PRBS_PAM4 = _credo.TX_PRBS_PAM4,
    LOWPOWER = _credo.TX_LOWPOWER,
    UNKNOWN = _credo.TX_UNKNOWN,
    TRAFFIC = _credo.TX_TRAFFIC,
    FORCE_TRAFFIC = _credo.TX_FORCE_TRAFFIC,
    SQUELCH = _credo.TX_SQUELCH,
    FORCE_PRBSS_PAM4 = _credo.TX_FORCE_PRBSS_PAM4,
    FORCE_DISABLE = _credo.TX_FORCE_DISABLE,
    FORCE_TEST_PATT = _credo.TX_FORCE_TEST_PATT
})
---@class credo.LaneLoopbackMode: enum.IntEnum
---@class credo.LaneLoopbackModeEnum : enum.IntEnumMeta
---@field public DISABLED credo.LaneLoopbackMode
---@field public TX_TO_RX credo.LaneLoopbackMode
---@field public RX_TO_TX credo.LaneLoopbackMode
---@type credo.LaneLoopbackModeEnum
credo.LaneLoopbackMode = enum.IntEnum("LaneLoopbackMode", {
    DISABLED = _credo.LB_DISABLED,
    RX_TO_TX = _credo.LB_RX_TO_TX,
    TX_TO_RX = _credo.LB_TX_TO_RX
})
---@class credo.FaultPropagation: enum.IntEnum
---@class credo.FaultPropagationEnum : enum.IntEnumMeta
---@field public SYS_TO_LINE credo.FaultPropagation
---@field public NONE credo.FaultPropagation
---@field public LINE_TO_SYS credo.FaultPropagation
---@field public BOTH credo.FaultPropagation
---@type credo.FaultPropagationEnum
credo.FaultPropagation = enum.IntEnum("FaultPropagation", {
    LINE_TO_SYS = _credo.FAULTPROP_LINE_TO_SYS,
    BOTH = _credo.FAULTPROP_BOTH,
    SYS_TO_LINE = _credo.FAULTPROP_SYS_TO_LINE,
    NONE = _credo.FAULTPROP_NONE
})
---@class credo.PortID: enum.IntEnum
---@class credo.PortIDEnum : enum.IntEnumMeta
---@field public UNCONFIGURED credo.PortID
---@field public AUTO_ASSIGN_ID credo.PortID
---@type credo.PortIDEnum
credo.PortID = enum.IntEnum("PortID",
                            {UNCONFIGURED = _credo.PORT_UNCONFIGURED, AUTO_ASSIGN_ID = _credo.PORT_AUTO_ASSIGN_ID})
---@class credo.PortFlags: enum.IntEnum
---@class credo.PortFlagsEnum : enum.IntEnumMeta
---@field public AUTONEG_DISABLE credo.PortFlags
---@field public LINE_SIDE_ANLT credo.PortFlags
---@field public LINE_SIDE_AN credo.PortFlags
---@field public LINE_SIDE_OPTICAL credo.PortFlags
---@field public SYS_SIDE_LT credo.PortFlags
---@field public AUTONEG_OVERRIDE credo.PortFlags
---@field public SYS_SIDE_OPTICAL credo.PortFlags
---@field public ENABLE_DOUBLE_CRC credo.PortFlags
---@type credo.PortFlagsEnum
credo.PortFlags = enum.IntEnum("PortFlags", {
    LINE_SIDE_ANLT = _credo.PFLAG_LINE_SIDE_ANLT,
    LINE_SIDE_OPTICAL = _credo.PFLAG_LINE_SIDE_OPTICAL,
    ENABLE_DOUBLE_CRC = _credo.PFLAG_ENABLE_DOUBLE_CRC,
    SYS_SIDE_LT = _credo.PFLAG_SYS_SIDE_LT,
    SYS_SIDE_OPTICAL = _credo.PFLAG_SYS_SIDE_OPTICAL,
    LINE_SIDE_AN = _credo.PFLAG_LINE_SIDE_AN,
    AUTONEG_DISABLE = _credo.PFLAG_AUTONEG_DISABLE,
    AUTONEG_OVERRIDE = _credo.PFLAG_AUTONEG_OVERRIDE
}, true)
---@class credo.PacketInjectMode: enum.IntEnum
---@class credo.PacketInjectModeEnum : enum.IntEnumMeta
---@field public DEBUG credo.PacketInjectMode
---@field public FUNC credo.PacketInjectMode
---@type credo.PacketInjectModeEnum
credo.PacketInjectMode = enum.IntEnum("PacketInjectMode", {FUNC = _credo.PKTINJ_FUNC, DEBUG = _credo.PKTINJ_DEBUG})
---@class credo.PrbsLockStatus: enum.IntEnum
---@class credo.PrbsLockStatusEnum : enum.IntEnumMeta
---@field public YES credo.PrbsLockStatus
---@field public INVALID credo.PrbsLockStatus
---@field public NO credo.PrbsLockStatus
---@type credo.PrbsLockStatusEnum
credo.PrbsLockStatus = enum.IntEnum("PrbsLockStatus", {
    YES = _credo.PRBS_LOCK_YES,
    NO = _credo.PRBS_LOCK_NO,
    INVALID = _credo.PRBS_LOCK_INVALID
})
---@class credo.PrbsTrainingStatus: enum.IntEnum
---@class credo.PrbsTrainingStatusEnum : enum.IntEnumMeta
---@field public ERROR credo.PrbsTrainingStatus
---@field public RELINK credo.PrbsTrainingStatus
---@field public OPTIMIZING credo.PrbsTrainingStatus
---@field public LINKED credo.PrbsTrainingStatus
---@type credo.PrbsTrainingStatusEnum
credo.PrbsTrainingStatus = enum.IntEnum("PrbsTrainingStatus", {
    LINKED = _credo.PRBS_TRAINING_LINKED,
    RELINK = _credo.PRBS_TRAINING_RELINK,
    OPTIMIZING = _credo.PRBS_TRAINING_OPTIMIZING,
    ERROR = _credo.PRBS_TRAINING_ERROR
})

---@class credo.ParamValue: enum.IntEnum
---@class credo.ParamValueEnum : enum.IntEnumMeta
---@field public FLOAT credo.ParamValue
---@field public INT credo.ParamValue
---@field public UINT credo.ParamValue
---@type credo.ParamValueEnum
credo.ParamValue = enum.IntEnum("ParamValue", {
    INT = _credo.PARAM_VAL_INT,
    UINT = _credo.PARAM_VAL_UINT,
    FLOAT = _credo.PARAM_VAL_FLOAT
})
---@class credo.ParamIndex: enum.IntEnum
---@class credo.ParamIndexEnum : enum.IntEnumMeta
---@field public SIDE credo.ParamIndex
---@field public LANE credo.ParamIndex
---@field public PORT credo.ParamIndex
---@field public TOP credo.ParamIndex
---@type credo.ParamIndexEnum
credo.ParamIndex = enum.IntEnum("ParamIndex", {
    LANE = _credo.PARAM_INDEX_LANE,
    PORT = _credo.PARAM_INDEX_PORT,
    TOP = _credo.PARAM_INDEX_TOP,
    SIDE = _credo.PARAM_INDEX_SIDE
})
---@class credo.LogLevel: enum.IntEnum
---@class credo.LogLevelEnum : enum.IntEnumMeta
---@field public INFO credo.LogLevel
---@field public WARN credo.LogLevel
---@field public DEBUG credo.LogLevel
---@field public TRACE credo.LogLevel
---@field public ERROR credo.LogLevel
---@type credo.LogLevelEnum
credo.LogLevel = enum.IntEnum("LogLevel", {
    WARN = _credo.LOG_WARN,
    DEBUG = _credo.LOG_DEBUG,
    ERROR = _credo.LOG_ERROR,
    TRACE = _credo.LOG_TRACE,
    INFO = _credo.LOG_INFO
})
---@class credo.Cipher: enum.IntEnum
---@class credo.CipherEnum : enum.IntEnumMeta
---@field public SM4 credo.Cipher
---@field public AES_GCM credo.Cipher
---@type credo.CipherEnum
credo.Cipher = enum.IntEnum("Cipher", {SM4 = _credo.CIPHER_SM4, AES_GCM = _credo.CIPHER_AES_GCM})
---@class credo.ParamFlags: enum.IntEnum
---@class credo.ParamFlagsEnum : enum.IntEnumMeta
---@field public VAR_COUNT credo.ParamFlags
---@field public VAR_SET_COUNT credo.ParamFlags
---@type credo.ParamFlagsEnum
credo.ParamFlags = enum.IntEnum("ParamFlags", {VAR_COUNT = _credo.PARAM_FLAG_VAR_COUNT}, true)
---@class credo.UsbProtocol : enum.IntEnum
---@class credo.UsbProtocolEnum: enum.IntEnumMeta
---@field public PCIE credo.UsbProtocol
---@field public USB credo.UsbProtocol
---@field public SATA credo.UsbProtocol
---@field public DISPLAY_PORT credo.UsbProtocol
---@field public USB4 credo.UsbProtocol
---@field public NONE credo.UsbProtocol
---@type credo.UsbProtocolEnum
credo.UsbProtocol = enum.IntEnum("UsbProtocol", {
    DISPLAY_PORT = _credo.USB_PROTO_DISPLAY_PORT,
    PCIE = _credo.USB_PROTO_PCIE,
    SATA = _credo.USB_PROTO_SATA,
    USB4 = _credo.USB_PROTO_USB4,
    NONE = _credo.USB_PROTO_NONE,
    USB = _credo.USB_PROTO_USB
})
---@class credo.LaneFlags : enum.IntEnum
---@class credo.LaneFlagsEnum: enum.IntEnumMeta
---@field public TX credo.LaneFlags
---@field public RX credo.LaneFlags
---@field public AN credo.LaneFlags
---@field public LT credo.LaneFlags
---@field public FLEXSPEED credo.LaneFlags
---@field public LOOPBACK credo.LaneFlags
---@type credo.LaneFlagsEnum
credo.LaneFlags = enum.IntEnum("LaneFlags", {
    LOOPBACK = _credo.LFLAG_LOOPBACK,
    TX = _credo.LFLAG_TX,
    RX = _credo.LFLAG_RX,
    LT = _credo.LFLAG_LT,
    AN = _credo.LFLAG_AN,
    FLEXSPEED = _credo.LFLAG_FLEXSPEED
}, true)
---@class credo.PacketCaptureMode : enum.IntEnum
---@class credo.PacketCaptureModeEnum: enum.IntEnumMeta
---@field public UNCOND credo.PacketCaptureMode
---@field public NONERASE credo.PacketCaptureMode
---@field public VPORT_EN credo.PacketCaptureMode
---@field public DST_MACADDR credo.PacketCaptureMode
---@type credo.PacketCaptureModeEnum
credo.PacketCaptureMode = enum.IntEnum("PacketCaptureMode", {
    NONERASE = _credo.PKTCAPT_NONERASE,
    UNCOND = _credo.PKTCAPT_UNCOND,
    VPORT_EN = _credo.PKTCAPT_VPORT_EN,
    DST_MACADDR = _credo.PKTCAPT_DST_MACADDR
})
---@class credo.PacketDef: enum.IntEnum
---@class credo.PacketDefEnum: enum.IntEnumMeta
---@field public MAX_SIZE credo.PacketDef
---@type credo.PacketDefEnum
credo.PacketDef = enum.IntEnum("PacketDef", {MAX_SIZE = _credo.PACKET_MAX_SIZE})

---@class credo.LinkTrainingStatus: enum.IntEnum
---@class credo.LinkTrainingStatusEnum: enum.IntEnumMeta
---@field public OFF credo.LinkTrainingStatus
---@field public IDLE credo.LinkTrainingStatus
---@field public TRAINING credo.LinkTrainingStatus
---@field public FINISHED credo.LinkTrainingStatus
---@field public FAILED credo.LinkTrainingStatus
---@type credo.LinkTrainingStatusEnum
credo.LinkTrainingStatus = enum.IntEnum("LinkTrainingStatus", {
    OFF = _credo.LT_STATUS_OFF,
    IDLE = _credo.LT_STATUS_IDLE,
    TRAINING = _credo.LT_STATUS_TRAINING,
    FINISHED = _credo.LT_STATUS_FINISHED,
    FAILED = _credo.LT_STATUS_FAILED
})

---@class credo.LinkTrainingState: enum.IntEnum
---@class credo.LinkTrainingStateEnum: enum.IntEnumMeta
---@field public UNKNOWN credo.LinkTrainingState
---@field public EXIT credo.LinkTrainingState
---@field public OFF credo.LinkTrainingState
---@field public IDLE credo.LinkTrainingState
---@field public LINK_UP credo.LinkTrainingState
---@field public START credo.LinkTrainingState
---@field public PAM2_WAIT_FRAME_LOCK credo.LinkTrainingState
---@field public PAM2_WAIT_REMOTE_LOCK credo.LinkTrainingState
---@field public PAM2_WAIT_INITIAL_CMD credo.LinkTrainingState
---@field public PAM2_TX_ADJUST credo.LinkTrainingState
---@field public PAM2_TX_ADJUST_DONE credo.LinkTrainingState
---@field public PAM4_START credo.LinkTrainingState
---@field public PAM4_WAIT_FRAME_LOCK credo.LinkTrainingState
---@field public PAM4_WAIT_PAM4_FRAME credo.LinkTrainingState
---@field public PAM4_TX_ADJUST credo.LinkTrainingState
---@field public PAM4_TX_ADJUST_DONE credo.LinkTrainingState
---@type credo.LinkTrainingStateEnum
credo.LinkTrainingState = enum.IntEnum("LinkTrainingState", {
    UNKNOWN = _credo.LT_STATE_UNKNOWN,
    EXIT = _credo.LT_STATE_EXIT,
    OFF = _credo.LT_STATE_OFF,
    IDLE = _credo.LT_STATE_IDLE,
    LINK_UP = _credo.LT_STATE_LINK_UP,
    START = _credo.LT_STATE_START,
    PAM2_WAIT_FRAME_LOCK = _credo.LT_STATE_PAM2_WAIT_FRAME_LOCK,
    PAM2_WAIT_REMOTE_LOCK = _credo.LT_STATE_PAM2_WAIT_REMOTE_LOCK,
    PAM2_WAIT_INITIAL_CMD = _credo.LT_STATE_PAM2_WAIT_INITIAL_CMD,
    PAM2_TX_ADJUST = _credo.LT_STATE_PAM2_TX_ADJUST,
    PAM2_TX_ADJUST_DONE = _credo.LT_STATE_PAM2_TX_ADJUST_DONE,
    PAM4_START = _credo.LT_STATE_PAM4_START,
    PAM4_WAIT_FRAME_LOCK = _credo.LT_STATE_PAM4_WAIT_FRAME_LOCK,
    PAM4_WAIT_PAM4_FRAME = _credo.LT_STATE_PAM4_WAIT_PAM4_FRAME,
    PAM4_TX_ADJUST = _credo.LT_STATE_PAM4_TX_ADJUST,
    PAM4_TX_ADJUST_DONE = _credo.LT_STATE_PAM4_TX_ADJUST_DONE
})

---@class credo.TestPoint
---@field public lane integer
---@field public group integer
---@field public mode integer
---@field public index integer
---@field public div2 boolean
---@field public flags integer
---@overload fun():credo.TestPoint
credo.TestPoint = _credo.TestPoint

---@param slice integer
---@param lane integer
---@param group integer
---@param mode integer
---@param index integer
---@param div2 boolean
---@param flags integer
function credo.testpoint_select(slice, lane, group, mode, index, div2, flags)
    local tp = credo.TestPoint()
    tp.lane = lane
    tp.group = group
    tp.mode = mode
    tp.index = index
    tp.div2 = div2
    tp.flags = flags
    _credo.testpoint_select(slice, tp)
end

---@param slice integer
function credo.testpoint_clear(slice)
    _credo.testpoint_clear(slice)
end

---@param slice integer
---@return number
function credo.testpoint_read(slice)
    return _credo.testpoint_read(slice)
end

---@type credo.LogLevel
credo.LogLevel = enum.IntEnum("LogLevel", {ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3, TRACE = 4})

---@return credo.LogLevel
function credo.logger_get_level()
    local level = _credo.logger_get_level()
    return credo.LogLevel(level)
end

---@param level credo.LogLevel
function credo.logger_set_level(level)
    _credo.logger_set_level(level.value)
end

---@class credo.LogFeature: enum.IntEnum
---@class credo.LogFeatureEnum: enum.IntEnumMeta
---@field public API credo.LogFeature
---@field public REG credo.LogFeature
---@field public TCM credo.LogFeature
---@type credo.LogFeatureEnum
credo.LogFeature = enum.IntEnum("LogFeature",
                                {API = _credo.LOG_FEAT_API, REG = _credo.LOG_FEAT_REG, TCM = _credo.LOG_FEAT_TCM})

---@class credo.LogIO: enum.IntEnum
---@class credo.LogIOEnum: enum.IntEnumMeta
---@field public OFF credo.LogIO
---@field public READ credo.LogIO
---@field public WRITE credo.LogIO
---@field public ON credo.LogIO
---@type credo.LogIOEnum
credo.LogIO = enum.IntEnum("LogIO", {
    OFF = _credo.LOG_IO_OFF,
    READ = _credo.LOG_IO_READ,
    WRITE = _credo.LOG_IO_WRITE,
    ON = _credo.LOG_IO_ON
})

---@param feature credo.LogFeature
---@param value credo.LogIO
function credo.logger_set_feature(feature, value)
    _credo.logger_set_feature(feature.value, value.value)
end

---@param feature credo.LogFeature
---@return credo.LogIO
function credo.logger_get_feature(feature)
    return credo.LogIO(_credo.logger_get_feature(feature.value))
end

credo.AUTONEG_STATE_UNKNOWN = credo.AutoNegState.UNKNOWN
credo.AUTONEG_STATE_OFF = credo.AutoNegState.OFF
credo.AUTONEG_STATE_ENABLE = credo.AutoNegState.ENABLE
credo.AUTONEG_STATE_TX_DISABLE = credo.AutoNegState.TX_DISABLE
credo.AUTONEG_STATE_ABILITY_DETECT = credo.AutoNegState.ABILITY_DETECT
credo.AUTONEG_STATE_ACK_DETECT = credo.AutoNegState.ACK_DETECT
credo.AUTONEG_STATE_COMPLETE_ACK = credo.AutoNegState.COMPLETE_ACK
credo.AUTONEG_STATE_NP_WAIT = credo.AutoNegState.NP_WAIT
credo.AUTONEG_STATE_GOOD_CHECK = credo.AutoNegState.GOOD_CHECK
credo.AUTONEG_STATE_LINK_STATUS_CHECK = credo.AutoNegState.LINK_STATUS_CHECK
credo.AUTONEG_STATE_PARALLEL_DET_FAULT = credo.AutoNegState.PARALLEL_DET_FAULT
credo.AUTONEG_STATE_GOOD = credo.AutoNegState.GOOD
credo.CIPHER_AES_GCM = credo.Cipher.AES_GCM
credo.CIPHER_SM4 = credo.Cipher.SM4
credo.DEV_ADMIRAL_TEST = credo.DeviceType.ADMIRAL_TEST
credo.DEV_BALDEAGLE_400 = credo.DeviceType.BALDEAGLE_400
credo.DEV_BALDEAGLE_800 = credo.DeviceType.BALDEAGLE_800
credo.DEV_BLACKHAWK_400 = credo.DeviceType.BLACKHAWK_400
credo.DEV_BLACKHAWK_800 = credo.DeviceType.BLACKHAWK_800
credo.DEV_FAKE_400 = credo.DeviceType.FAKE_400
credo.DEV_FAKE_800 = credo.DeviceType.FAKE_800
credo.DEV_FAKE_32 = credo.DeviceType.FAKE_32
credo.DEV_HERON_1P0 = credo.DeviceType.HERON_1P0
credo.DEV_HERON_MR = credo.DeviceType.HERON_MR
credo.DEV_HERON_P1 = credo.DeviceType.HERON_P1
credo.DEV_HERON_P3 = credo.DeviceType.HERON_P3
credo.DEV_KITE_TEST = credo.DeviceType.KITE_TEST
credo.DEV_NUTCRACKER_32 = credo.DeviceType.NUTCRACKER_32
credo.DEV_OSPREY_400 = credo.DeviceType.OSPREY_400
credo.DEV_OSPREY_800 = credo.DeviceType.OSPREY_800
credo.DEV_OSPREY_AEC = credo.DeviceType.OSPREY_AEC
credo.DEV_SCREAMING_EAGLE = credo.DeviceType.SCREAMING_EAGLE
credo.DEV_SCREAMING_EAGLE_AEC = credo.DeviceType.SCREAMING_EAGLE_AEC
credo.DEV_OWL_400 = credo.DeviceType.OWL_400
credo.DEV_OWL_800 = credo.DeviceType.OWL_800
credo.DEV_SEAHAWK = credo.DeviceType.SEAHAWK
credo.DEV_OSTRICH_1P1 = credo.DeviceType.OSTRICH_1P1
credo.DEV_VICEROY_IN_NC = credo.DeviceType.VICEROY_IN_NC
credo.DEV_VICEROY_TEST = credo.DeviceType.VICEROY_TEST
credo.DEV_RAVEN_TEST = credo.DeviceType.RAVEN_TEST
credo.DEV_NIGHTHAWK_1P0 = credo.DeviceType.NIGHTHAWK_1P0
credo.DEV_NIGHTHAWK = credo.DeviceType.NIGHTHAWK
credo.DEV_PEREGRINE_EVB = credo.DeviceType.PEREGRINE_EVB
credo.DEV_PEREGRINE = credo.DeviceType.PEREGRINE
credo.DEV_CROW = credo.DeviceType.CROW
credo.DEV_SKIPPER_TEST = credo.DeviceType.SKIPPER_TEST
credo.DEV_SKIPPER = credo.DeviceType.SKIPPER
credo.DEV_SPARROW_800 = credo.DeviceType.SPARROW_800
credo.DEV_BLUEJAY = credo.DeviceType.BLUEJAY
credo.DEV_MONARCH2P1_TEST = credo.DeviceType.MONARCH2P1_TEST
credo.DEV_MONARCH2P1 = credo.DeviceType.MONARCH2P1
credo.EYE_MONITOR_BATHTUB = credo.EyeMonitorFlags.BATHTUB
credo.EYE_MONITOR_DESTRUCTIVE = credo.EyeMonitorFlags.DESTRUCTIVE
credo.FAULTPROP_BOTH = credo.FaultPropagation.BOTH
credo.FAULTPROP_LINE_TO_SYS = credo.FaultPropagation.LINE_TO_SYS
credo.FAULTPROP_NONE = credo.FaultPropagation.NONE
credo.FAULTPROP_SYS_TO_LINE = credo.FaultPropagation.SYS_TO_LINE
credo.FAMILY_BALDEAGLE = credo.Family.BALDEAGLE
credo.FAMILY_OWL = credo.Family.OWL
credo.FAMILY_BLACKHAWK = credo.Family.BLACKHAWK
credo.FAMILY_HERON = credo.Family.HERON
credo.FAMILY_OSPREY = credo.Family.OSPREY
credo.FAMILY_NUTCRACKER = credo.Family.NUTCRACKER
credo.FAMILY_KITE = credo.Family.KITE
credo.FAMILY_ADMIRAL = credo.Family.ADMIRAL
credo.FAMILY_SEAHAWK = credo.Family.SEAHAWK
credo.FAMILY_OSTRICH = credo.Family.OSTRICH
credo.FAMILY_SCREAMING_EAGLE = credo.Family.SCREAMING_EAGLE
credo.FAMILY_VICEROY = credo.Family.VICEROY
credo.FAMILY_NIGHTHAWK = credo.Family.NIGHTHAWK
credo.FAMILY_RAVEN = credo.Family.RAVEN
credo.FAMILY_BLUEJAY = credo.Family.BLUEJAY
credo.FAMILY_SPARROW = credo.Family.SPARROW
credo.FAMILY_MONARCH2P1_TEST = credo.Family.MONARCH2P1_TEST
credo.FAMILY_SKIPPER_TEST = credo.Family.SKIPPER_TEST
credo.FAMILY_FAKE = credo.Family.FAKE
credo.FEC_ERROR_BIT = credo.FecErrorType.BIT
credo.FEC_ERROR_FRAME = credo.FecErrorType.FRAME
credo.FEC_ERROR_SYMBOL = credo.FecErrorType.SYMBOL
credo.FEC_FIRE_CODE = credo.FecType.FIRE_CODE
credo.FEC_NONE = credo.FecType.NONE
credo.FEC_RS_528 = credo.FecType.RS_528
credo.FEC_RS_544 = credo.FecType.RS_544
credo.COUPLING_AC = credo.LaneCoupling.AC
credo.COUPLING_DC = credo.LaneCoupling.DC
credo.LB_DISABLED = credo.LaneLoopbackMode.DISABLED
credo.LB_RX_TO_TX = credo.LaneLoopbackMode.RX_TO_TX
credo.LB_TX_TO_RX = credo.LaneLoopbackMode.TX_TO_RX
credo.LMODE_AN = credo.LaneMode.AN
credo.LMODE_DISABLE = credo.LaneMode.DISABLE
credo.LMODE_NRZ = credo.LaneMode.NRZ
credo.LMODE_OFF = credo.LaneMode.OFF
credo.LMODE_PAM4 = credo.LaneMode.PAM4
credo.LMODE_PAM3 = credo.LaneMode.PAM3
credo.PRBS11 = credo.PrbsPattern.PRBS11
credo.PRBS13 = credo.PrbsPattern.PRBS13
credo.PRBS15 = credo.PrbsPattern.PRBS15
credo.PRBS23 = credo.PrbsPattern.PRBS23
credo.PRBS31 = credo.PrbsPattern.PRBS31
credo.PRBS7 = credo.PrbsPattern.PRBS7
credo.PRBS9 = credo.PrbsPattern.PRBS9
credo.PRBS19 = credo.PrbsPattern.PRBS19
credo.PRBS_UNKNOWN = credo.PrbsPattern.UNKNOWN
credo.REG_VERIFY_OVERRIDE_ADDRESS = credo.RegVerifyFlags.OVERRIDE_ADDRESS
credo.REG_VERIFY_BURST = credo.RegVerifyFlags.BURST
credo.REG_VERIFY_USE_DURATION = credo.RegVerifyFlags.USE_DURATION
credo.TX_FORCE_DISABLE = credo.LaneTxState.FORCE_DISABLE
credo.TX_FORCE_PRBSS_PAM4 = credo.LaneTxState.FORCE_PRBSS_PAM4
credo.TX_FORCE_PRBS_NRZ = credo.LaneTxState.FORCE_PRBS_NRZ
credo.TX_FORCE_TEST_PATT = credo.LaneTxState.FORCE_TEST_PATT
credo.TX_FORCE_TRAFFIC = credo.LaneTxState.FORCE_TRAFFIC
credo.TX_LOWPOWER = credo.LaneTxState.LOWPOWER
credo.TX_PRBS_NRZ = credo.LaneTxState.PRBS_NRZ
credo.TX_PRBS_PAM4 = credo.LaneTxState.PRBS_PAM4
credo.TX_SQUELCH = credo.LaneTxState.SQUELCH
credo.TX_TRAFFIC = credo.LaneTxState.TRAFFIC
credo.TX_UNKNOWN = credo.LaneTxState.UNKNOWN
credo.TESTPATT_CUSTOM = credo.TestPatternMode.CUSTOM
credo.TESTPATT_JP03A = credo.TestPatternMode.JP03A
credo.TESTPATT_JP03B = credo.TestPatternMode.JP03B
credo.TESTPATT_LINEAR = credo.TestPatternMode.LINEAR
credo.TESTPATT_UNKNOWN = credo.TestPatternMode.UNKNOWN
credo.LOG_DEBUG = credo.LogLevel.DEBUG
credo.LOG_ERROR = credo.LogLevel.ERROR
credo.LOG_INFO = credo.LogLevel.INFO
credo.LOG_TRACE = credo.LogLevel.TRACE
credo.LOG_WARN = credo.LogLevel.WARN
credo.MACSEC_EGRESS = credo.MACsecDirection.EGRESS
credo.MACSEC_INGRESS = credo.MACsecDirection.INGRESS
credo.PKTINJ_DEBUG = credo.PacketInjectMode.DEBUG
credo.PKTINJ_FUNC = credo.PacketInjectMode.FUNC
credo.PARAM_INDEX_LANE = credo.ParamIndex.LANE
credo.PARAM_INDEX_PORT = credo.ParamIndex.PORT
credo.PARAM_INDEX_SIDE = credo.ParamIndex.SIDE
credo.PARAM_INDEX_TOP = credo.ParamIndex.TOP
credo.PARAM_FLAG_VAR_COUNT = credo.ParamFlags.VAR_COUNT
credo.PARAM_VAL_FLOAT = credo.ParamValue.FLOAT
credo.PARAM_VAL_INT = credo.ParamValue.INT
credo.PARAM_VAL_UINT = credo.ParamValue.UINT
credo.PORT_BITMUX = credo.PortType.BITMUX
credo.PORT_GEARBOX = credo.PortType.GEARBOX
credo.PORT_SWITCHOVER_RETIMER = credo.PortType.SWITCHOVER_RETIMER
credo.PORT_RETIMER = credo.PortType.RETIMER
credo.PORT_DIR_BIDIRECTIONAL = credo.PortDirection.BIDIRECTIONAL
credo.PORT_DIR_HOST_TO_LINE = credo.PortDirection.HOST_TO_LINE
credo.PORT_DIR_LINE_TO_HOST = credo.PortDirection.LINE_TO_HOST
credo.PORT_DIR_INGRESS = credo.PortDirection.INGRESS
credo.PORT_DIR_EGRESS = credo.PortDirection.EGRESS
credo.PFLAG_AUTONEG_DISABLE = credo.PortFlags.AUTONEG_DISABLE
credo.PFLAG_AUTONEG_OVERRIDE = credo.PortFlags.AUTONEG_OVERRIDE
credo.PFLAG_ENABLE_DOUBLE_CRC = credo.PortFlags.ENABLE_DOUBLE_CRC
credo.PFLAG_LINE_SIDE_AN = credo.PortFlags.LINE_SIDE_AN
credo.PFLAG_LINE_SIDE_ANLT = credo.PortFlags.LINE_SIDE_ANLT
credo.PFLAG_LINE_SIDE_LT = credo.PFLAG_LINE_SIDE_ANLT | credo.PFLAG_AUTONEG_DISABLE
credo.PFLAG_LINE_SIDE_OPTICAL = credo.PortFlags.LINE_SIDE_OPTICAL
credo.PFLAG_SYS_SIDE_LT = credo.PortFlags.SYS_SIDE_LT
credo.PFLAG_SYS_SIDE_OPTICAL = credo.PortFlags.SYS_SIDE_OPTICAL
credo.PORT_UNCONFIGURED = credo.PortID.UNCONFIGURED
credo.PORT_AUTO_ASSIGN_ID = credo.PortID.AUTO_ASSIGN_ID
credo.PMODE_MACSEC = credo.PortMode.MACSEC
credo.PMODE_PCS = credo.PortMode.PCS
credo.PMODE_SERDES = credo.PortMode.SERDES
credo.PRBS_LOCK_INVALID = credo.PrbsLockStatus.INVALID
credo.PRBS_LOCK_NO = credo.PrbsLockStatus.NO
credo.PRBS_LOCK_YES = credo.PrbsLockStatus.YES
credo.PRBS_TRAINING_ERROR = credo.PrbsTrainingStatus.ERROR
credo.PRBS_TRAINING_RELINK = credo.PrbsTrainingStatus.RELINK
credo.PRBS_TRAINING_OPTIMIZING = credo.PrbsTrainingStatus.OPTIMIZING
credo.PRBS_TRAINING_LINKED = credo.PrbsTrainingStatus.LINKED
credo.SIDE_HOST = credo.Side.HOST
credo.SIDE_LINE = credo.Side.LINE
credo.SIDE_SYSTEM = credo.Side.SYSTEM
credo.INIT_FULL = credo.SliceInitType.FULL
credo.INIT_NONE = credo.SliceInitType.NONE
credo.INIT_NO_FIRMWARE = credo.SliceInitType.NO_FIRMWARE
credo.INIT_WARM = credo.SliceInitType.WARM
credo.INIT_SPIFLASH = credo.SliceInitType.SPIFLASH
credo.SLC_ADMIRAL = credo.SliceType.ADMIRAL
credo.SLC_BALDEAGLE = credo.SliceType.BALDEAGLE
credo.SLC_BLACKHAWK = credo.SliceType.BLACKHAWK
credo.SLC_BLACKHAWK_AC = credo.SliceType.BLACKHAWK_AC
credo.SLC_BLACKHAWK_DC = credo.SliceType.BLACKHAWK_DC
credo.SLC_SEAHAWK = credo.SliceType.BLACKHAWK_DC
credo.SLC_FAKE = credo.SliceType.FAKE
credo.SLC_HERON = credo.SliceType.HERON
credo.SLC_KITE = credo.SliceType.KITE
credo.SLC_NUTCRACKER = credo.SliceType.NUTCRACKER
credo.SLC_OSPREY = credo.SliceType.OSPREY
credo.SLC_OWL = credo.SliceType.OWL
credo.SLC_OWL_A0 = credo.SliceType.OWL_A0
credo.SLC_OWL_B0 = credo.SliceType.OWL_B0
credo.SLC_OWL_B0B = credo.SliceType.OWL_B0B
credo.SLC_OWL_B1 = credo.SliceType.OWL_B1
credo.SLC_SEAHAWK = credo.SliceType.SEAHAWK
credo.SLC_OSTRICH = credo.SliceType.OSTRICH
credo.SLC_SCREAMING_EAGLE = credo.SliceType.SCREAMING_EAGLE
credo.SLC_VICEROY_IN_NC = credo.SliceType.VICEROY_IN_NC
credo.SLC_VICEROY_TEST = credo.SliceType.VICEROY_TEST
credo.SLC_RAVEN = credo.SliceType.RAVEN
credo.SLC_NIGHTHAWK = credo.SliceType.NIGHTHAWK
credo.SLC_PEREGRINE_EVB = credo.SliceType.PEREGRINE_EVB
credo.SLC_PEREGRINE = credo.SliceType.PEREGRINE
credo.SLC_CROW = credo.SliceType.CROW
credo.SLC_SKIPPER_TEST = credo.SliceType.SKIPPER_TEST
credo.SLC_SPARROW = credo.SliceType.SPARROW
credo.SLC_BLUEJAY = credo.SliceType.BLUEJAY
credo.SLC_MONARCH2P1_TEST = credo.SliceType.MONARCH2P1_TEST
credo.SRAM_CORR_ERROR = credo.SramStatus.CORR_ERROR
credo.SRAM_NO_ERROR = credo.SramStatus.NO_ERROR
credo.SRAM_UNCORR_ERROR = credo.SramStatus.UNCORR_ERROR
credo.USB_PROTO_PCIE = credo.UsbProtocol.PCIE
credo.USB_PROTO_USB = credo.UsbProtocol.USB
credo.USB_PROTO_SATA = credo.UsbProtocol.SATA
credo.USB_PROTO_DISPLAY_PORT = credo.UsbProtocol.DISPLAY_PORT
credo.USB_PROTO_USB4 = credo.UsbProtocol.USB4
credo.USB_PROTO_NONE = credo.UsbProtocol.NONE
credo.LFLAG_TX = credo.LaneFlags.TX
credo.LFLAG_RX = credo.LaneFlags.RX
credo.LFLAG_AN = credo.LaneFlags.AN
credo.LFLAG_LT = credo.LaneFlags.LT
credo.LFLAG_LOOPBACK = credo.LaneFlags.LOOPBACK
credo.LFLAG_FLEXSPEED = credo.LaneFlags.FLEXSPEED
credo.PKTCAPT_UNCOND = credo.PacketCaptureMode.UNCOND
credo.PKTCAPT_NONERASE = credo.PacketCaptureMode.NONERASE
credo.PKTCAPT_VPORT_EN = credo.PacketCaptureMode.VPORT_EN
credo.PKTCAPT_DST_MACADDR = credo.PacketCaptureMode.DST_MACADDR
credo.PACKET_MAX_SIZE = credo.PacketDef.MAX_SIZE
credo.LT_STATUS_OFF = credo.LinkTrainingStatus.OFF
credo.LT_STATUS_IDLE = credo.LinkTrainingStatus.IDLE
credo.LT_STATUS_TRAINING = credo.LinkTrainingStatus.TRAINING
credo.LT_STATUS_FINISHED = credo.LinkTrainingStatus.FINISHED
credo.LT_STATUS_FAILED = credo.LinkTrainingStatus.FAILED
credo.LT_STATE_UNKNOWN = credo.LinkTrainingState.UNKNOWN
credo.LT_STATE_EXIT = credo.LinkTrainingState.EXIT
credo.LT_STATE_OFF = credo.LinkTrainingState.OFF
credo.LT_STATE_IDLE = credo.LinkTrainingState.IDLE
credo.LT_STATE_LINK_UP = credo.LinkTrainingState.LINK_UP
credo.LT_STATE_START = credo.LinkTrainingState.START
credo.LT_STATE_PAM2_WAIT_FRAME_LOCK = credo.LinkTrainingState.PAM2_WAIT_FRAME_LOCK
credo.LT_STATE_PAM2_WAIT_REMOTE_LOCK = credo.LinkTrainingState.PAM2_WAIT_REMOTE_LOCK
credo.LT_STATE_PAM2_WAIT_INITIAL_CMD = credo.LinkTrainingState.PAM2_WAIT_INITIAL_CMD
credo.LT_STATE_PAM2_TX_ADJUST = credo.LinkTrainingState.PAM2_TX_ADJUST
credo.LT_STATE_PAM2_TX_ADJUST_DONE = credo.LinkTrainingState.PAM2_TX_ADJUST_DONE
credo.LT_STATE_PAM4_START = credo.LinkTrainingState.PAM4_START
credo.LT_STATE_PAM4_WAIT_FRAME_LOCK = credo.LinkTrainingState.PAM4_WAIT_FRAME_LOCK
credo.LT_STATE_PAM4_WAIT_PAM4_FRAME = credo.LinkTrainingState.PAM4_WAIT_PAM4_FRAME
credo.LT_STATE_PAM4_TX_ADJUST = credo.LinkTrainingState.PAM4_TX_ADJUST
credo.LT_STATE_PAM4_TX_ADJUST_DONE = credo.LinkTrainingState.PAM4_TX_ADJUST_DONE
credo.FRECOV_DISABLED = credo.FastRecoverStatus.DISABLED
credo.FRECOV_ENABLED = credo.FastRecoverStatus.ENABLED
credo.FRECOV_ARMED = credo.FastRecoverStatus.ARMED

credo.LOG_FEAT_API = credo.LogFeature.API
credo.LOG_FEAT_REG = credo.LogFeature.REG
credo.LOG_FEAT_TCM = credo.LogFeature.TCM
credo.LOG_IO_OFF = credo.LogIO.OFF
credo.LOG_IO_READ = credo.LogIO.READ
credo.LOG_IO_WRITE = credo.LogIO.WRITE
credo.LOG_IO_ON = credo.LogIO.ON

-- internal tools

---@param slice integer
---@return table<string, integer[]>
function credo.reghive_get_map(slice)
    local count = _credo.cri_slice_get_reghive_count(slice)
    local reghives = {}
    for i = 0, count - 1 do
        local reghive = _credo.CrIntlRegHive_t()
        local data = {}

        _credo.cri_slice_index_reghive(slice, i, reghive)
        for idx = 0, reghive.count - 1 do
            local addr = _credo.cri_slice_get_addr(slice, idx, reghive, 0)
            list.append(data, addr)
        end

        reghives[string.lower(reghive.hivename)] = data
    end
    return reghives
end

--- Make all structs with enum types use them and have nice luaish bindings
do
    local struct = require "struct"

    struct.enum(credo.PortConfig, "flags", credo.PortFlags)
    struct.enum(credo.PortConfig, "port_type", credo.PortType)
    struct.enum(credo.PortConfig, "port_mode", credo.PortMode)
    struct.enum(credo.PortConfig, "host_fec_type", credo.FecType)
    struct.enum(credo.PortConfig, "line_fec_type", credo.FecType)
    struct.enum(credo.PortConfig, "port_id", credo.PortID)

    struct.enum(credo.PortSetup, "mode", credo.PortType)

    struct.enum(credo.FecAnalyzerConfig, "error_type", credo.FecErrorType)
    struct.enum(credo.Param, "val_type", credo.ParamValue)
    struct.enum(credo.Param, "index_type", credo.ParamIndex)

    struct.enum(credo.PacketInjectConfig, "mode", credo.PacketInjectMode, true)
    struct.enum(credo.ChannelDesc, "mode", credo.LaneMode)
    struct.enum(credo.RegVerifyConfig, "flags", credo.RegVerifyFlags)
    struct.set_struct_tostrings(credo)
end

return credo
