#ifndef __FIRMWARE_UPGRADE_RPD__
#define __FIRMWARE_UPGRADE_RPD__

#include "fw_upg_spi_logic_dev.h"

/****************************** Intel logical device define start*********************************/
#define FIRMWARE_SPI_INTEL_LOGIC_SECTOR_SIZE    (0x10000)           /* One sector is 64K */
#define FPGA_QSPI_RDSR_OPCODE                   (0x05000041)        /* Opcode:0x05(RDSR), Read_data_en, Execute cmd*/
#define FPGA_QSPI_SE_TIMEOUT                    (10 * 1000 * 1000)  /* Polling block erase timeout period: 10s */
#define FPGA_QSPI_SE_SLEEP_TIME                 (10 * 1000)         /* Polling block erase sleep time: 10ms */
#define FPGA_QSPI_WAIT_FIFO_SLEEP               (5)                 /* Polling for FIFO idle time delay: 5us */
#define FPGA_QSPI_WAIT_FIFO_TIMEOUT             (10 * 1000 * 1000)  /* Polling for FIFO idle timeout: 10s */
#define FPGA_QSPI_RW_FIFO_RETRY_SLEEP           (5)                 /* FIFO read/write failure retry delay: 5us */

#define FPGA_QSPI_SR_WIP                        (0x01)
#define FPGA_RDDATA_INV_VALUE                   (0x5A5A5A5A)
#define IFC_TOP_CPU_FIFO_SIZE                   (512)

/* refer to UG-20159 */
#define IFC_TOP_CPU_CSR_STATUS_REGISTER         (0x000)
#define IFC_TOP_CPU_ISR_REGISTER                (0x004)
#define IFC_TOP_CPU_IER_REGISTER                (0x008)
#define IFC_TOP_CPU_CHIPSEL_REGISTER            (0x00C)
#define IFC_TOP_CPU_OPEN_REGISTER               (0x010)
#define IFC_TOP_CPU_CLOSE_REGISTER              (0x014)
#define IFC_TOP_CPU_WREN_REGISTER               (0x018)
#define IFC_TOP_CPU_WRSTATUS_REGISTER           (0x01C)
#define IFC_TOP_CPU_RDSTATUS_REGISTER           (0x020)
#define IFC_TOP_CPU_SE_REGISTER                 (0x024)
#define IFC_TOP_CPU_RDID_REGISTER               (0x028)
#define IFC_TOP_CPU_CONTROL_REGISTER            (0x034)
#define IFC_TOP_CPU_NUMBYTES_REGISTER           (0x038)
#define IFC_TOP_CPU_WRDATA0_REGISTER            (0x03C)
#define IFC_TOP_CPU_WRDATA1_REGISTER            (0x040)
#define IFC_TOP_CPU_RDDATA0_REGISTER            (0x044)
#define IFC_TOP_CPU_RDDATA1_REGISTER            (0x048)
#define IFC_TOP_CPU_WRITEOP_REGISTER            (0x050)
#define IFC_TOP_CPU_WRADDR_REGISTER             (0x054)
#define IFC_TOP_CPU_WRFIFOLVL_REGISTER          (0x058)
#define IFC_TOP_CPU_READOP_REGISTER             (0x05C)
#define IFC_TOP_CPU_READADDR_REGISTER           (0x060)
#define IFC_TOP_CPU_READWORDS_REGISTER          (0x064)
#define IFC_TOP_CPU_READFIFOLVL_REGISTER        (0x068)
#define IFC_TOP_CPU_CONTROL_REGISTER0           (0x800)
#define IFC_TOP_CPU_CONTROL_REGISTER1           (0x804)
#define IFC_TOP_CPU_MEM_STATUS_REGISTER         (0x808)
#define IFC_TOP_CPU_FLASH_RDDATA                (0x900)
#define IFC_TOP_CPU_FLASH_WRDATA                (0xB00)

/* Error Codes */
enum ifc_status {
    IFC_SUCCESS                       = 0,
    /* Generic codes : Range -1..-49 */
    IFC_ERR_PARAM                     = -1,
    IFC_ERR_INVALID_ARGUMENT          = -2,
    IFC_ERR_FW_CMD_TRUNCATED          = -3,
    IFC_ERR_BAD_CONFIG                = -4,
    IFC_ERR_OP_FAILED                 = -5,
    IFC_ERR_MAP_FAILED                = -6,
    IFC_ERR_NO_MEMORY                 = -11,
    IFC_ERR_LOCKID_FAILURE            = -12,
    IFC_ERR_OUT_OF_RANGE              = -13,
    IFC_ERR_LOCKED                    = -14,
    IFC_ERR_LOCK_FAILURE              = -15,
    IFC_ERR_NOT_IMPLEMENTED           = -16,
    /* Message specific error codes. Range -100..-109 */
    IFC_ERR_PKT_TOO_SMALL             = -100,
    IFC_ERR_MSG_TYPE_INVALID          = -101,
    IFC_ERR_MSG_NO_MORE               = -102,
    IFC_ERR_MSGQ_EMPTY                = -102,
    IFC_ERR_MSGQ_FULL                 = -103,
    /* FW command specific error codes. Range -120..-129 */
    IFC_ERR_HOST_INTERFACE_COMMAND    = -120,
    IFC_ERR_FW_CMD_STATUS             = -121,
    IFC_ERR_FW_CMD_TIMEOUT            = -122,
    IFC_ERR_FW_CMD_BAD_ECHO           = -128,
    IFC_FLOW_HANDLE_TOUT              = -129,
    IFC_FT_ERROR_CMD_TOUT             = -130,
    IFC_ERR_FLOW_INSERT_FAILED        = -131,
    IFC_ERR_DUMMY_KEY_RECIRC          = -132,

    IFC_ERR_INSERT_ROW_FULL           = -133,
    IFC_ERR_INSERT_REHASH_FAIL        = -134,
    IFC_ERR_INSERT_KEY_EXISTS         = -135,
    IFC_ERR_INSERT_NO_FREE_PTR        = -136,
    IFC_ERR_INSERT_TABLE_FULL         = -137,
    IFC_ERR_KEY_NOT_EXIST             = -138,
    IFC_ERR_DELETE_NEXTPTR_ERROR      = -139,
    IFC_ERR_FLOW_DELETE_FAILED        = -140,
};

/* Intel spi flash upgrade */
extern int firmware_upgrade_do_spi_intel_logic(firmware_spi_logic_info_t *logic_dev_info, uint8_t *buf, uint32_t size);
/* Intel spi flash upgrade test*/
extern int firmware_intel_fpga_upgrade_test(firmware_spi_logic_info_t *logic_dev_info);
/* intel spi flash data print*/
extern int firmware_upgreade_spi_intel_logic_dump(firmware_spi_logic_info_t *logic_dev_info, uint32_t offset,
               uint8_t *buf, uint32_t rd_size);
/****************************** Intel logical device define end*********************************/

#endif