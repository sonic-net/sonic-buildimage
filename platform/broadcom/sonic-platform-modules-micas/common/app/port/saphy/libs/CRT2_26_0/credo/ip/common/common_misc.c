#include "project.h"

#include "common/common_firmware.h"
#include "common/common_init.h"
#include "common/common_misc.h"
#include "sdk/driver_regs.h"

#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <string.h>

CredoError_t common_set_lane_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config) {
    if (lane_config == NULL) return CR_INVALID_ARGS;
    hal_set_rx_polarity(slice, lane, lane_config->rx_polarity_swap);
    hal_set_tx_polarity(slice, lane, lane_config->tx_polarity_swap);
    hal_set_rx_input_mode(slice, lane, (lane_config->using_dc_input) == 0 ? CR_COUPLING_AC : CR_COUPLING_DC);
    return CR_OK;
}

CredoError_t common_get_lane_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config) {
    if (lane_config == NULL) return CR_INVALID_ARGS;

    CredoLaneCoupling_t input_mode = CR_COUPLING_DC;
    int tx_pol = 0, rx_pol = 0;
    hal_get_rx_polarity(slice, lane, &rx_pol);
    hal_get_tx_polarity(slice, lane, &tx_pol);
    hal_get_rx_input_mode(slice, lane, &input_mode);

    lane_config->tx_polarity_swap = tx_pol;
    lane_config->rx_polarity_swap = rx_pol;
    lane_config->using_dc_input = (input_mode == CR_COUPLING_DC) ? 1 : 0;
    return CR_OK;
}

CredoError_t common_slice_get_oui(CredoSlice_t* slice, unsigned* oui) {
#ifdef REG_OUI_LSB
    unsigned lsb = 0, msb = 0;

    ERR_PROP(readReg(slice, REG_OUI_LSB, &lsb));
    ERR_PROP(readReg(slice, REG_OUI_MSB, &msb));

    *oui = (msb << 16) | lsb;
    return CR_OK;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

CredoError_t common_slice_get_model_number(CredoSlice_t* slice, unsigned* model_number) {
#ifdef REG_MODEL_NUMBER
    ERR_PROP(readReg(slice, REG_MODEL_NUMBER, model_number));
    return CR_OK;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

CredoError_t common_slice_get_revision_number(CredoSlice_t* slice, unsigned* revision_number) {
#ifdef REG_REVISION_NUMBER
    ERR_PROP(readReg(slice, REG_REVISION_NUMBER, revision_number));
    return CR_OK;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

static CredoError_t common_slice_load_setup_str(CredoSlice_t* slice, const char* data) {
    CredoError_t ret = CR_OK;
    unsigned count = 0;
    int mismatch = 0;

    const char* buffer = data;
    while (buffer != NULL) {
        if (buffer[0] == '\0') break;
        if (buffer[0] == '#' || buffer[0] == '\n') goto next_line;  // treat # as comments
        unsigned address = 0, value = 0;
        unsigned msb = 0, lsb = 0;
        if (sscanf(buffer, "%04X.%u:%u %04X", &address, &msb, &lsb, &value) == 4) {
            unsigned old_val = 0;
            // swap if in opposite order
            if (lsb > msb) {
                unsigned tmp = msb;
                msb = lsb;
                lsb = tmp;
            }
            ERR_CATCH((ret = readTop(slice, address, &old_val)), goto exit);
            unsigned mask = (((uint64_t)1 << ((msb) - (lsb) + 1)) - 1);
            old_val &= ~(mask << lsb);
            old_val |= (value & mask) << lsb;
            ERR_CATCH((ret = writeTop(slice, address, old_val)), goto exit);
        } else if (sscanf(buffer, "%04X %04X", &address, &value) == 2) {
            ERR_CATCH((ret = writeTop(slice, address, value)), goto exit);
        } else {
            mismatch++;
            goto next_line;
        }
        count++;
    next_line:
        buffer = strchr(buffer, '\n');
        if (buffer != NULL) {
            buffer += 1;
        }
    }

exit:
    if (mismatch) LOGS_ERROR("mismatch %d\n", mismatch);
    LOGS_INFO("%d registers were written from string", count);
    return ret;
}

CredoError_t common_slice_load_setup(CredoSlice_t* slice, const char* file_path) {
    // heruistic user didnt provide a file but a buffer
    if (strchr(file_path, '\n') != NULL) {
        return common_slice_load_setup_str(slice, file_path);
    }

    FILE* fp = fopen(file_path, "r");
    CredoError_t ret = CR_OK;
    unsigned count = 0;
    int mismatch = 0;

    if (fp == NULL) {
        LOGS_ERROR("file %s open fail, %s\n", file_path, strerror(errno));
        return CR_FAIL;
    }

    while (!feof(fp)) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            if (buffer[0] == '#' || buffer[0] == '\n') continue;  // treat # as comments
            unsigned address = 0, value = 0;
            unsigned msb = 0, lsb = 0;
            if (sscanf(buffer, "%04X.%u:%u %04X", &address, &msb, &lsb, &value) == 4) {
                unsigned old_val = 0;
                // swap if in opposite order
                if (lsb > msb) {
                    unsigned tmp = msb;
                    msb = lsb;
                    lsb = tmp;
                }
                ERR_CATCH((ret = readTop(slice, address, &old_val)), goto exit);
                unsigned mask = (((uint64_t)1 << ((msb) - (lsb) + 1)) - 1);
                old_val &= ~(mask << lsb);
                old_val |= (value & mask) << lsb;
                ERR_CATCH((ret = writeTop(slice, address, old_val)), goto exit);
            } else if (sscanf(buffer, "%04X %04X", &address, &value) == 2) {
                ERR_CATCH((ret = writeTop(slice, address, value)), goto exit);
            } else {
                mismatch++;
                continue;
            }
            count++;
        }
    }

exit:
    if (mismatch) LOGS_ERROR("mismatch %d\n", mismatch);
    LOGS_INFO("%d registers were written from file %s", count, file_path);
    if (fp) fclose(fp);
    return ret;
}

CredoError_t common_slice_get_reghive(CredoSlice_t* slice, const char* hivename, const RegHive_t** reghive) {
    extern const RegHive_t* reghive_array[];

    if (hivename && strlen(hivename) > 0) {
        for (int i = 0; reghive_array[i] != NULL; i++) {
            unsigned hive_index = 0;
            const RegHive_t* hive = reghive_array[i];
            hal_get_hive_index(slice, &hive_index);
            hive += hive_index;

            if (!strcmp(hivename, hive->hivename)) {
                *reghive = hive;
                return CR_OK;
            }
        }
        return CR_FAIL;
    }
    return CR_OK;
}

CredoError_t common_slice_get_reghive_count(CredoSlice_t* slice, unsigned* count) {
    extern const RegHive_t* reghive_array[];
    *count = 0;
    for (unsigned i = 0; reghive_array[i] != NULL; i++) {
        *count += 1;
    }
    return CR_OK;
}

CredoError_t common_slice_index_reghive(CredoSlice_t* slice, int index, const RegHive_t** reghive) {
    extern const RegHive_t* reghive_array[];
    for (int i = 0; reghive_array[i] != NULL; i++) {
        if (index != i) {
            continue;
        }
        unsigned hive_index = 0;
        const RegHive_t* hive = reghive_array[i];
        hal_get_hive_index(slice, &hive_index);
        hive += hive_index;
        *reghive = hive;
        return CR_OK;
    }
    return CR_FAIL;
}

CredoError_t common_slice_get_sram_status(CredoSlice_t* slice, CredoSramStatus_t* sram_status) {
#define ECC_STATUS_CORR (1 << 15)
#define ECC_STATUS_ERR  (1 << 14)

    unsigned value = 0;
    ERR_PROP(readReg(slice, REG_ECC_STATUS, &value));
    if (value & ECC_STATUS_ERR)
        *sram_status = CR_SRAM_UNCORR_ERROR;
    else if (value & ECC_STATUS_CORR)
        *sram_status = CR_SRAM_CORR_ERROR;
    else
        *sram_status = CR_SRAM_NO_ERROR;
    return CR_OK;
}

CredoError_t common_slice_generate_sram_error(CredoSlice_t* slice, CredoSramStatus_t sram_status) {
    unsigned test_reg = 0;
    switch (sram_status) {
        case CR_SRAM_CORR_ERROR:
            test_reg = 1;
            break;
        case CR_SRAM_UNCORR_ERROR:
            test_reg = 3;
            break;
        default:
            return CR_OK;
    }

    ERR_PROP(writeReg(slice, REG_ECC_TEST, test_reg));
    sleep_us(1);
    ERR_PROP(writeReg(slice, REG_ECC_TEST, 0x0));
    return CR_OK;
}

CredoError_t common_option_get_fw_freeze(CredoSlice_t* slice, const char* name, int* value) {
    unsigned cmd = FW_CMD_INTERNAL_REG_READ;
    unsigned r, param = FW_CMD_LOG_SILENT;
    hal_fw_cmd(slice, cmd, 0, &r, &param);
    *value = ((r & FW_RESPONSE_CODE_MASK) == ERROR_FW_FREEZE) ? 1 : 0;
    return CR_OK;
}

CredoError_t common_option_set_fw_freeze(CredoSlice_t* slice, const char* name, int value) {
#if defined(FW_CMD_FREEZE) && defined(FW_CMD_UNFREEZE)
    unsigned cmd = (value == 0) ? FW_CMD_UNFREEZE : FW_CMD_FREEZE;
    ERR_PROP(common_fw_cmd_send(slice, cmd, 0x0));
#else
    LOGS_INFO("Firmware don't support or not implemented.");
#endif
    return CR_OK;
}

CredoError_t common_option_get_fw_cmd_timeout(CredoSlice_t* slice, const char* name, int* value) {
    *value = (int)slice->data->fw_cmd_timeout;
    return CR_OK;
}

CredoError_t common_option_set_fw_cmd_timeout(CredoSlice_t* slice, const char* name, int value) {
    slice->data->fw_cmd_timeout = (value == 0) ? FW_CMD_TIMEOUT : (unsigned)value;
    return CR_OK;
}

CredoError_t common_option_get_fw_unload_timeout(CredoSlice_t* slice, const char* name, int* value) {
    *value = (int)slice->data->fw_unload_timeout;
    return CR_OK;
}

CredoError_t common_option_set_fw_unload_timeout(CredoSlice_t* slice, const char* name, int value) {
    slice->data->fw_unload_timeout = (value == 0) ? FW_UNLOAD_TIMEOUT : (unsigned)value;
    return CR_OK;
}

CredoError_t common_get_fw_cmd_timeout(CredoSlice_t* slice, int dummy, int* value) {
    *value = (int)slice->data->fw_cmd_timeout;
    return CR_OK;
}

CredoError_t common_set_fw_cmd_timeout(CredoSlice_t* slice, int dummy, int value) {
    slice->data->fw_cmd_timeout = (value == 0) ? FW_CMD_TIMEOUT : (unsigned)value;
    return CR_OK;
}

CredoError_t common_option_get_top_cal_timeout(CredoSlice_t* slice, const char* name, int* value) {
    *value = (int)slice->data->top_cal_timeout;
    return CR_OK;
}

CredoError_t common_option_set_top_cal_timeout(CredoSlice_t* slice, const char* name, int value) {
    slice->data->top_cal_timeout = (value == 0) ? TOP_CAL_TIMEOUT : (unsigned)value;
    return CR_OK;
}

CredoError_t common_get_top_cal_timeout(CredoSlice_t* slice, int dummy, int* value) {
    return common_option_get_top_cal_timeout(slice, NULL, value);
}

CredoError_t common_set_top_cal_timeout(CredoSlice_t* slice, int dummy, int value) {
    return common_option_set_top_cal_timeout(slice, NULL, value);
}

CredoError_t common_option_get_fw_spiflash_load_timeout(CredoSlice_t* slice, const char* name, int* value) {
    *value = (int)slice->data->fw_spiflash_load_timeout;
    return CR_OK;
}

CredoError_t common_option_set_fw_spiflash_load_timeout(CredoSlice_t* slice, const char* name, int value) {
    slice->data->fw_spiflash_load_timeout = (value == 0) ? FW_LOAD_SPI_TIMEOUT : (unsigned)value;
    return CR_OK;
}

CredoError_t common_get_fw_spiflash_load_timeout(CredoSlice_t* slice, int dummy, int* value) {
    return common_option_get_fw_spiflash_load_timeout(slice, NULL, value);
}

CredoError_t common_set_fw_spiflash_load_timeout(CredoSlice_t* slice, int dummy, int value) {
    return common_option_set_fw_spiflash_load_timeout(slice, NULL, value);
}

const char* common_addr_stringify(CredoSlice_t* slice, int address) {
#define RETURN_TARGET_STRING(target) return " (" #target ")"
#define RETURN_TARGET_STRING_IF_ADDR_MATCH(prefix, target)                                   \
    do {                                                                                     \
        if (address == addrReg(slice, GLUE__(prefix, target))) RETURN_TARGET_STRING(target); \
    } while (0)
#ifdef REG_CMD
    RETURN_TARGET_STRING_IF_ADDR_MATCH(REG_, CMD);
#endif
#ifdef REG_CMD_DETAIL
    RETURN_TARGET_STRING_IF_ADDR_MATCH(REG_CMD_, DETAIL);
#endif
#ifdef REG_CMD_DETAIL2
    RETURN_TARGET_STRING_IF_ADDR_MATCH(REG_CMD_, DETAIL2);
#endif
#ifdef REG_CMD_DETAIL3
    RETURN_TARGET_STRING_IF_ADDR_MATCH(REG_CMD_, DETAIL3);
#endif
#ifdef REG_MAGIC
    RETURN_TARGET_STRING_IF_ADDR_MATCH(REG_, MAGIC);
#endif

#define RETURN_TARGET_STRING_IF_ADDR_RANGE_MATCH(prefix, target, count)                          \
    do {                                                                                         \
        if ((GLUE__(prefix, target) <= address) && (address < (GLUE__(prefix, target) + count))) \
            RETURN_TARGET_STRING(target);                                                        \
    } while (0)
#ifdef REG_DATA
    RETURN_TARGET_STRING_IF_ADDR_RANGE_MATCH(REG_, DATA, 16);
#endif

    return NULL;
}

CredoError_t common_slice_get_slice_id(CredoSlice_t* slice, int dummy, int* value) {
    *value = slice->slice_id;
    return CR_OK;
}
