
#include "dii.h"

CredoError_t cr_tcm_read(CredoSlice_t* slice, unsigned addr, unsigned* data) {
    CALL_HAL(slice, hal_tcm_read(slice, addr, data));
}

CredoError_t cr_tcm_write(CredoSlice_t* slice, unsigned addr, unsigned data) {
    CALL_HAL(slice, hal_tcm_write(slice, addr, data));
}

CredoError_t cr_tcm_burst_read(CredoSlice_t* slice, unsigned first_address, uint64_t val[], unsigned count) {
    CALL_HAL(slice, hal_tcm_burst_read(slice, first_address, val, count));
}

CredoError_t cr_tcm_burst_write(CredoSlice_t* slice, unsigned first_address, const uint64_t val[], unsigned count) {
    CALL_HAL(slice, hal_tcm_burst_write(slice, first_address, val, count));
}

CredoError_t cri_tcm_get_base_address(CredoSlice_t* slice, const CrIntlRegHive_t* reghive, unsigned* base_addr) {
    CALL_HAL(slice, hal_tcm_get_base_address(slice, (const RegHive_t*)reghive->handler, base_addr));
}

CredoError_t cri_tcm_get_burst_width(CredoSlice_t* slice, unsigned address, CrIntlTCMBurstWidth_t* width) {
    CALL_HAL(slice, hal_get_tcm_burst_width(slice, address, (TCMBurstWidth_t*)width));
}

CredoError_t cr_tcm_memset(CredoSlice_t* slice, const CredoTCMBurstIOCtrl_t* io_ctrl, unsigned value) {
    CALL_HAL(slice, hal_tcm_memset(slice, io_ctrl, value));
}

CredoError_t cr_tcm_range_program(CredoSlice_t* slice, int index, const CredoTCMBurstIORangeProgram_t* range_param) {
    CALL_HAL(slice, hal_tcm_range_program(slice, index, range_param));
}

CredoError_t cr_tcm_multi_slice_range_read(CredoSlice_t* slices[], int slice_count,
                                           const CredoTCMBurstIORange_t* io_range[], CredoTCMBurstIOData_t* data[]) {
    // needs to be more than 1 slice
    if (slices == NULL || slice_count < 1) return CR_INVALID_ARGS;

    CredoError_t ret;
    int lock_index = 0;
    for (lock_index = 0; lock_index < slice_count; lock_index++) {
        ERR_CATCH_SLICE(slices[lock_index], (ret = cr_slice_lock(slices[lock_index])), goto exit);
    }

    ret = hal_tcm_multi_slice_range_read(slices, slice_count, io_range, data);
    if (ret != CR_OK) goto exit;

exit:
    // unlock any opened locks
    for (int i = 0; i < lock_index; i++) {
        cr_slice_unlock(slices[i]);
    }
    return ret;
}

/* PCS register (16-bit) read/write. Address=pcs_base_addr(port, side)+offset*2. */

CredoError_t cr_pcs_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned* val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_pcs_read(slice, portId, side, offset, val));
}

CredoError_t cr_pcs_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_pcs_write(slice, portId, side, offset, val));
}
/* PCS status read. Equivalent to read PCS offset 1 (address 0x02). */

CredoError_t cr_pcs_status_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned* pcs_status) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_pcs_status_read(slice, portId, side, pcs_status));
}
/* RS-FEC register (32/64-bit) read/write. Address=fec_base_addr(port, side)+offset*4.
 * For 64-bit registers, MSB are in register DATA_HI. */

CredoError_t cr_rsfec_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned* val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_rs_fec_read(slice, portId, side, offset, val));
}

CredoError_t cr_rsfec_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_rs_fec_write(slice, portId, side, offset, val));
}
/* MAC register (32-bit) read/write. Address=mac_base_addr(port, side)+offset*4.
 * *** For status register (reg 16), must use cr_mac_status_read() */

CredoError_t cr_mac_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned* val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_read(slice, portId, side, offset, val));
}

CredoError_t cr_mac_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_write(slice, portId, side, offset, val));
}
/* MAC status read. Equivalent to read MAC offset 16 (address 0x40), but goes through firmware */

CredoError_t cr_mac_status_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned* mac_status) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_status_read(slice, portId, side, mac_status));
}
/* MAC statistics register (32/64-bit) read/write. Address=mac_stat_base_addr(port, side)+offset*4.
 * For 64-bit registers, MSB are in register DATA_HI. */

CredoError_t cr_mac_stats_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned* val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_stats_read(slice, portId, side, offset, val));
}
CredoError_t cr_mac_stats_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset, unsigned val) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_stats_write(slice, portId, side, offset, val));
}

CredoError_t cr_mac_stats_read_counters(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                        CredoMACStatistics_t* stats) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_stats_read_counters(slice, portId, side, stats));
}

CredoError_t cr_mac_stats_clear_counters(CredoSlice_t* slice, uint32_t portId, CredoSide_t side) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_stats_clear_counters(slice, portId, side));
}

/* MAC and EIP stop function before port is stopped. */

CredoError_t cr_mac_stop(CredoSlice_t* slice, uint32_t portId) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_mac_stop(slice, portId));
}

CredoError_t cr_eip_stop(CredoSlice_t* slice, uint32_t portId) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_eip_stop(slice, portId));
}

/* MAC and EIP configuration function. Should be replaceable */

CredoError_t cr_eip_configure(CredoSlice_t* slice, uint32_t portId) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_configure_eip(slice, portId));
}

CredoError_t cr_mac_configure(CredoSlice_t* slice, uint32_t portId) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_configure_mac(slice, portId));
}
/* Per-slice topology control. Must change without running ports. */

CredoError_t cr_eip_display_topologies(CredoSlice_t* slice) {
    CALL_HAL(slice, hal_display_topology_types(slice));
}

CredoError_t cr_eip_set_topology(CredoSlice_t* slice, const char* topology) {
    CALL_HAL(slice, hal_set_topology(slice, topology));
}

CredoError_t cr_eip_get_topology(CredoSlice_t* slice, char* topology, unsigned len) {
    CALL_HAL(slice, hal_get_topology(slice, topology, len));
}
/* Fault propagation control */

CredoError_t cr_mac_set_faultprop(CredoSlice_t* slice, uint32_t portId, CredoFaultPropagation_t enable) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_fault_propagation_control(slice, portId, enable));
}

CredoError_t cr_mac_get_faultprop(CredoSlice_t* slice, uint32_t portId, CredoFaultPropagation_t* enable) {
    CHECK_PORT_VALID(slice, portId);
    CALL_HAL(slice, hal_fault_propagation_status(slice, portId, enable));
}

CredoError_t cr_eip_set_low_latency_bypass(CredoSlice_t* slice, uint32_t port_id, CredoMACsecDirection_t direction,
                                           uint8_t enable) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_enable_low_latency_bypass(slice, port_id, direction, enable));
}

CredoError_t cr_eip_get_low_latency_bypass(CredoSlice_t* slice, uint32_t port_id, CredoMACsecDirection_t direction,
                                           uint8_t* enable) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_low_latency_bypass_status(slice, port_id, direction, enable));
}

CredoError_t cr_eip_get_client_id(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint32_t* client) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_get_client_id(slice, port_id, side, client));
}
CredoError_t cr_eip_get_channel_id(CredoSlice_t* slice, uint32_t port_id, uint32_t* channel_id) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_get_channel_id(slice, port_id, channel_id));
}

CredoError_t cr_eip_get_macsec_datapath(CredoSlice_t* slice, CredoMACsecDirection_t direction,
                                        CredoMACsecDataPath_t* datapath) {
    CALL_HAL(slice, hal_get_macsec_datapath(slice, direction, datapath));
}

CredoError_t cr_eip_inject_packet(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                  CredoPacketInjectConfig_t* pkt_config) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_packet_inject(slice, port_id, side, pkt_config));
}

CredoError_t cr_eip_setup_capture_packet(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                         CredoPacketCaptureConfig_t* pkt_config) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_setup_capture_packet(slice, port_id, side, pkt_config));
}

CredoError_t cr_eip_stop_capture_packet(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    CHECK_PORT_VALID(slice, port_id);
    CALL_HAL(slice, hal_stop_capture_packet(slice, port_id, side));
}

CredoError_t cr_eip_get_capture_packet_buffer_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                     CredoPacketCaptureBufferStatus_t* status) {
    CALL_HAL(slice, hal_get_capture_packet_buffer_status(slice, port_id, side, status));
}

CredoError_t cr_eip_clear_capture_packet_buffer(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    CALL_HAL(slice, hal_clear_capture_packet_buffer(slice, port_id, side));
}

CredoError_t cr_eip_get_capture_packet_info(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                            CredoPacketCaptureInfo_t* pkt_info) {
    CALL_HAL(slice, hal_get_capture_packet_info(slice, port_id, side, pkt_info));
}
