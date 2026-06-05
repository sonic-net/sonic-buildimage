#include "project.h"

#include "common/common_firmware.h"
#include "common/common_reset.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>

/* Firmware load and unload. These two function only take care of load/unload
 * itself; any extra step should be placed in individual HALs. */

#define FW_UNLOAD_MAGIC FW_UNLOAD_WORD

#define FW_UNLOAD_RETRY 3

CredoError_t common_fw_clear_top_pll_cal(CredoSlice_t* slice) {
#ifdef REG_FW_TOPPLL_CALDONE
    ERR_PROPS(writeReg(slice, REG_FW_TOPPLL_CALDONE, 0));
#endif
    return CR_OK;
}

CredoError_t common_fw_unload(CredoSlice_t* slice) {
    unsigned val = 0;
    unsigned long t2;
    unsigned unload_done = FW_UNLOAD_DONE;

#ifdef HAL_SUPPORT_MULTI_FW_UNLOAD_DONE
    unload_done = HAL_SUPPORT_MULTI_FW_UNLOAD_DONE(slice);
#endif

#ifdef REG_MCU_CLK_SEL
    ERR_PROPS(writeReg(slice, REG_MCU_CLK_SEL, 0));
#endif

    if (CHECK_HAL(slice, hal_fw_clear_top_pll_cal)) {
        ERR_PROPS(hal_fw_clear_top_pll_cal(slice));
    }

    for (int retry = 0; retry < FW_UNLOAD_RETRY; retry++) {
        ERR_PROPS(writeReg(slice, REG_MAGIC, FW_UNLOAD_MAGIC));

        // check FW_UNLOAD_MAGIC be written correctly
        ERR_PROPS(readReg(slice, REG_MAGIC, &val));
        if (val != FW_UNLOAD_MAGIC) {
            LOGS_WARN("[Firmware unload] FW_UNLOAD_MAGIC write fail. try again.");
            continue;
        }

        if (hal_mcu_reset(slice) != CR_OK) {
            LOGS_WARN("[Firmware unload] MCU reset fail. try again.");
            continue;
        }

        CredoTime_t start_time;
        get_time(&start_time);
        do {
            ERR_PROPS(readReg(slice, REG_MAGIC, &val));
            t2 = us_passed(&start_time);
            if (val == unload_done) break;
        } while (t2 <= slice->data->fw_unload_timeout);
        if (val == FW_UNLOAD_MAGIC) {
            ERR_PROPS(readReg(slice, REG_MAGIC, &val));
        }
        if (val == unload_done) {
            LOGS_DEBUG("[Firmware unload] Success");
            return CR_OK;
        }
        if (val == FW_UNLOAD_MAGIC) {
            LOGS_WARN("[Firmware unload] Timeout, unload magic still 0x%04x retry: %d", FW_UNLOAD_MAGIC, retry);
            continue;
        }
        /* Read got some strange value */
        LOGS_WARN("[Firmware unload] Get unexpected response 0x%04x", val);
        continue;
    }

    LOGS_ERROR("[Firmware unload] Fail");
    CredoRegVerifyConfig_t config = {.test_count = 10000};
    CredoRegVerifyStats_t stats;
    LOGS_DEBUG("[Firmware unload] failure in unloading proccess, performing register verification check");
    ERR_PROPS(cr_reg_verify(slice, &config, &stats));
    if (stats.fail_count == 0) {
        LOGS_DEBUG("Register access is clean after %.2f sec (%d WrRds)", stats.duration_sec, stats.reg_count);
    } else {
        LOGS_ERROR("Register access is NOT clean after %.2f sec (%.2f%% WrRds Failure Rate)", stats.duration_sec,
                   (100.0 * stats.fail_count) / stats.reg_count);
    }
    return CR_FAIL;
}

typedef enum {
    FW_FORMAT_1_0 = 0x0100,
    FW_FORMAT_OLD = 0xFFFF,
} FirmwareFormat_t;

#define FW_HEADER_LENGTH 0x1000

CredoError_t common_fw_parse_header(CredoSlice_t* slice, FILE* file, unsigned* start_offset) {
    unsigned char data[0x1000];
    FirmwareFormat_t fw_format;
    unsigned fw_count, fw_revision;
    unsigned default_fw_idx = 0;
    unsigned crc;
    unsigned device_id;
    unsigned hw_rev, hw_sub_rev;
    unsigned fw_start_offset, fw_sha_offset, fw_rsa_offset;

    fseek(file, 0, SEEK_SET);
    fread(data, sizeof(char), FW_HEADER_LENGTH, file);
    fw_format = (data[0] << 8) + data[1];

    if (fw_format == FW_FORMAT_OLD) {
        *start_offset = 0x1000;
        return CR_OK;
    }

    fw_count = data[2];
    default_fw_idx = data[3];
    device_id = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7];
    fw_revision = (data[8] << 24) + (data[9] << 16) + (data[10] << 8) + data[11];
    crc = (data[12] << 24) + (data[13] << 16) + (data[14] << 8) + data[15];
    LOGS_INFO(
        "[Firmware Header] Format: 0x%04x, Firmware Count: %d, "
        "Default Firmware Index: %d, Device ID: 0x%08x, Firmware Revision: 0x%08x, "
        "CRC: 0x%08x",
        fw_format, fw_count, default_fw_idx, device_id, fw_revision, crc);

    for (int i = 0; i < fw_count; i++) {
        unsigned offset = 0x20 + (0x20 * i);
        hw_rev = (data[offset + 0] << 16) + data[offset + 1];
        hw_sub_rev = (data[offset + 2] << 16) + data[offset + 3];
        fw_start_offset =
            (data[offset + 4] << 24) + (data[offset + 5] << 16) + (data[offset + 6] << 8) + data[offset + 7];
        fw_sha_offset =
            (data[offset + 8] << 24) + (data[offset + 9] << 16) + (data[offset + 10] << 8) + data[offset + 11];
        fw_rsa_offset =
            (data[offset + 12] << 24) + (data[offset + 13] << 16) + (data[offset + 14] << 8) + data[offset + 15];

        LOGS_DEBUG(
            "[Firmware Sub Header] HW Rev: 0x%04x, HW Sub Rev: 0x%04x, Start Offset: 0x%08x, "
            "SHA Offset: 0x%08x, RSA Offset: 0x%08x",
            hw_rev, hw_sub_rev, fw_start_offset, fw_sha_offset, fw_rsa_offset);

        if (i == default_fw_idx) {
            *start_offset = fw_start_offset;
        }

        if (hw_rev == slice->revision) {
            *start_offset = fw_start_offset;
            break;
        }
    }

    return CR_OK;
}

#define FILE_BUFFER_MAX 32

/* Load firmware file into chip. It doesn't unload! */
CredoError_t common_fw_load(CredoSlice_t* slice, FILE* file) {
    // no delay time and not in broadcast mode
    ERR_CATCH(common_fw_load_inner(slice, file, 0, 0), goto failed_firmware_load);

    FirmwareHeader_t fw_header;
    unsigned image_start_offset = 0, fw_length = 0;
    ERR_PROPS(common_fw_parse_header(slice, file, &image_start_offset));

    fseek(file, image_start_offset, SEEK_SET);
    fread(&fw_header, sizeof(char), sizeof(fw_header), file);

    fw_length = common_fw_firmware_length(&fw_header);

    // in non-broadcast ensure the firmware came up
    ERR_CATCH(hal_fw_wait_magic_word(slice, FW_MAGIC_WORD_TIMEOUT + fw_length * FW_DOWNLOAD_MARGIN_UNIT),
              goto failed_firmware_load);

    ERR_CATCH(common_fw_wait_ready(slice), goto failed_firmware_load);

    unsigned fw_ver = 0;
    char fw_ver_str[32];
    if (CHECK_HAL(slice, hal_fw_ver)) {
        ERR_PROPS(hal_fw_ver(slice, &fw_ver));
    }
    stringify_firmware_version(fw_ver, fw_ver_str);

    unsigned fw_hash = 0, fw_crc = 0, fw_date = 0;
    ERR_PROPS(hal_fw_hash(slice, &fw_hash));
    ERR_PROPS(hal_fw_crc(slice, &fw_crc));
    ERR_PROPS(hal_fw_date(slice, &fw_date));

    LOGS_INFO("[Firmware load] Version: %s, Date Code: 0x%04x, Hash Code: 0x%06x, CRC Code: 0x%04x", fw_ver_str,
              fw_date, fw_hash, fw_crc);

    return CR_OK;
failed_firmware_load : {
    CredoRegVerifyConfig_t config = {.test_count = 10000};
    CredoRegVerifyStats_t stats;
    LOGS_DEBUG("[Firmware load] failure in loading proccess, performing register verification check");
    ERR_PROPS(cr_reg_verify(slice, &config, &stats));
    if (stats.fail_count == 0) {
        LOGS_DEBUG("Register access is clean after %.2f sec (%d WrRds)", stats.duration_sec, stats.reg_count);
    } else {
        LOGS_ERROR("Register access is NOT clean after %.2f sec (%.2f%% WrRds Failure Rate)", stats.duration_sec,
                   (100.0 * stats.fail_count) / stats.reg_count);
    }
    return CR_PHY_FW_DOWNLOAD_FAIL;
}
}

CredoError_t common_fw_load_broadcast(CredoSlice_t* slices[], int slice_count, FILE* file, unsigned delay_time_us) {
    // put into broadcast mode
    for (int i = 0; i < slice_count; i++) {
        ERR_PROP_LOG(
            writeTop(slices[i], REG_MDIO_MAGIC_NUMER, 0x8888),
            LOG_ERROR(slices[i],
                      "[Firmware load] Failed to put slice %d into broadcast mode due to register access failure",
                      slices[i]->slice_id));
    }
    CredoError_t error_code = CR_OK;

    // log firmware loading broadcast
    for (int i = 0; i < slice_count; i++) {
        LOG_DEBUG(slices[i], "[Firmware load] Firmware loading broadcast slice id %d", slices[i]->slice_id);
    }
    error_code = common_fw_load_inner(slices[0], file, delay_time_us, 1);

    // bring out of broadcast mode

    ERR_PROP_LOG(
        cr_slice_broadcast_write(slices[0], REG_MDIO_MAGIC_NUMER, 0x0),
        LOG_ERROR(slices[0],
                  "[Firmware load] Failed to put slices out of broadcast mode due to register access failure"));
    // if firmware load failed now exit, to allow slices to be brought out of broadcast mode
    ERR_CATCH_SLICE(slices[0], error_code, goto failed_firmware_load);

    // check that firmware came up successfully
    for (int i = 0; i < slice_count; i++) {
        // use oversized unit for wait time as length isnt known
        if (hal_fw_wait_magic_word(slices[i], FW_MAGIC_WORD_TIMEOUT + 100000 * FW_DOWNLOAD_MARGIN_UNIT) != CR_OK) {
            error_code = CR_FAIL;  // allow to check all slices to see which failed
            LOG_ERROR(slices[i], "[Firmware load] broadcast failed (magic word)");
            continue;
        }

        if (common_fw_wait_ready(slices[i]) != CR_OK) {
            error_code = CR_FAIL;
            LOG_ERROR(slices[i], "[Firmware load] broadcast failed (wait ready)");
            continue;
        }

        unsigned fw_ver = 0, fw_date = 0, fw_hash = 0, fw_crc = 0;
        if (CHECK_HAL(slices[i], hal_fw_ver)) ERR_PROP_SLICE(slices[i], hal_fw_ver(slices[i], &fw_ver));
        if (CHECK_HAL(slices[i], hal_fw_date)) ERR_PROP_SLICE(slices[i], hal_fw_date(slices[i], &fw_date));
        if (CHECK_HAL(slices[i], hal_fw_hash)) ERR_PROP_SLICE(slices[i], hal_fw_hash(slices[i], &fw_hash));
        if (CHECK_HAL(slices[i], hal_fw_crc)) ERR_PROP_SLICE(slices[i], hal_fw_crc(slices[i], &fw_crc));
        char fw_ver_str[32] = {0};
        stringify_firmware_version(fw_ver, fw_ver_str);
        LOG_INFO(slices[i], "[Firmware load] Version: %s, Date Code: 0x%04x, Hash Code: 0x%06x, CRC Code: 0x%04x",
                 fw_ver_str, fw_date, fw_hash, fw_crc);
    }

    if (error_code == CR_OK) {
        return CR_OK;
    }
    // if any slice failed then the load failed
failed_firmware_load : {
    for (int i = 0; i < slice_count; i++) {
        CredoRegVerifyConfig_t config = {.test_count = 10000};
        CredoRegVerifyStats_t stats;
        LOG_DEBUG(slices[i], "[Firmware load] failure in loading proccess, performing register verification check");
        ERR_PROP_SLICE(slices[i], cr_reg_verify(slices[i], &config, &stats));
        if (stats.fail_count == 0) {
            LOG_DEBUG(slices[i], "Register access is clean after %.2f sec (%d WrRds)", stats.duration_sec,
                      stats.reg_count);
        } else {
            LOG_ERROR(slices[i], "Register access is NOT clean after %.2f sec (%.2f%% WrRds Failure Rate)",
                      stats.duration_sec, (100.0 * stats.fail_count) / stats.reg_count);
        }
        LOG_DEBUG(slices[i], "[Firmware load] Firmware loading broadcast slice id %d", slices[i]->slice_id);
    }
}
    return error_code;
}

CredoError_t common_fw_load_spi(CredoSlice_t* slice, int partition_num) {
#if defined(FW_LOAD_FROM_SPI) && defined(FW_LOADING_SPI)
    unsigned val;
    unsigned long t2;
    unsigned unload_done = FW_UNLOAD_DONE;

    if (partition_num != -1) {
        LOGS_WARN("Ignore unsed partition_num %d", partition_num);
    }

#ifdef HAL_SUPPORT_MULTI_FW_UNLOAD_DONE
    unload_done = HAL_SUPPORT_MULTI_FW_UNLOAD_DONE(slice);
#endif

#ifdef REG_MCU_CLK_SEL
    ERR_PROPS(writeReg(slice, REG_MCU_CLK_SEL, 0));
#endif

    bool is_pass_1sec_hint = false;
    for (int retry = 0; retry < FW_UNLOAD_RETRY; retry++) {
        ERR_PROPS(writeReg(slice, REG_MAGIC, FW_LOAD_FROM_SPI));
        ERR_PROP(common_mcu_reset(slice));

        CredoTime_t start_time;
        get_time(&start_time);
        do {
            ERR_PROPS(readReg(slice, REG_MAGIC, &val));
            t2 = us_passed(&start_time);
            if (is_fw_load_magic(val)) break;
            if ((!is_pass_1sec_hint) && (t2 > 1000 * 1000)) {
                is_pass_1sec_hint = true;
                LOGS_WARN("[Firmware load] Try %d times per %d ms timeout", FW_UNLOAD_RETRY,
                          slice->data->fw_spiflash_load_timeout / 1000);
            }
        } while (t2 <= slice->data->fw_spiflash_load_timeout);
        if ((val == FW_LOAD_FROM_SPI) || (val == FW_LOADING_SPI)) {
            ERR_PROPS(readReg(slice, REG_MAGIC, &val));
        }
        if (is_fw_load_magic(val)) {
            t2 = us_passed(&start_time);
            LOGS_INFO("[Firmware load] Succeed to load firmware from SPI flash in %.3f secs", t2 / 1.0e6);
            ERR_PROPS(common_fw_wait_ready(slice));
            unsigned fw_date = 0xFFFF, fw_hash = 0xFFFFFF, fw_crc = 0xFFFF;
            hal_fw_date(slice, &fw_date);
            hal_fw_hash(slice, &fw_hash);
            hal_fw_crc(slice, &fw_crc);
            unsigned fw_ver = 0;
            char fw_ver_str[32] = "Unknown";
            if (hal_fw_ver(slice, &fw_ver) == CR_OK) {
                stringify_firmware_version(fw_ver, fw_ver_str);
            }
            LOGS_INFO("[Firmware Info] Version: %s, Date Code: 0x%04x, Hash Code: 0x%06x, CRC Code: 0x%04x", fw_ver_str,
                      fw_date, fw_hash, fw_crc);
            return CR_OK;
        }
        if (val == unload_done) {
            LOGS_WARN("fail to load firmware from SPI flash retry: %d", retry);
            continue;
        }
        if ((val == FW_LOAD_FROM_SPI) || (val == FW_LOADING_SPI)) {
            LOGS_WARN("Unload and load timeout retry: %d", retry);
            continue;
        }
        /* Read got some strange value */
        LOGS_WARN("Get unexpected response 0x%04x", val);
        continue;
    }

    LOGS_ERROR("fail to load firmware from SPI flash");
    return CR_FAIL;
#else
    return CR_NOTIMPLEMENTED_HAL;
#endif
}

// utility that handles doing firmware load for either broadcast or regular
CredoError_t common_fw_load_inner(CredoSlice_t* slice, FILE* file, unsigned delay_time_us, int broadcast) {
    unsigned char buf[FILE_BUFFER_MAX];
    unsigned shift = 20;
    unsigned char* data = buf;
    const char* file_type_string;
    unsigned file_hash_code, file_crc_code, file_date_code, entryPoint, length, ramAddr;
    FirmwareType_t file_type;
    unsigned image_start_offset = 0;
    unsigned uncompressAddr = 0;

    unsigned device_buffer[16];
    unsigned reg_data_base = REG_DATA;

    ERR_PROPS(common_fw_parse_header(slice, file, &image_start_offset));

    if (fseek(file, image_start_offset, SEEK_SET)) {
        LOGS_ERROR("fseek fail!");
        return CR_FAIL;
    }
    fread(buf, sizeof(char), FILE_BUFFER_MAX, file);

    file_type = data[0];
    file_hash_code = (data[1] << 16) + (data[2] << 8) + data[3];
    file_crc_code = (data[4] << 8) + data[5];
    file_date_code = (data[6] << 8) + data[7];
    entryPoint = (data[8] << 24) + (data[9] << 16) + (data[10] << 8) + data[11];
    length = (data[12] << 24) + (data[13] << 16) + (data[14] << 8) + data[15];
    ramAddr = (data[16] << 24) + (data[17] << 16) + (data[18] << 8) + data[19];

    switch (file_type) {
        case UNCOMPRESSED:
            file_type_string = "uncompressed";
            break;
        case COMPRESSED1:
            file_type_string = "compressed 1";
            break;
        case COMPRESSED2:
            file_type_string = "compressed 2";
            uncompressAddr = (data[20] << 24) + (data[21] << 16) + (data[22] << 8) + data[23];
            shift = 24;
            break;
        default:
            LOGS_ERROR("[Firmware load] Unrecognized image type %d", file_type);
            goto error_exit;
    }
    LOGS_DEBUG("[Firmware load] File Type: %s, Entry: 0x%08x, Length: 0x%08x, RAM: 0x%08x", file_type_string,
               entryPoint, length, ramAddr);

    /* timer */
    CredoTime_t start_time, loop_start_time;
    get_time(&start_time);

    // ##### Main FW Download loop
    unsigned data_len = 0;
    if (fseek(file, image_start_offset + shift, SEEK_SET)) {
        LOGS_ERROR("[Firmware load] Failed shifting to firmware data start");
        goto error_exit;
    }

    unsigned cmd, count, checkSum, frame_index = 0;
    while (data_len < length) {
        if (data_len == 0) {
            /* First frame, write address */
            cmd = 0x800C;
            count = 12;
            device_buffer[12] = ramAddr >> 16;
            device_buffer[13] = ramAddr & 0xFFFF;
            checkSum = (ramAddr >> 16) + (ramAddr & 0xFFFF);
        } else {
            /* Skip address for the rest of load */
            cmd = 0x100E;
            count = 14;
            checkSum = 0;
        }
        checkSum += cmd;
        size_t read_count = fread(buf, sizeof(char), count * 2, file);
        if (read_count == 0) {
            LOGS_ERROR("[Firmware load] Unable to read firmware file index %d", data_len);
            goto error_exit;
        }
        for (unsigned j = 0; j < count; j++) {
            unsigned mdioData = (data_len >= length) ? 0x0000 : ((buf[2 * j] << 8) + buf[2 * j + 1]);
            device_buffer[j] = mdioData;
            checkSum += mdioData;
            data_len += 2;
            // ramAddr += 2;
        }
        device_buffer[14] = (~checkSum + 1) & 0xFFFF;
        device_buffer[15] = cmd;

        if (broadcast) {
            ERR_CATCH(cr_slice_broadcast_burst_write(slice, reg_data_base, device_buffer, 16), goto error_exit);
            // delay after writing frame
            sleep_us(delay_time_us);
        } else {
            ERR_CATCH(cr_slice_burst_write(slice, reg_data_base, device_buffer, 16), goto error_exit);

            unsigned status = 0;
            do {
                ERR_CATCH(readTop(slice, reg_data_base + 15, &status), goto error_exit);
                if (status != cmd) break;
            } while (!is_timeout(&loop_start_time, FW_LOAD_FRAME_TIMEOUT));
            if (status == cmd) {
                ERR_CATCH(readTop(slice, reg_data_base + 15, &status), goto error_exit);
            }
            if (status == cmd) {
                LOGS_ERROR("[Firmware load] Frame %u timeout!", frame_index);
                goto error_exit;
            } else if (status != 0) {
                LOGS_ERROR("[Firmware load] Frame %u returns 0x%04x!, check register access integrity", frame_index,
                           status);
                goto error_exit;
            }
            frame_index += 1;
        }
        get_time(&loop_start_time);
    }
    /* End of Main FW Download loop */
    unsigned start = 12;
    device_buffer[12] = entryPoint >> 16;
    device_buffer[13] = entryPoint & 0xFFFF;
    checkSum = (entryPoint >> 16) + (entryPoint & 0xFFFF) + 0x4000;
    if (file_type == COMPRESSED2) {
        device_buffer[10] = ramAddr >> 16;
        device_buffer[11] = ramAddr & 0xFFFF;
        checkSum += (ramAddr >> 16) + (ramAddr & 0xFFFF);
        device_buffer[8] = uncompressAddr >> 16;
        device_buffer[9] = uncompressAddr & 0xFFFF;
        checkSum += (uncompressAddr >> 16) + (uncompressAddr & 0xFFFF);
        cmd = 0x4100;
        start = 8;
    } else {
        cmd = 0x4000;
    }
    device_buffer[14] = (~checkSum + 1) & 0xffff;
    device_buffer[15] = cmd;

    if (broadcast) {
        ERR_CATCH(cr_slice_broadcast_burst_write(slice, reg_data_base + start, device_buffer + start, 16 - start),
                  goto error_exit);
    } else {
        ERR_CATCH(cr_slice_burst_write(slice, reg_data_base + start, device_buffer + start, 16 - start),
                  goto error_exit);
    }

    /* timer */
    unsigned long timelapse = us_passed(&start_time);
    LOGS_DEBUG("[Firmware load] Image downloaded using %.3f secs", timelapse / 1.0e6);

    return CR_OK;

error_exit:
    LOGS_INFO("[Firmware load] Fail, Date Code: 0x%04x, Hash Code: 0x%06x, CRC Code: 0x%04x", file_date_code,
              file_hash_code, file_crc_code);
    return CR_FAIL;
}

CredoError_t common_fw_wait_magic_word(CredoSlice_t* slice, unsigned timeout) {
    unsigned magic_word;
    CredoTime_t start_time;
    get_time(&start_time);
    do {
        ERR_PROPS(readReg(slice, REG_MAGIC, &magic_word));
        if (is_fw_load_magic(magic_word)) break;
        sleep_ms(1);  // decrease CPU loading in while loop
    } while (!is_timeout(&start_time, timeout));

    if (!is_fw_load_magic(magic_word)) {
        // read again to confirm not magic word still
        ERR_PROPS(readReg(slice, REG_MAGIC, &magic_word));
        unsigned long timelapse = us_passed(&start_time);
        if (!is_fw_load_magic(magic_word)) {
            log_if_invalid_fw_load_magic(slice, magic_word);
            LOGS_ERROR(
                "Timeout magic_word after %lu usec, timeout is %u. Firmware did not start either to register access "
                "integrity or power integrity issues",
                timelapse, timeout);
            return CR_FAIL;
        }
    }
    return CR_OK;
}

static CredoError_t common_fw_wait_auto_cfg_done(CredoSlice_t* slice) {
#ifdef REG_FW_ACFG_DONE
#define FW_ACFG_TIMEOUT 1000000
    unsigned val;
    CredoTime_t start_time;

    get_time(&start_time);
    do {
        ERR_PROPS(readReg(slice, REG_FW_ACFG_DONE, &val));
        if (val == 0) break;
    } while (!is_timeout(&start_time, FW_ACFG_TIMEOUT));

    ERR_PROPS(readReg(slice, REG_FW_ACFG_DONE, &val));
    if (val == 1) {
        LOGS_ERROR("[Firmware] Wait FW auto cfg %d usecs timeout", FW_ACFG_TIMEOUT);
        return CR_FW_TIMEOUT;
    }
    unsigned long timelapse = us_passed(&start_time);
    LOGS_DEBUG("[Firmware] FW auto_cfg flag done after %lu usecs", timelapse);
#endif
    return CR_OK;
}

CredoError_t common_fw_wait_ready(CredoSlice_t* slice) {
    CredoError_t ret = CR_OK;
    unsigned fw_rdy = 0;
    CredoTime_t start_time;

    if (CHECK_HAL(slice, hal_fw_wait_top_pll_cal)) {
        ERR_PROPS(hal_fw_wait_top_pll_cal(slice, slice->data->top_cal_timeout));
    }
    ERR_PROPS(common_fw_wait_auto_cfg_done(slice));

    get_time(&start_time);
    do {
        ERR_PROPS(hal_fw_get_status(slice, &fw_rdy));
        if (fw_rdy == 1) break;
        sleep_ms(1);  // decrease CPU loading in while loop
    } while (!is_timeout(&start_time, FW_READY_TIMEOUT));

    if (fw_rdy == 0) {
        // read again to confirm not fw ready still
        ERR_PROPS(hal_fw_get_status(slice, &fw_rdy));
        if (fw_rdy == 0) {
            LOGS_ERROR("[Firmware] Wait FW ready %d usecs timeout", FW_READY_TIMEOUT);
            return CR_FW_TIMEOUT;
        }
    }

    ret = hal_fw_ready_post_actions(slice);
    if (ret != CR_OK && ret != CR_NOTIMPLEMENTED_HAL) return ret;

    unsigned long timelapse = us_passed(&start_time);
    LOGS_DEBUG("[Firmware] FW ready after %lu usecs", timelapse);

    return CR_OK;
}

CredoError_t common_fw_wait_top_pll_cal(CredoSlice_t* slice, unsigned timeout) {
#if HAL_SUPPORT_TOP_PLL_CAL
    unsigned pll_cal_done;
    unsigned long timelapse;
    CredoTime_t start_time;

    get_time(&start_time);
    do {
        ERR_PROPS(readReg(slice, REG_FW_TOPPLL_CALDONE, &pll_cal_done));
        if (pll_cal_done) break;
        sleep_ms(1);  // decrease CPU loading in while loop
    } while (!is_timeout(&start_time, timeout));

    if (pll_cal_done == 0) {
        // read again to confirm not pll_cal_done still
        ERR_PROPS(readReg(slice, REG_FW_TOPPLL_CALDONE, &pll_cal_done));
        if (pll_cal_done == 0) {
            LOGS_ERROR(
                "[Firmware] TOP PLL calibration timeout %u usecs. Either a poor reference clock or firmware is not "
                "properly loaded due to register access failure.",
                timeout);
            return CR_FW_TIMEOUT;
        }
    }

    timelapse = us_passed(&start_time);
    LOGS_DEBUG("[Firmware] TOP PLL calibration done after %lu usecs", timelapse);
#endif
    return CR_OK;
}

FirmwareType_t common_fw_firmware_type(void* buffer) {
    unsigned char* data = (unsigned char*)buffer;
    return data[0];
}

unsigned common_fw_firmware_length(void* buffer) {
    unsigned char* data = (unsigned char*)buffer;
    return (data[12] << 24) + (data[13] << 16) + (data[14] << 8) + data[15];
}

CredoError_t common_fw_header_display(CredoSlice_t* slice, void* buffer) {
    CredoError_t ret = CR_OK;
    unsigned char* data = (unsigned char*)buffer;
    unsigned shift = shift = 20;  // assign self to avoid unused variable
    const char* file_type_string = "";
    unsigned file_hash_code, file_crc_code, file_date_code, entryPoint, length, ramAddr;
    FirmwareType_t file_type;
    unsigned uncompressAddr = uncompressAddr = 0;  // assign self to avoid unused variable

    file_type = common_fw_firmware_type(data);
    file_hash_code = (data[1] << 16) + (data[2] << 8) + data[3];
    file_crc_code = (data[4] << 8) + data[5];
    file_date_code = (data[6] << 8) + data[7];
    entryPoint = (data[8] << 24) + (data[9] << 16) + (data[10] << 8) + data[11];
    length = common_fw_firmware_length(data);
    ramAddr = (data[16] << 24) + (data[17] << 16) + (data[18] << 8) + data[19];

    switch (file_type) {
        case UNCOMPRESSED:
            file_type_string = "uncompressed";
            break;
        case COMPRESSED1:
            file_type_string = "compressed 1";
            break;
        case COMPRESSED2:
            file_type_string = "compressed 2";
            uncompressAddr = (data[20] << 24) + (data[21] << 16) + (data[22] << 8) + data[23];
            shift = 24;
            break;
        default:
            LOGS_ERROR("[Firmware Info] Unrecognized image type %d", file_type);
            ret = CR_FAIL;
    }
    LOGS_DEBUG("[Firmware Info] File Type: %s, Entry: 0x%08x, Length: 0x%08x, RAM: 0x%08x", file_type_string,
               entryPoint, length, ramAddr);

    LOGS_INFO("[Firmware Info] Date Code: 0x%04x, Hash Code: 0x%06x, CRC Code: 0x%04x", file_date_code, file_hash_code,
              file_crc_code);

    return ret;
}
