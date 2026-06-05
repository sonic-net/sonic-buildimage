#ifndef COMMON_FIRMWARE_H
#define COMMON_FIRMWARE_H

#define FW_RESPONSE_FREEZE_MASK  0x0F0F
#define FW_RESPONSE_FREEZE_ERROR 0x030D

#ifndef FW_RESPONSE_ERROR
#define FW_RESPONSE_ERROR 0x0300
#endif

#ifndef FW_RESPONSE_MASK
#define FW_RESPONSE_MASK 0x0F00
#endif

#ifndef FW_RESPONSE_CODE_MASK
#define FW_RESPONSE_CODE_MASK 0x00FF
#endif

#define FW_DOWNLOAD_MARGIN_UNIT 8
#define FW_LOAD_FRAME_TIMEOUT   2000
#define FW_READY_TIMEOUT        30000
#define FW_MAGIC_WORD_TIMEOUT   10000

#include "common/common_display.h"

typedef struct {
    uint8_t fw_type;
    uint8_t fw_hash[3];
    uint16_t fw_crc;
    uint16_t fw_date;
    uint32_t entry;
    uint32_t length;
    uint32_t download_dest;
    uint32_t decompress_dest;
    uint32_t SHA_offset;
    uint32_t RSA_offset;
} FirmwareHeader_t;

typedef enum {
    UNCOMPRESSED = 0,
    COMPRESSED1 = 1,
    COMPRESSED2 = 2,
} FirmwareType_t;

typedef enum {
    ERROR_UNKNOWN_COMMAND = 1,
    ERROR_INVALID_INPUT = 2,
    ERROR_PHY_NOT_READY = 3,
    ERROR_EM_NOT_READY = 4,
    ERROR_EM_GOING_ON = 5,
    ERROR_EM_CANCELLED = 6,
    ERROR_EM_NOT_STARTED = 7,
    ERROR_OUT_OF_MEMORY = 8,
    ERROR_FIRMWARE_BUSY = 9,
    ERROR_INVALID_INDEX = 10,
    ERROR_INVALID_MODE = 11,
    ERROR_INVALID_SECTION = 12,
    ERROR_FW_FREEZE = 13,
    ERROR_LANE_MAPPING = 14,
    ERROR_LANE_OUT_OF_RANGE = 15,
    ERROR_LANE_USED = 16,
    ERROR_MODE = 17,
    ERROR_MODE_OUT_OF_RANGE = 18,
    ERROR_NOT_IMPLEMENTED = 19,
    ERROR_PORT_USED = 20,
    ERROR_UNSUPPORT_AN_SPEED = 21,
    ERROR_UNSUPPORT_LINE_ANLT = 22,
    ERROR_UNSUPPORT_SYS_ANLT = 23,
    ERROR_OUT_OF_RANGE = 24,
    ERROR_UNSUPPORT_NRZ_LTONLY = 25,
    ERROR_PORT_OUT_OF_RANGE = 26,
    ERROR_VSENSOR_BUSY = 27,
    ERROR_CODE_MAX = 255,
} ERROR_CODE_COMMON_t;

const char* fw_errorcodes_to_string(unsigned errorcodes);

static inline bool is_fw_load_magic(unsigned magic) {
    return ((magic == FW_LOAD_MAGIC_WORD)
#ifdef FW_LOAD_MAGIC_WORD_2
            || (magic == FW_LOAD_MAGIC_WORD_2)
#endif
#ifdef FW_LOAD_MAGIC_WORD_3
            || (magic == FW_LOAD_MAGIC_WORD_3)
#endif
    );
}

static inline void log_if_invalid_fw_load_magic(CredoSlice_t* slice, unsigned magic) {
    if (is_fw_load_magic(magic)) return;
#ifdef FW_LOAD_MAGIC_WORD_2
    LOGS_ERROR("Invalid firmware magic word 0x%04x! Expecting 0x%04x or 0x%04x", magic, FW_LOAD_MAGIC_WORD,
               FW_LOAD_MAGIC_WORD_2);
#else
    LOGS_ERROR("Invalid firmware magic word 0x%04x! Expecting 0x%04x", magic, FW_LOAD_MAGIC_WORD);
#endif
}

CredoError_t common_fw_notepad(CredoSlice_t* slice, unsigned* notepad_address);
CredoError_t common_fw_parse_header(CredoSlice_t* slice, FILE* file, unsigned* start_offset);
CredoError_t common_fw_unload(CredoSlice_t* slice);
CredoError_t common_fw_load(CredoSlice_t* slice, FILE* file);
CredoError_t common_fw_load_broadcast(CredoSlice_t* slices[], int slice_count, FILE* file, unsigned delay_time_us);
CredoError_t common_fw_load_inner(CredoSlice_t* slice, FILE* file, unsigned delay_time_us, int broadcast);

CredoError_t common_fw_wait_magic_word(CredoSlice_t* slice, unsigned timeout);
CredoError_t common_fw_wait_ready(CredoSlice_t* slice);

CredoError_t common_fw_clear_top_pll_cal(CredoSlice_t* slice);
CredoError_t common_fw_wait_top_pll_cal(CredoSlice_t* slice, unsigned timeout);

CredoError_t common_fw_magic(CredoSlice_t* slice, unsigned* magic_word);
CredoError_t common_fw_hash(CredoSlice_t* slice, unsigned* hash_code);
CredoError_t common_fw_date(CredoSlice_t* slice, unsigned* date_code);
CredoError_t common_fw_crc(CredoSlice_t* slice, unsigned* crc16_code);
CredoError_t common_fw_ver(CredoSlice_t* slice, unsigned* version);

#define FW_CMD_LOG_SILENT (0x0F00 << 16)
CredoError_t common_wait_fw_cmd(CredoSlice_t* slice, unsigned cmd, unsigned* response, unsigned CMD_TIMEOUT);
CredoError_t common_fw_cmd_send(CredoSlice_t* slice, unsigned cmd, unsigned param);
CredoError_t common_fw_cmd_response(CredoSlice_t* slice, unsigned cmd, unsigned param, unsigned* response,
                                    unsigned* response_param);
CredoError_t common_fw_cmd(CredoSlice_t* slice, unsigned cmd, unsigned param, unsigned* response,
                           unsigned* response_param);
CredoError_t common_fw_cmd_ex(CredoSlice_t* slice, unsigned cmd, unsigned param1, unsigned param2, unsigned* response,
                              unsigned* response_param1, unsigned* response_param2);

CredoError_t common_fw_reg_rd(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned* value);
CredoError_t common_fw_reg_wr(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned value);
CredoError_t common_fw_reg_rd_internal(CredoSlice_t* slice, unsigned combined_addr, unsigned* value);
CredoError_t common_fw_reg_wr_internal(CredoSlice_t* slice, unsigned combined_addr, unsigned value);

CredoError_t common_fw_debug_cmd_ex(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                    unsigned* value1, unsigned* value2);
CredoError_t common_fw_debug_cmd(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index, unsigned* value);
CredoError_t common_fw_info_data(CredoSlice_t* slice, int length, int data[]);
CredoError_t common_fw_info_data_unsigned(CredoSlice_t* slice, int length, unsigned data[]);

CredoError_t common_fw_get_slice_temp(CredoSlice_t* slice, double* temp);

#ifdef FW_SPEED_INFO
CredoError_t common_fw_get_speed_index(CredoSlice_t* slice, int lane, unsigned* speed_index);
#endif
#if HAL_SUPPORT_FW_ADAPT_COUNT
CredoError_t common_fw_get_adapt_count(CredoSlice_t* slice, int lane, unsigned* count);
#endif
#ifdef FW_READAPT_COUNT
CredoError_t common_fw_get_readapt_count(CredoSlice_t* slice, int lane, unsigned* count);
#endif
#ifdef FW_LINK_LOST_COUNT
CredoError_t common_fw_get_link_lost_count(CredoSlice_t* slice, int lane, unsigned* count);
#endif
#ifdef FW_LOS_COUNT
CredoError_t common_fw_get_los_count(CredoSlice_t* slice, int lane, unsigned* count);
#endif
#if HAL_SUPPORT_FW_RATIO
CredoError_t common_fw_get_channel_estimate(CredoSlice_t* slice, int lane, double* chan_est);
#endif
#if HAL_SUPPORT_FW_OF
CredoError_t common_fw_get_of(CredoSlice_t* slice, int lane, unsigned* of);
#endif
#if HAL_SUPPORT_FW_HF
CredoError_t common_fw_get_hf(CredoSlice_t* slice, int lane, unsigned* hf);
#endif
#if HAL_SUPPORT_FW_DFE
CredoError_t common_fw_get_ths(CredoSlice_t* slice, int lane, int ths[]);
CredoError_t common_fw_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]);
#endif
#if HAL_SUPPORT_FW_EYE
CredoError_t common_fw_get_eye(CredoSlice_t* slice, int lane, int eyes[3]);
#endif
#if HAL_SUPPORT_FW_ISI
CredoError_t common_fw_get_isi(CredoSlice_t* slice, int lane, int isi[]);
#endif
CredoError_t common_fw_get_rx_ffe(CredoSlice_t* slice, int lane, int ffe[]);
CredoError_t common_fw_get_rx_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]);
CredoError_t common_fw_get_rx_ffe_kaccu(CredoSlice_t* slice, int lane, double kaccu[]);
CredoError_t common_fw_get_rx_ffe_flip_counter(CredoSlice_t* slice, int lane, int flip_counter[]);

#if HAL_SUPPORT_EYE_MONITOR
CredoError_t common_fw_em_start(CredoSlice_t* slice, int lane, int ber_exp, int flag);
CredoError_t common_fw_em_stop(CredoSlice_t* slice, int lane);
CredoError_t common_fw_em_progress(CredoSlice_t* slice, int lane, int* percent);
CredoError_t common_fw_em_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv);
#ifdef FW_EM_VSTEP_SEPARATOR
CredoError_t common_fw_em_separator(CredoSlice_t* slice, int separator[5]);
#endif
CredoError_t common_fw_em_print_bathtub_ascii(CredoSlice_t* slice, int** data, int vstep_side, int hstep_side,
                                              int extent_mv, const DisplayState_t* D);
CredoError_t common_fw_em_print_eye_monitor_ascii(CredoSlice_t* slice, int** data, int vstep_side, int hstep_side,
                                                  int extent_mv, const DisplayState_t* D);
#endif

FirmwareType_t common_fw_firmware_type(void* buffer);
unsigned common_fw_firmware_length(void* buffer);
CredoError_t common_fw_header_display(CredoSlice_t* slice, void* buffer);
CredoError_t common_fw_load_spi(CredoSlice_t* slice, int partition_num);
#if HAL_SUPPORT_LOAD_SPI
CredoError_t common_fw_spiflash_status(CredoSlice_t* slice, unsigned* response);
CredoError_t common_fw_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count);
CredoError_t common_fw_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count);
CredoError_t common_fw_spiflash_erase(CredoSlice_t* slice, unsigned addr);
CredoError_t common_fw_spiflash_display_mbr(CredoSlice_t* slice);
CredoError_t common_fw_spiflash_format_mbr(CredoSlice_t* slice, unsigned flash_kb_size, int force);
CredoError_t common_fw_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num);
CredoError_t common_fw_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num, int force);
#endif

#endif
