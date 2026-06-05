#include "dii.h"

#include "crintl/firmware.h"
#include "sbs.h"

#include <stdio.h>

// Firmware Management

CredoError_t cr_firmware_unload(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_fw_unload(slice));
}

CredoError_t cr_firmware_load(CredoSlice_t* slice, const char* image_file, int force) {
    LOGS_API("image_file: %s", image_file);
    if (slice == NULL) return CR_INVALID_ARGS;
    if (force) {
        LOGS_WARN("force parameter in firmware load does nothing");
    }

    SLICE_LOCK_GUARD(slice);

    FILE* fw;
    CredoError_t ret;
    fw = fopen(image_file, "rb");

    if (!fw) {
        LOGS_ERROR("[Firmware load] Error opening firmware file %s", image_file);
        SLICE_UNLOCK(slice);
        return CR_FAIL;
    }
    /* Unload firmware */
    if (hal_fw_unload(slice) != CR_OK) {
        fclose(fw);
        SLICE_UNLOCK(slice);
        return CR_FAIL;
    }
    ret = hal_fw_load(slice, fw);
    //(table->hal_fw_load)(slice, fw);
    fclose(fw);
    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_firmware_loadbuf(CredoSlice_t* slice, void* buf, size_t size) {
#ifndef _WIN32
    LOGS_API("size %u", (unsigned)size);
    if (slice == NULL) return CR_INVALID_ARGS;

    SLICE_LOCK_GUARD(slice);

    FILE* fw;
    CredoError_t ret;
    fw = fmemopen(buf, size, "rb");

    if (!fw) {
        LOGS_ERROR("[Firmware load] Error opening firmware buffer");
        SLICE_UNLOCK(slice);
        return CR_FAIL;
    }
    /* Unload firmware */
    if (hal_fw_unload(slice) != CR_OK) {
        fclose(fw);
        SLICE_UNLOCK(slice);
        return CR_FAIL;
    }
    ret = hal_fw_load(slice, fw);
    fclose(fw);
    SLICE_UNLOCK(slice);
    return ret;
#else
    // windows doesnt support fmemopen
    return CR_NOTIMPLEMENTED;
#endif
}

CredoError_t cr_firmware_load_broadcast(CredoSlice_t* slices[], int slice_count, const char* image_file,
                                        unsigned delay_time_us, int force) {
    LOGSDK_API("slice_count %d, image_file: %s, delay_time_us %u", slice_count, image_file, delay_time_us);
    // needs to be more than 1 slice
    if (slices == NULL || slice_count < 1) return CR_INVALID_ARGS;
    CredoSlice_t* start_slice = slices[0];

    // ensure broadcast firmware load defined
    if (!CHECK_HAL(start_slice, hal_fw_load_broadcast)) return CR_INVALID_ARGS;

    FILE* fw;
    CredoError_t ret;
    fw = fopen(image_file, "rb");
    if (!fw) {
        LOG_ERROR(slices[0], "[Firmware load] Error opening firmware file %s", image_file);
        return CR_FAIL;
    }
    // cannot use slice lock guard for variable length components :(
    int lock_index = 0;
    for (lock_index = 0; lock_index < slice_count; lock_index++) {
        ERR_CATCH_SLICE(slices[lock_index], (ret = cr_slice_lock(slices[lock_index])), goto exit);
    }
    /* Unload firmware */
    for (int i = 0; i < slice_count; i++) {
        ERR_CATCH_SLICE(slices[i], (ret = hal_fw_unload(slices[i])), goto exit);
    }

    if (CHECK_HAL(start_slice, hal_slice_data_init)) {
        for (int i = 0; i < slice_count; i++) {
            ERR_CATCH_SLICE(slices[i], (ret = hal_slice_data_init(slices[i])), goto exit);
        }
    }

    ret = hal_fw_load_broadcast(slices, slice_count, fw, delay_time_us);
    if (ret != CR_OK) goto exit;

exit:
    // unlock any opened locks
    for (int i = 0; i < lock_index; i++) {
        cr_slice_unlock(slices[i]);
    }
    fclose(fw);
    return ret;
}

CredoError_t cr_firmware_loadbuf_broadcast(CredoSlice_t* slices[], int slice_count, void* buf, size_t size,
                                           unsigned delay_time_us) {
#ifndef _WIN32
    // needs to be more than 1 slice
    if (slices == NULL || slice_count < 1) return CR_INVALID_ARGS;
    CredoSlice_t* start_slice = slices[0];

    // ensure broadcast firmware load defined
    if (!CHECK_HAL(start_slice, hal_fw_load_broadcast)) return CR_INVALID_ARGS;

    FILE* fw;
    CredoError_t ret;
    fw = fmemopen(buf, size, "rb");
    if (!fw) {
        LOG_ERROR(slices[0], "[Firmware load] Error opening firmware buffer");
        return CR_FAIL;
    }
    // cannot use slice lock guard for variable length components :(
    int lock_index = 0;
    for (lock_index = 0; lock_index < slice_count; lock_index++) {
        ERR_CATCH_SLICE(slices[lock_index], (ret = cr_slice_lock(slices[lock_index])), goto exit);
    }
    /* Unload firmware */
    for (int i = 0; i < slice_count; i++) {
        ERR_CATCH_SLICE(slices[i], (ret = hal_fw_unload(slices[i])), goto exit);
    }

    if (CHECK_HAL(start_slice, hal_slice_data_init)) {
        for (int i = 0; i < slice_count; i++) {
            ERR_CATCH_SLICE(slices[i], (ret = hal_slice_data_init(slices[i])), goto exit);
        }
    }

    ret = hal_fw_load_broadcast(slices, slice_count, fw, delay_time_us);
    if (ret != CR_OK) goto exit;

exit:
    // unlock any opened locks
    for (int i = 0; i < lock_index; i++) {
        cr_slice_unlock(slices[i]);
    }
    fclose(fw);
    return ret;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

// Firmware SPI Operations

CredoError_t cri_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count) {
    CALL_HAL(slice, hal_fw_spiflash_read(slice, addr, buffer, count));
}

CredoError_t cri_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count) {
    CALL_HAL(slice, hal_fw_spiflash_write(slice, addr, buffer, count));
}

CredoError_t cri_spiflash_erase(CredoSlice_t* slice, unsigned addr) {
    CALL_HAL(slice, hal_fw_spiflash_erase(slice, addr));
}

CredoError_t cr_spiflash_load_firmware(CredoSlice_t* slice, int partition_num) {
    CALL_HAL(slice, hal_fw_load_spi(slice, partition_num));
}

CredoError_t cr_spiflash_display_mbr(CredoSlice_t* slice) {
    CALL_HAL(slice, hal_fw_spiflash_display_mbr(slice));
}

CredoError_t cr_spiflash_format_mbr(CredoSlice_t* slice, unsigned flash_kb_size, int force) {
    CALL_HAL(slice, hal_fw_spiflash_format_mbr(slice, flash_kb_size, force));
}

CredoError_t cr_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num) {
    CALL_HAL(slice, hal_fw_spiflash_read_firmware(slice, fwname, partition_num));
}

CredoError_t cr_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num, int force) {
    CALL_HAL(slice, hal_fw_spiflash_write_firmware(slice, fwname, partition_num, force));
}

CredoError_t cr_spiflash_set_partition(CredoSlice_t* slice, int partition_num) {
    CALL_HAL(slice, hal_fw_spiflash_set_partition(slice, partition_num));
}

CredoError_t cr_firmware_wait_magic_word(CredoSlice_t* slice, unsigned timeout) {
    CALL_HAL(slice, hal_fw_wait_magic_word(slice, timeout));
}

CredoError_t cr_firmware_wait_top_pll_cal(CredoSlice_t* slice, unsigned timeout) {
    CALL_HAL(slice, hal_fw_wait_top_pll_cal(slice, timeout));
}

CredoError_t cr_firmware_get_status(CredoSlice_t* slice, unsigned* status) {
    CALL_HAL(slice, hal_fw_get_status(slice, status));
}

// Firmware Information

CredoError_t cr_firmware_magic(CredoSlice_t* slice, unsigned* magic) {
    CALL_HAL(slice, hal_fw_magic(slice, magic));
}

static void stringify_firmware_version(unsigned version, char version_str[32]) {
    bool test_fw = (version >> 23) & 0x1;
    char branch = ((version >> 19) & 0xF);
    unsigned major = (version >> 16) & 0x7;
    unsigned minor = (version >> 8) & 0xFF;
    unsigned subpatch = (version >> 4) & 0xF;
    unsigned patch = (version >> 0) & 0xF;

    // make it backwards compatible with older chip firmwares
    if (branch == 0) {
        patch = (subpatch << 4) | patch;
    }

    sbs* fw_version = SBS64("v");

    if (test_fw) {
        sbscat(fw_version, "t");
    }
    sbscatprintf(fw_version, "%u.%u.%u", major, minor, patch);
    if (branch > 0) {
        sbscatprintf(fw_version, ".%c%u", 'A' + branch - 1, subpatch);
    }
    snprintf(version_str, 31, "%s", sbsstr(fw_version));
}

CredoError_t cr_firmware_version_str(CredoSlice_t* slice, char version[32]) {
    SLICE_LOCK_GUARD(slice);
    unsigned fw_version_num = 0;
    CredoError_t err = hal_fw_ver(slice, &fw_version_num);
    if (err != CR_OK) {
        SLICE_UNLOCK(slice);
        return err;
    }
    stringify_firmware_version(fw_version_num, version);
    SLICE_UNLOCK(slice);
    return CR_OK;
}

CredoError_t cr_firmware_version(CredoSlice_t* slice, unsigned* version) {
    CALL_HAL(slice, hal_fw_ver(slice, version));
}

CredoError_t cr_firmware_hash(CredoSlice_t* slice, unsigned* hash) {
    CALL_HAL(slice, hal_fw_hash(slice, hash));
}

CredoError_t cr_firmware_crc(CredoSlice_t* slice, unsigned* crc) {
    CALL_HAL(slice, hal_fw_crc(slice, crc));
}

CredoError_t cr_firmware_date(CredoSlice_t* slice, unsigned* date) {
    CALL_HAL(slice, hal_fw_date(slice, date));
}

// Firmware Commands

CredoError_t cr_firmware_cmd(CredoSlice_t* slice, unsigned cmd, unsigned param, unsigned* response,
                             unsigned* response_param) {
    CALL_HAL(slice, hal_fw_cmd(slice, cmd, param, response, response_param));
}

CredoError_t cr_firmware_cmd_ex(CredoSlice_t* slice, unsigned cmd, unsigned param1, unsigned param2, unsigned* response,
                                unsigned* response_param1, unsigned* response_param2) {
    CALL_HAL(slice, hal_fw_cmd_ex(slice, cmd, param1, param2, response, response_param1, response_param2));
}

CredoError_t cr_firmware_debug_cmd(CredoSlice_t* slice, int lane, unsigned section, unsigned index,
                                   unsigned* response_params) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_fw_debug_cmd(slice, lane, section, index, response_params));
}

CredoError_t cr_firmware_debug_cmd_ex(CredoSlice_t* slice, int lane, unsigned section, unsigned index,
                                      unsigned* response1, unsigned* response2) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_fw_debug_cmd_ex(slice, lane, section, index, response1, response2));
}

// Firmware Registers

CredoError_t cr_firmware_reg_rd(CredoSlice_t* slice, unsigned addr, unsigned* value) {
    CALL_HAL(slice, hal_fw_reg_rd(slice, addr, 0, value));
}

CredoError_t cr_firmware_reg_wr(CredoSlice_t* slice, unsigned addr, unsigned value) {
    CALL_HAL(slice, hal_fw_reg_wr(slice, addr, 0, value));
}

CredoError_t cr_firmware_reg_rd_ex(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned* value) {
    CALL_HAL(slice, hal_fw_reg_rd(slice, addr, section, value));
}

CredoError_t cr_firmware_reg_wr_ex(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned value) {
    CALL_HAL(slice, hal_fw_reg_wr(slice, addr, section, value));
}
