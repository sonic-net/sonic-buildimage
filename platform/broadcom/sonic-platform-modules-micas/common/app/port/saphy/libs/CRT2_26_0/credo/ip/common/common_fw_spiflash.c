#include "project.h"

#include "common/common_firmware.h"

#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>

#if HAL_SUPPORT_LOAD_SPI
#define SPI_FLASH_STATUS_READY_TIMEOUT (500)  // ms
static CredoError_t common_fw_spiflash_cmd(CredoSlice_t* slice, unsigned cmd, unsigned addr_lsb, unsigned addr_msb,
                                           unsigned* response, unsigned* response_param1, unsigned* response_param2) {
    unsigned res, res1, res2;
    CredoError_t ret = CR_FAIL;
    CredoTime_t start_time;
    get_time(&start_time);

    // LOGS_DEBUG( "cmd 0x%04x, addr 0x%08x", cmd, (addr_msb << 16) | addr_lsb);
    do {
        ERR_PROPS(hal_fw_cmd_ex(slice, cmd, addr_lsb, addr_msb, &res, &res1, &res2));
        // 0xff: flash busy
        if ((res & 0xff) != 0xff) {
            ret = CR_OK;
            goto exit;
        }
    } while (us_passed(&start_time) < (SPI_FLASH_STATUS_READY_TIMEOUT * 1000));

    LOGS_ERROR("spiflash timeout after %u msecs", (unsigned)(us_passed(&start_time) / 1000));

exit:
    if (response) *response = res;
    if (response_param1) *response_param1 = res1;
    if (response_param2) *response_param2 = res2;

    return ret;
}

CredoError_t common_fw_spiflash_status(CredoSlice_t* slice, unsigned* response) {
    ERR_PROPS(common_fw_spiflash_cmd(slice, FW_CMD_SPI_STATUS, 0, 0, NULL, response, NULL));
    return CR_OK;
}

#define MAX_SECTION_COUNT (16 / 2)  // 16 REG_DATA as 8*4bytes
CredoError_t common_fw_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count) {
    unsigned sec_count;
    unsigned buf_idx = 0;

    if ((slice == NULL) || (buffer == NULL) || (count == 0)) return CR_INVALID_ARGS;
    if (addr & 0x3) {
        LOGS_ERROR("addr 0x%08x must be aligned 4", addr);
        return CR_INVALID_ARGS;
    }

    while (count != 0) {
        unsigned val;

        sec_count = (count > MAX_SECTION_COUNT) ? MAX_SECTION_COUNT : count;
        ERR_PROPS(common_fw_spiflash_cmd(slice, FW_CMD_SPI_READ + sec_count, (addr & 0xffff), (addr >> 16), NULL, NULL,
                                         NULL));

        for (int i = 0; i < sec_count; i++) {
            unsigned char* b = (unsigned char*)(buffer + buf_idx);
            ERR_PROPS(readTop(slice, REG_DATA + i * 2, &val));
            b[0] = (val >> 8) & 0xFF;
            b[1] = (val >> 0) & 0xFF;
            ERR_PROPS(readTop(slice, REG_DATA + i * 2 + 1, &val));
            b[2] = (val >> 8) & 0xFF;
            b[3] = (val >> 0) & 0xFF;
            buf_idx++;
        }

        count -= sec_count;
        addr += sec_count * 4;
    }

    return CR_OK;
}

CredoError_t common_fw_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count) {
    unsigned sec_count;
    unsigned buf_idx = 0;

    if ((slice == NULL) || (buffer == NULL) || (count == 0)) return CR_INVALID_ARGS;
    if (addr & 0x3) {
        LOGS_ERROR("addr 0x%08x must be aligned 4", addr);
        return CR_INVALID_ARGS;
    }

    while (count != 0) {
        unsigned val;

        sec_count = (count > MAX_SECTION_COUNT) ? MAX_SECTION_COUNT : count;

        for (int i = 0; i < sec_count; i++) {
            unsigned char* b = (unsigned char*)(buffer + buf_idx);
            val = b[0] << 8 | b[1];
            ERR_PROPS(writeTop(slice, REG_DATA + i * 2, val));
            val = b[2] << 8 | b[3];
            ERR_PROPS(writeTop(slice, REG_DATA + i * 2 + 1, val));
            buf_idx++;
        }

        ERR_PROPS(common_fw_spiflash_cmd(slice, FW_CMD_SPI_WRITE + sec_count - 1, (addr & 0xffff), (addr >> 16), NULL,
                                         NULL, NULL));

        count -= sec_count;
        addr += sec_count * 4;
    }

    return CR_OK;
}

CredoError_t common_fw_spiflash_erase(CredoSlice_t* slice, unsigned addr) {
    if (addr % 4096) {
        LOGS_ERROR("erase addr 0x%08X must be multiple of 4096", addr);
        return CR_FAIL;
    }
    ERR_PROPS(common_fw_spiflash_cmd(slice, FW_CMD_SPI_ERASE, (addr & 0xffff), (addr >> 16), NULL, NULL, NULL));
    return CR_OK;
}

#define SWAP(m, n) \
    do {           \
        m ^= n;    \
        n ^= m;    \
        m ^= n;    \
    } while (0)
#define ENDIAN_CONVERT(x)                       \
    do {                                        \
        size_t bytes = sizeof(x);               \
        uint8_t* m = (uint8_t*)&(x);            \
        uint8_t* n = m + bytes - 1;             \
        for (int i = 0; i < (bytes / 2); i++) { \
            SWAP(*m, *n);                       \
            m += 1;                             \
            n -= 1;                             \
        }                                       \
    } while (0)

#define MBR_SPIFLASH_ADDR    0xbc
#define MBR_SIGNATURE        0x33CC55AA
#define PART_SIGNATURE       0xAA55CC33
#define NUM_SPI_PARTITION    4
#define SPIFLASH_SECTOR_SIZE 256

typedef struct {
    uint32_t part_status;
    uint32_t start_sector;
    uint32_t end_sector;
    uint32_t _unused_;
} SPIPartitionRecord_t;

typedef struct {
    SPIPartitionRecord_t parts[NUM_SPI_PARTITION];
    uint32_t signature;
} SPIMBR_t;

typedef struct {
    uint32_t signature;
    uint32_t start_sector;
    uint32_t end_sector;
    uint32_t status;
    uint32_t image_size;
    uint32_t _reserved_[3];
} SPIPartition_t;

#define SPI_PART_STATUS_ACTIVE  (1 << 0)
#define SPI_PART_STATUS_FWARE   (1 << 1)
#define SPI_PART_STATUS_DATA    (1 << 2)
#define SPI_PART_STATUS_HAS_SHA (1 << 3)
#define SPI_PART_STATUS_HAS_RSA (1 << 4)
#define SPI_PART_STATUS_ACFG    (1 << 5)

static CredoError_t check_spiflash_mbr_valid(CredoSlice_t* slice, SPIMBR_t* mbr) {
    if (mbr == NULL) return CR_INVALID_ARGS;
    if (mbr->signature != MBR_SIGNATURE) {
        LOGS_WARN("Valid MBR not found. Please format the flash first");
        return CR_FAIL;
    }

    return CR_OK;
}

static CredoError_t show_spiflash_partition_record(CredoSlice_t* slice, SPIMBR_t* mbr, int partition_num) {
    if (0 <= partition_num && partition_num < NUM_SPI_PARTITION) {
        SPIPartitionRecord_t* record = &mbr->parts[partition_num];
        LOGS_INFO("Partition[%02d]: Sector 0x%08x - 0x%08x, flag 0x%04x", partition_num, record->start_sector,
                  record->end_sector, record->part_status);
        return CR_OK;
    }
    LOGS_ERROR("partition_num %d must <= %d", partition_num, NUM_SPI_PARTITION);
    return CR_FAIL;
}

static CredoError_t show_spiflash_mbr_info(CredoSlice_t* slice, SPIMBR_t* mbr) {
    for (int i = 0; i < NUM_SPI_PARTITION; i++) {
        show_spiflash_partition_record(slice, mbr, i);
    }
    // LOGS_INFO("Signature: 0x%08x", mbr->signature);

    return CR_OK;
}

static void _endian_convert(void* vp, uint32_t size) {
    uint32_t* p = (uint32_t*)vp;
    for (int i = 0; i < size / sizeof(uint32_t); i++) ENDIAN_CONVERT(p[i]);
}

static CredoError_t read_spiflash_mbr(CredoSlice_t* slice, SPIMBR_t* mbr) {
    if (mbr == NULL) return CR_INVALID_ARGS;

    ERR_PROPS(common_fw_spiflash_read(slice, MBR_SPIFLASH_ADDR, (unsigned*)mbr, sizeof(SPIMBR_t) / sizeof(unsigned)));
    _endian_convert(mbr, sizeof(*mbr));
    ERR_PROPS(check_spiflash_mbr_valid(slice, mbr));
    return CR_OK;
}

static CredoError_t write_spiflash_mbr(CredoSlice_t* slice, SPIMBR_t* mbr) {
    if (mbr == NULL) return CR_INVALID_ARGS;

    ERR_PROPS(check_spiflash_mbr_valid(slice, mbr));
    ERR_PROPS(common_fw_spiflash_erase(slice, 0));
    _endian_convert(mbr, sizeof(*mbr));
    ERR_PROPS(common_fw_spiflash_write(slice, MBR_SPIFLASH_ADDR, (unsigned*)mbr, sizeof(SPIMBR_t) / sizeof(unsigned)));
    _endian_convert(mbr, sizeof(*mbr));
    return CR_OK;
}

CredoError_t common_fw_spiflash_display_mbr(CredoSlice_t* slice) {
    SPIMBR_t mbr;
    ERR_PROPS(read_spiflash_mbr(slice, &mbr));
    LOGS_INFO("MBR:");
    ERR_PROPS(show_spiflash_mbr_info(slice, &mbr));
    return CR_OK;
}

CredoError_t common_fw_spiflash_format_mbr(CredoSlice_t* slice, unsigned flash_kb_size, int force) {
    if (flash_kb_size < 512) {
        LOGS_ERROR("The flash is too small, %d kb", flash_kb_size);
        return CR_FAIL;
    }

    SPIMBR_t mbr;

    // skip format if MBR exists
    if ((force != 1) && (read_spiflash_mbr(slice, &mbr) == CR_OK)) {
        LOGS_INFO("Old MBR:");
        ERR_PROPS(show_spiflash_mbr_info(slice, &mbr));
        LOGS_WARN("Valid MBR exists");
        return CR_OK;
    }

    unsigned num_part_sectors = flash_kb_size * 1024 / SPIFLASH_SECTOR_SIZE / NUM_SPI_PARTITION;

    for (int i = 0; i < NUM_SPI_PARTITION; i++) {
        mbr.parts[i].part_status = 0;
        mbr.parts[i].start_sector = num_part_sectors * i;
        mbr.parts[i].end_sector = num_part_sectors * (i + 1) - 1;
        mbr.parts[i]._unused_ = 0;
    }
    // The fisrt 4KB is used as MBR, and the last 4KB is reserved.
    // Therefore the first and last partition are 4KB smaller than other two partitions.
    mbr.parts[0].start_sector += 4096 / SPIFLASH_SECTOR_SIZE;
    mbr.parts[NUM_SPI_PARTITION - 1].end_sector -= 4096 / SPIFLASH_SECTOR_SIZE;
    mbr.signature = MBR_SIGNATURE;

    ERR_PROPS(write_spiflash_mbr(slice, &mbr));

    LOGS_INFO("New MBR:");
    ERR_PROPS(show_spiflash_mbr_info(slice, &mbr));

    return CR_OK;
}

static CredoError_t read_spiflash_partition_info(CredoSlice_t* slice, unsigned addr, SPIPartition_t* part) {
    if (part == NULL) return CR_INVALID_ARGS;

    ERR_PROPS(common_fw_spiflash_read(slice, addr, (unsigned*)part, sizeof(SPIPartition_t) / sizeof(unsigned)));
    _endian_convert(part, sizeof(*part));
    if (part->signature != PART_SIGNATURE) {
        LOGS_ERROR("incorrect partition signature 0x%08x", part->signature);
        return CR_FAIL;
    }
    return CR_OK;
}

static CredoError_t write_spiflash_partition_info(CredoSlice_t* slice, unsigned addr, SPIPartition_t* part) {
    if (part == NULL) return CR_INVALID_ARGS;

    if (part->signature != PART_SIGNATURE) {
        LOGS_ERROR("incorrect partition signature 0x%08x", part->signature);
        return CR_FAIL;
    }
    _endian_convert(part, sizeof(*part));
    ERR_PROPS(common_fw_spiflash_erase(slice, addr));
    ERR_PROPS(common_fw_spiflash_write(slice, addr, (unsigned*)part, sizeof(SPIPartition_t) / sizeof(unsigned)));
    _endian_convert(part, sizeof(*part));
    return CR_OK;
}

static CredoError_t read_spiflash_fw_header(CredoSlice_t* slice, unsigned addr, FirmwareHeader_t* fw_header) {
    if (fw_header == NULL) return CR_INVALID_ARGS;

    ERR_PROPS(common_fw_spiflash_read(slice, addr, (unsigned*)fw_header, sizeof(FirmwareHeader_t) / sizeof(unsigned)));
    return CR_OK;
}

static CredoError_t write_spiflash_fw_header(CredoSlice_t* slice, unsigned addr, FirmwareHeader_t* fw_header) {
    if (fw_header == NULL) return CR_INVALID_ARGS;

    ERR_PROPS(common_fw_spiflash_write(slice, addr, (unsigned*)fw_header, sizeof(FirmwareHeader_t) / sizeof(unsigned)));
    return CR_OK;
}

static CredoError_t common_fw_spiflash_read_to_file(CredoSlice_t* slice, uint32_t addr, uint32_t count, FILE* fp) {
    const unsigned sector_count = SPIFLASH_SECTOR_SIZE / sizeof(uint32_t);
    while (count != 0) {
        unsigned buffer[SPIFLASH_SECTOR_SIZE / sizeof(unsigned)];
        unsigned write_count = (count > sector_count) ? sector_count : count;
        ERR_PROPS(common_fw_spiflash_read(slice, addr, buffer, write_count));

        if (fwrite(&buffer, sizeof(unsigned), write_count, fp) != write_count) {
            LOGS_ERROR("unable to write firmware file index");
            return CR_FAIL;
        }

        addr += write_count * sizeof(unsigned);
        count -= write_count;
    }
    return CR_OK;
}

static CredoError_t common_fw_spiflash_read_firmware_inner(CredoSlice_t* slice, FILE* fp, int partition_num,
                                                           SPIPartition_t* part) {
    FirmwareHeader_t fw_header;
    unsigned flash_start_addr = part->start_sector * SPIFLASH_SECTOR_SIZE;
    unsigned fw_length;
    unsigned write_count;
    CredoTime_t start_time;

    if ((part->status & SPI_PART_STATUS_FWARE) == 0) {
        LOGS_ERROR("partition status 0x%08x is not firmware", part->status);
        return CR_FAIL;
    }

    get_time(&start_time);
    ERR_PROPS(read_spiflash_fw_header(slice, flash_start_addr + sizeof(SPIPartition_t), &fw_header));

    LOGS_INFO("Partition[%02d] Firmware:", partition_num);
    ERR_PROPS(common_fw_header_display(slice, &fw_header));
    LOGS_INFO("Start firmware reading...");

    // FW_FORMAT_OLD
    unsigned buffer[4096 / sizeof(unsigned)];
    write_count = sizeof(buffer) / sizeof(unsigned);
    for (int i = 0; i < write_count; i++) buffer[i] = 0xffffffff;
    if (fwrite(buffer, sizeof(unsigned), write_count, fp) != write_count) {
        LOGS_ERROR("unable to write firmware format");
        return CR_FAIL;
    }

    fw_length = common_fw_firmware_length(&fw_header);
    write_count = ((common_fw_firmware_type(&fw_header) < COMPRESSED2) ? 20 : 24) / sizeof(unsigned);
    if (fwrite(&fw_header, sizeof(unsigned), write_count, fp) != write_count) {
        LOGS_ERROR("unable to write firmware header");
        return CR_FAIL;
    }

    flash_start_addr += sizeof(SPIPartition_t) + sizeof(FirmwareHeader_t);
    ERR_PROPS(common_fw_spiflash_read_to_file(slice, flash_start_addr, fw_length / sizeof(unsigned), fp));

    LOGS_INFO("                         Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));

    return CR_OK;
}

static CredoError_t common_fw_spiflash_read_data_inner(CredoSlice_t* slice, FILE* fp, int partition_num,
                                                       SPIPartition_t* part) {
    unsigned flash_start_addr = part->start_sector * SPIFLASH_SECTOR_SIZE + sizeof(SPIPartition_t);
    uint32_t image_size = part->image_size;
    CredoTime_t start_time;

    get_time(&start_time);
    LOGS_INFO("Start data reading...");
    ERR_PROPS(common_fw_spiflash_read_to_file(slice, flash_start_addr, image_size / sizeof(unsigned), fp));
    LOGS_INFO("                         Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));
    return CR_OK;
}

CredoError_t common_fw_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num) {
    CredoError_t ret;
    FILE* fp = NULL;
    SPIMBR_t mbr;

    if (fwname == NULL) {
        LOGS_ERROR("Please input fw file\n");
        return CR_FAIL;
    }

    if ((partition_num < 0) || (partition_num >= NUM_SPI_PARTITION)) {
        LOGS_ERROR("Partition number is from 0 to %d", NUM_SPI_PARTITION - 1);
        return CR_FAIL;
    }

    if ((fp = fopen(fwname, "wb")) == NULL) {
        LOGS_ERROR("fw file %s open fail, %s\n", fwname, strerror(errno));
        return CR_FAIL;
    }

    if ((ret = read_spiflash_mbr(slice, &mbr)) == CR_OK) {
        SPIPartition_t part;
        if ((ret = read_spiflash_partition_info(slice, mbr.parts[partition_num].start_sector * SPIFLASH_SECTOR_SIZE,
                                                &part)) == CR_OK) {
            if (part.status & SPI_PART_STATUS_FWARE) {
                ret = common_fw_spiflash_read_firmware_inner(slice, fp, partition_num, &part);
                LOGS_INFO("Partition[%02d] Firmware, size %d", partition_num, part.image_size);
            } else if (part.image_size > 0) {
                ret = common_fw_spiflash_read_data_inner(slice, fp, partition_num, &part);
                if (part.status & SPI_PART_STATUS_DATA) {
                    LOGS_INFO("Partition[%02d] %sData, size %d", partition_num,
                              (part.status & SPI_PART_STATUS_ACFG) ? "Auto Config " : "", part.image_size);
                } else {
                    LOGS_INFO("Partition[%02d] Unknown Data, size %d", partition_num, part.image_size);
                }
            } else {
                LOGS_WARN("Partition[%02d] is empty", partition_num);
            }
        }
    }

    if (fp) fclose(fp);

    return ret;
}

static CredoError_t common_fw_spiflash_write_firmware_inner(CredoSlice_t* slice, FILE* fp, int partition_num) {
    SPIMBR_t mbr;
    SPIPartitionRecord_t* record;
    unsigned image_start_offset;
    unsigned shift;
    FirmwareHeader_t fw_header;
    unsigned flash_start_addr;
    SPIPartition_t part;
    unsigned fw_length;
    CredoTime_t start_time;

    ERR_PROPS(read_spiflash_mbr(slice, &mbr));
    record = &mbr.parts[partition_num];

    // Write new firmware into flash
    ERR_PROPS(common_fw_parse_header(slice, fp, &image_start_offset));

    if (fseek(fp, image_start_offset, SEEK_SET)) {
        LOGS_ERROR("fseek fail!");
        return CR_FAIL;
    }

    if (sizeof(fw_header) != fread(&fw_header, sizeof(char), sizeof(fw_header), fp)) {
        LOGS_ERROR("unable to read firmware header");
        return CR_FAIL;
    }

    fw_header.SHA_offset = fw_header.RSA_offset = 0;    // clear SHA and RSA first
    fw_length = common_fw_firmware_length(&fw_header);  // get length before convert
    LOGS_INFO("Firmware image:");
    ERR_PROPS(common_fw_header_display(slice, &fw_header));

    shift = (common_fw_firmware_type(&fw_header) < COMPRESSED2) ? 20 : 24;
    if (fseek(fp, image_start_offset + shift, SEEK_SET)) {
        LOGS_ERROR("Failed shifting to firmware data start");
        return CR_FAIL;
    }

    part.signature = PART_SIGNATURE;
    part.start_sector = record->start_sector;
    part.end_sector = record->end_sector;
    record->part_status = part.status = SPI_PART_STATUS_FWARE;
    part.image_size = fw_length;
    for (int i = 0; i < 3; i++) part._reserved_[i] = 0;

    unsigned sector_left = SPIFLASH_SECTOR_SIZE / sizeof(unsigned);

    LOGS_INFO("Start firmware programming...");
    get_time(&start_time);
    flash_start_addr = part.start_sector * SPIFLASH_SECTOR_SIZE;
    ERR_PROPS(write_spiflash_partition_info(slice, flash_start_addr, &part));

    sector_left -= sizeof(part) / sizeof(unsigned);
    flash_start_addr += sizeof(part);
    ERR_PROPS(write_spiflash_fw_header(slice, flash_start_addr, &fw_header));

    sector_left -= sizeof(fw_header) / sizeof(unsigned);
    flash_start_addr += sizeof(fw_header);

    fw_length /= sizeof(unsigned);
    while (fw_length != 0) {
        unsigned buffer[SPIFLASH_SECTOR_SIZE / sizeof(unsigned)];
        unsigned write_count = (fw_length > sector_left) ? sector_left : fw_length;

        if ((flash_start_addr % 4096) == 0) ERR_PROPS(common_fw_spiflash_erase(slice, flash_start_addr));

        if (fread(buffer, sizeof(unsigned), write_count, fp) != write_count) {
            LOGS_ERROR("unable to read firmware file index");
            return CR_FAIL;
        }

        ERR_PROPS(common_fw_spiflash_write(slice, flash_start_addr, (unsigned*)buffer, write_count));

        flash_start_addr += write_count * sizeof(unsigned);
        fw_length -= write_count;
        sector_left = SPIFLASH_SECTOR_SIZE / sizeof(unsigned);
    }

    LOGS_INFO("                             Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));
    get_time(&start_time);
    ERR_PROPS(write_spiflash_mbr(slice, &mbr));
    LOGS_INFO("Update MBR...Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));
    LOGS_INFO("Updated MBR:");
    ERR_PROPS(show_spiflash_partition_record(slice, &mbr, partition_num));

    return CR_OK;
}

static CredoError_t common_fw_spiflash_write_auto_cfg_data_inner(CredoSlice_t* slice, FILE* fp, int partition_num) {
    SPIMBR_t mbr;
    SPIPartitionRecord_t* record;
    unsigned flash_start_addr;
    SPIPartition_t part;
    CredoTime_t start_time;

    ERR_PROPS(read_spiflash_mbr(slice, &mbr));
    record = &mbr.parts[partition_num];

    part.signature = PART_SIGNATURE;
    part.start_sector = record->start_sector;
    part.end_sector = record->end_sector;
    record->part_status = part.status = SPI_PART_STATUS_DATA;
    part.status |= SPI_PART_STATUS_ACFG;
    fseek(fp, 0, SEEK_END);
    part.image_size = ftell(fp);
    for (int i = 0; i < 3; i++) part._reserved_[i] = 0;

    unsigned sector_left = SPIFLASH_SECTOR_SIZE / sizeof(unsigned);

    LOGS_INFO("Start auto_config_data programming...");
    get_time(&start_time);
    flash_start_addr = part.start_sector * SPIFLASH_SECTOR_SIZE;
    ERR_PROPS(write_spiflash_partition_info(slice, flash_start_addr, &part));

    sector_left -= sizeof(part) / sizeof(unsigned);
    flash_start_addr += sizeof(part);

    fseek(fp, 0, SEEK_SET);

    unsigned buffer[SPIFLASH_SECTOR_SIZE / sizeof(unsigned)];
    unsigned write_count;

    while ((write_count = fread(buffer, sizeof(unsigned), sector_left, fp)) != 0) {
        if ((flash_start_addr % 4096) == 0) ERR_PROPS(common_fw_spiflash_erase(slice, flash_start_addr));

        ERR_PROPS(common_fw_spiflash_write(slice, flash_start_addr, (unsigned*)buffer, write_count));

        flash_start_addr += write_count * sizeof(unsigned);
        sector_left = SPIFLASH_SECTOR_SIZE / sizeof(unsigned);
    }

    LOGS_INFO("                             Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));
    get_time(&start_time);
    ERR_PROPS(write_spiflash_mbr(slice, &mbr));
    LOGS_INFO("Update MBR...Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));
    LOGS_INFO("Updated MBR:");
    ERR_PROPS(show_spiflash_partition_record(slice, &mbr, partition_num));
    return CR_OK;
}

static int is_auto_cfg_data_format(FILE* file) {
    unsigned char data[32];

    fseek(file, 0, SEEK_SET);
    fread(data, sizeof(char), 32, file);
    return (data[0] == 0xBE && data[1] == 0xEF) ? 1 : 0;
}

static CredoError_t check_any_data_exist_flash(CredoSlice_t* slice, int partition_num, int* is_exist) {
    SPIMBR_t mbr;
    SPIPartitionRecord_t* record;

    *is_exist = 0;

    ERR_PROPS(read_spiflash_mbr(slice, &mbr));
    record = &mbr.parts[partition_num];

    // Check old firmware exists in flash
    if (record->part_status != 0) {
        SPIPartition_t part;
        FirmwareHeader_t fw_header;
        unsigned flash_start_addr = record->start_sector * SPIFLASH_SECTOR_SIZE;

        ERR_PROPS(read_spiflash_partition_info(slice, flash_start_addr, &part));
        if (part.status != 0) {
            *is_exist = 1;
            if ((part.status & SPI_PART_STATUS_FWARE) != 0) {
                ERR_PROPS(read_spiflash_fw_header(slice, flash_start_addr + sizeof(SPIPartition_t), &fw_header));
                LOGS_INFO("Partition[%02d] Firmware:", partition_num);
                ERR_PROPS(common_fw_header_display(slice, &fw_header));
                LOGS_WARN("Skip programming because firmware exists in flash");
            } else if ((part.status & SPI_PART_STATUS_DATA) != 0) {
                if (part.status & SPI_PART_STATUS_ACFG) {
                    LOGS_INFO("Partition[%02d] Auto Config Data", partition_num);
                } else {
                    LOGS_INFO("Partition[%02d] Data", partition_num);
                }
                LOGS_WARN("Skip programming because data exists in flash");
            }
        }
    }

    return CR_OK;
}

CredoError_t common_fw_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num, int force) {
    CredoError_t ret;
    FILE* fp = NULL;

    if (fwname == NULL) {
        LOGS_ERROR("Please input fw file\n");
        return CR_FAIL;
    }

    if ((partition_num < 0) || (partition_num >= NUM_SPI_PARTITION)) {
        LOGS_ERROR("Partition number is from 0 to %d", NUM_SPI_PARTITION - 1);
        return CR_FAIL;
    }

    if (force == 0) {
        int is_exist = 0;
        ERR_PROPS(check_any_data_exist_flash(slice, partition_num, &is_exist));
        if (is_exist) {
            return CR_OK;
        }
    }

    if ((fp = fopen(fwname, "rb")) == NULL) {
        LOGS_ERROR("fw file %s open fail, %s\n", fwname, strerror(errno));
        return CR_FAIL;
    }

    if (is_auto_cfg_data_format(fp)) {
        ret = common_fw_spiflash_write_auto_cfg_data_inner(slice, fp, partition_num);
    } else {
        ret = common_fw_spiflash_write_firmware_inner(slice, fp, partition_num);
    }

    if (fp) fclose(fp);

    return ret;
}
#endif  // HAL_SUPPORT_LOAD_SPI
