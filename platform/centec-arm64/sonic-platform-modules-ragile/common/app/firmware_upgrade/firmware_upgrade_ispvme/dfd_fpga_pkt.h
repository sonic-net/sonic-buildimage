#ifndef __DFD_FPGA_PKT_H__
#define __DFD_FPGA_PKT_H__

typedef enum dfd_fpga_pkt_op_type_e {
    DFD_FPGA_PKT_OP_TYPE_READ           = 0,        /* read */
    DFD_FPGA_PKT_OP_TYPE_WRITE          = 1,        /* write */
    DFD_FPGA_PKT_OP_TYPE_END,
} dfd_fpga_pkt_op_type_t;

typedef enum dfd_fpga_pkt_ack_type_e {
    DFD_FPGA_PKT_ACK_TYPE_NO_ERROR       = 0,        /* successful operation */
    DFD_FPGA_PKT_ACK_TYPE_FCS_ERROR      = 1,        /* operation FCS check error */
    DFD_FPGA_PKT_ACK_TYPE_FAIL_ERROR     = 2,        /* operation failed */
    DFD_FPGA_PKT_ACK_TYPE_END,
} dfd_fpga_pkt_ack_type_t;

typedef enum dfd_rv_s {
    DFD_RV_OK               = 0,
    DFD_RV_INIT_ERR         = 1,
    DFD_RV_SLOT_INVALID     = 2,
    DFD_RV_MODE_INVALID     = 3,
    DFD_RV_MODE_NOTSUPPORT  = 4,
    DFD_RV_TYPE_ERR         = 5,
    DFD_RV_DEV_NOTSUPPORT   = 6,
    DFD_RV_DEV_FAIL         = 7,
    DFD_RV_INDEX_INVALID    = 8,
    DFD_RV_NO_INTF          = 9,
    DFD_RV_NO_NODE          = 10,
    DFD_RV_NODE_FAIL        = 11,
} dfd_rv_t;

typedef struct dfd_pci_dev_priv_s {
    int pcibus;
    int slot;
    int fn;
    int bar;
    int offset;
    int times;
    int align;
    int fpga_upg_base;
}dfd_pci_dev_priv_t;

#define DFD_PCI_MAX_NAME_SIZE          256

#define DFD_MAX_FPGA_NUM                                        (8)
#define DFD_FPGA_PKT_WORD_LEN                                   (4)

#define DFD_FPGA_PKT_MAC_LEN                                    (6)
#define DFD_FPGA_PKT_PAYLOAD_WORD_DATA_LEN                      (4)

#define DFD_FPGA_PKT_WORD_RW_LEN                                (1)/* for each access, according to 1 WORD, 4 bytes to access FPGA */

#define DFD_FPGA_PKT_ETYPE                                      (0xfff9)

#define DFD_FPGA_PKT_PAYLOAD_ADDR_LEN                           (4)
#define DFD_FPGA_PKT_PAYLOAD_LENGTH_LEN                         (2)

#define DFD_FPGA_PKT_PAYLOAD_ADDR_OFFSET                        (0)
#define DFD_FPGA_PKT_PAYLOAD_LENGTH_OFFSET                      (DFD_FPGA_PKT_PAYLOAD_ADDR_LEN)
#define DFD_FPGA_PKT_PAYLOAD_DATA_OFFSET                        ((DFD_FPGA_PKT_PAYLOAD_ADDR_LEN) + (DFD_FPGA_PKT_PAYLOAD_LENGTH_LEN))

#define DFD_FPGA_PKT_GET_DATA(payload)                          (((uint8_t*)(payload)) + DFD_FPGA_PKT_PAYLOAD_DATA_OFFSET)
#define DFD_FPGA_PKT_GET_PAYLOAD_LEN(len)                       ((DFD_FPGA_PKT_PAYLOAD_ADDR_LEN) + (DFD_FPGA_PKT_PAYLOAD_LENGTH_LEN) + (len))

#pragma pack (1)
typedef struct dfd_fpga_pkt_payload_s {
    uint32_t addr;                                  /* the address of reading and writting */
    uint16_t length;                                /* the length of reading and writting */
    uint32_t data;                                  /* read and write data (for read operations, you don't need to care about this field) */
} dfd_fpga_pkt_payload_t;

typedef struct dfd_fpga_pkt_rd_payload_s {
    uint32_t addr;                                  /* the address of reading  */
    uint16_t length;                                /* the length of reading */
} dfd_fpga_pkt_rd_payload_t;
#pragma pack ()

typedef enum fpga_version_e {
    FPGA_VER_00  = 0x00,
    FPGA_VER_01,
    FPGA_VER_02,
    FPGA_VER_03,
    FPGA_VER_04,
    FPGA_VER_05,
    FPGA_VER_06,
} fpga_version_t;	

int dfd_fpga_upg_init(void);
int dfd_fpga_write_word(dfd_pci_dev_priv_t *pci_priv, int addr, int val);
int dfd_fpga_read_word(dfd_pci_dev_priv_t *pci_priv, int addr, int *val);
int dfd_fpga_pci_write(dfd_pci_dev_priv_t *pci_priv, int offset, uint8_t *buf, int wr_len);
int dfd_fpga_pci_read(dfd_pci_dev_priv_t *pci_priv, int offset, uint8_t *buf, int rd_len);
extern int drv_get_my_dev_type(void);

#endif