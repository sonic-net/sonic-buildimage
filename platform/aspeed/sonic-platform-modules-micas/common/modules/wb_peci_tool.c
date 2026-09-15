/*
 * wb_peci_tool.c
 * ko to read/write peci
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/peci.h>
#include <linux/peci-cpu.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/unaligned.h>

#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>
#include <peci-ioctl.h>

MODULE_IMPORT_NS(PECI);
MODULE_IMPORT_NS(PECI_CPU);

#define PECI_RETRY_BIT              BIT(0)
#define PECI_RETRY_TIMEOUT          msecs_to_jiffies(700)
#define PECI_RETRY_INTERVAL_MIN     msecs_to_jiffies(1)
#define PECI_RETRY_INTERVAL_MAX     msecs_to_jiffies(128)

#define PECI_NAME_BUFF_SIZE     32

#define PECI_TOOL_MINOR 0

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static dev_t peci_tool_dev;
static struct cdev peci_tool_cdev;
static struct class *peci_tool_class;
extern struct bus_type peci_bus_type;

extern int peci_request_status(struct peci_request *req);
extern struct peci_request *peci_xfer_get_dib(struct peci_device *device);
extern u64 peci_request_dib_read(struct peci_request *req);
extern int peci_temp_read(struct peci_device *device, s16 *temp_raw);
extern struct peci_request *peci_request_alloc(struct peci_device *device, u8 tx_len, u8 rx_len);
extern void peci_request_free(struct peci_request *req);

#define CRC8_TABLE_SIZE         256

#define PECI_CRC8_POLYNOMIAL    0x07
static u8 peci_crc8_table[CRC8_TABLE_SIZE];

static u8 crc8(const u8 table[CRC8_TABLE_SIZE], const u8 *pdata, size_t nbytes, u8 crc)
{
    /* loop over the buffer data */
    while (nbytes-- > 0) {
        crc = table[(crc ^ *pdata++) & 0xff];
    }

    return crc;
}

static void crc8_populate_msb(u8 table[CRC8_TABLE_SIZE], u8 polynomial)
{
    int i, j;
    const u8 msbit = 0x80;
    u8 t = msbit;

    table[0] = 0;

    for (i = 1; i < CRC8_TABLE_SIZE; i *= 2) {
        t = (t << 1) ^ (t & msbit ? polynomial : 0);
        for (j = 0; j < i; j++) {
            table[i+j] = table[j] ^ t;
        }
    }
}

static struct peci_device *peci_tool_get_device(int controller_id, int addr)
{
    struct device *dev;
    struct peci_device *peci_dev;
    char name[PECI_NAME_BUFF_SIZE];

    mem_clear(name, sizeof(name));
    (void)snprintf(name, sizeof(name), "%d-%02x", controller_id, addr);
    dev = bus_find_device_by_name(&peci_bus_type, NULL, name);
    if (dev != NULL) {
        peci_dev = container_of(dev, struct peci_device, dev);
        return peci_dev;
    }

    return NULL;
}

static void peci_tool_put_device(struct peci_device *device)
{
    put_device(&(device->dev));
}

/**
 * peci_request_xfer
 * 
 * drivers\peci\request.c: peci_request_xfer()
 */
static int peci_request_xfer(struct peci_request *req)
{
    struct peci_device *device = req->device;
    struct peci_controller *controller = to_peci_controller(device->dev.parent);
    int ret;

    mutex_lock(&controller->bus_lock);
    ret = controller->ops->xfer(controller, device->addr, req);
    mutex_unlock(&controller->bus_lock);

    if (ret < 0) {
        DEBUG_ERROR("controller->ops->xfer fail. ret = %d.\n", ret);
    } else {
        DEBUG_INFO("controller->ops->xfer succ. ret = %d.\n", ret);
    }

    return ret;
}

static int peci_request_xfer_retry(struct peci_request *req)
{
    long wait_interval = PECI_RETRY_INTERVAL_MIN;
    struct peci_device *device = req->device;
    unsigned long start = jiffies;
    int ret;

    /* Don't try to use it for ping */
    if (WARN_ON(req->tx.len == 0)) {
        return 0;
    }

    do {
        ret = peci_request_xfer(req);
        if (ret) {
            DEBUG_ERROR("xfer error: %d\n", ret);
            return ret;
        }

        if (peci_request_status(req) != -EAGAIN) {
            return 0;
        }

        /* Set the retry bit to indicate a retry attempt */
        req->tx.buf[1] |= PECI_RETRY_BIT;

        if (schedule_timeout_interruptible(wait_interval)) {
            DEBUG_ERROR("xfer error for restart sys.\n");
            return -ERESTARTSYS;
        }

        wait_interval = min_t(long, wait_interval * 2, PECI_RETRY_INTERVAL_MAX);
    } while (time_before(jiffies, start + PECI_RETRY_TIMEOUT));

    DEBUG_ERROR("request timed out\n");

    return -ETIMEDOUT;
}

/* Calculate an Assured Write Frame Check Sequence byte */
static int peci_aw_fcs(struct peci_request *req, int len, u8 *aw_fcs)
{
    u8 *tmp_buf;

    /* Allocate a temporary buffer to use a contiguous byte array */
    tmp_buf = kmalloc(len, GFP_KERNEL);
    if (!tmp_buf) {
        DEBUG_ERROR("kmalloc fail, len = %d.\n", len);
        return -ENOMEM;
    }

    tmp_buf[0] = req->device->addr;
    tmp_buf[1] = req->tx.len;
    tmp_buf[2] = req->rx.len;
    memcpy(&tmp_buf[3], req->tx.buf, len - 3);

    *aw_fcs = crc8(peci_crc8_table, tmp_buf, (size_t)len, 0);
    DEBUG_INFO("fcs = 0x%x\n", *aw_fcs);
    kfree(tmp_buf);

    return 0;
}

static int peci_cmd_xfer(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_xfer_msg *umsg;
    struct peci_request req;
    u8 aw_fcs;
    int ret;

    umsg = &(info->cmd_info.xfer);

    mem_clear(&req, sizeof(struct peci_request));
    req.device = device;
    req.rx.len = umsg->rx_len;
    req.tx.len = umsg->tx_len;

    if (req.rx.len >= PECI_REQUEST_MAX_BUF_SIZE || req.tx.len >= PECI_REQUEST_MAX_BUF_SIZE) {
        DEBUG_ERROR("Invalid tx/rx length, tx_len: %d, rx_len: %d\n", req.tx.len, req.rx.len);
        return -EINVAL;
    }

    if (umsg->tx_len > 0) {
        if (copy_from_user(req.tx.buf, umsg->tx_buf, umsg->tx_len))
            return -EFAULT;
    }

    if (!umsg->tx_len) {
        ret = peci_request_xfer(&req);
    } else {
        switch (req.tx.buf[0]) {
        case PECI_GET_DIB_CMD:
        case PECI_GET_TEMP_CMD:
            ret = peci_request_xfer(&req);
            break;
        case PECI_WRPKGCFG_CMD:
        case PECI_WRIAMSR_CMD:
        case PECI_WRPCICFG_CMD:
        case PECI_WRPCICFGLOCAL_CMD:
        case PECI_WRENDPTCFG_CMD:
            /*
             * The sender may not have supplied the AW FCS byte.
             * Unconditionally add an Assured Write Frame Check
             * Sequence byte
             */
            ret = peci_aw_fcs(&req, 2 + req.tx.len, &aw_fcs);
            if (ret) {
                break;
            }

            req.tx.buf[req.tx.len - 1] = 0x80 ^ aw_fcs;

            ret = peci_request_xfer(&req);
            break;
        default:
            ret = peci_request_xfer_retry(&req);
            break;
        }
    }

    if (!ret && umsg->rx_len > 0) {
        if (copy_to_user(umsg->rx_buf, req.rx.buf, umsg->rx_len)) {
            ret = -EFAULT;
        }
    }

    return ret;
}

/**
 * peci_tool_ping
 * 
 * drivers\peci\device.c: peci_detect()
 */
static int peci_cmd_ping(struct peci_device *device, peci_tool_info_t *info)
{
    /*
     * PECI Ping is a command encoded by tx_len = 0, rx_len = 0.
     * We expect correct Write FCS if the device at the target address
     * is able to respond.
     */
    struct peci_request req = { 0 };

    pr_debug("Pinging PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    req.device = device;
    return peci_request_xfer(&req);
}

/**
 * peci_tool_get_dib
 * 
 * drivers\peci\device.c:peci_get_revision()
 */
static int peci_cmd_get_dib(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_request *req;
    struct peci_get_dib_msg *dib_msg;

    pr_debug("Getting DIB from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    dib_msg = &(info->cmd_info.dib);
    req = peci_xfer_get_dib(device);
    if (IS_ERR(req)) {
        DEBUG_ERROR("peci_xfer_get_dib fail.\n");
        return -EIO;
    }

    /*
     * PECI device may be in a state where it is unable to return a proper
     * DIB, in which case it returns 0 as DIB value.
     * Let's treat this as an error to avoid carrying on with the detection
     * using invalid revision.
     */
    dib_msg->dib = peci_request_dib_read(req);
    if (dib_msg->dib == 0) {
        DEBUG_ERROR("peci_request_dib_read fail.\n");
        peci_request_free(req);
        return -EIO;
    }

    peci_request_free(req);

    return 0;
}

/**
 * peci_tool_get_temp
 */
static int peci_cmd_get_temp(struct peci_device *device, peci_tool_info_t *info)
{
    int ret;
    struct peci_get_temp_msg *temp;

    pr_debug("Getting temperature from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    temp = &(info->cmd_info.temp);
    ret = peci_temp_read(device, &(temp->temp_raw));
    if (ret < 0) {
        DEBUG_ERROR("peci_temp_read fail. ret = %d\n", ret);
        return ret;
    }

    return 0;
}

/**
 * peci_cmd_rd_pkg_cfg
 * drivers\peci\request.c：__pkg_cfg_read()
 * drivers\peci\peci-core.c: peci_cmd_rd_pkg_cfg
 */
static int peci_cmd_rd_pkg_cfg(struct peci_device *device, peci_tool_info_t *info)
{
    int ret;
    struct peci_rd_pkg_cfg_msg *umsg;
    struct peci_request *req;

    pr_debug("Reading package configuration from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.rd_pkg_cfg);

    if (umsg->rx_len != 1 && umsg->rx_len != 2 && umsg->rx_len != 4) {
        DEBUG_ERROR("Invalid read length, rx_len: %d\n", umsg->rx_len);
        return -EINVAL;
    }

    req = peci_request_alloc(device, PECI_RDPKGCFG_WRITE_LEN, 
            PECI_RDPKGCFG_READ_LEN_BASE + umsg->rx_len);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    req->tx.buf[0] = PECI_RDPKGCFG_CMD;
    req->tx.buf[1] = 0;
    req->tx.buf[2] = umsg->index;
    put_unaligned_le16(umsg->param, &req->tx.buf[3]);

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(umsg->pkg_config, &req->rx.buf[1], umsg->rx_len);
    }

    umsg->cc = req->rx.buf[0];

    peci_request_free(req);
    return ret;
}

/**
 * peci_cmd_wr_pkg_cfg
 * drivers\peci\peci-core.c: peci_cmd_wr_pkg_cfg()
 */
static int peci_cmd_wr_pkg_cfg(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_wr_pkg_cfg_msg *umsg;
    struct peci_request *req;
    u8 aw_fcs, domain_id;
    int ret, i;

    pr_debug("Writing package configuration to PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.wr_pkg_cfg);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    /* Per the PECI spec, the write length must be a dword */
    if (umsg->tx_len != 4) {
        DEBUG_ERROR("Invalid write length, tx_len: %d\n", umsg->tx_len);
        return -EINVAL;
    }

    req = peci_request_alloc(device, PECI_WRPKGCFG_WRITE_LEN_BASE + umsg->tx_len,
            PECI_WRPKGCFG_READ_LEN);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    req->tx.buf[0] = PECI_WRPKGCFG_CMD;
    req->tx.buf[1] = domain_id << 1;         /* Domain ID [7:1] | Retry bit [0] */
    req->tx.buf[2] = umsg->index;            /* RdPkgConfig index */
    put_unaligned_le16(umsg->param, &req->tx.buf[3]);
    for (i = 0; i < umsg->tx_len; i++) {
        req->tx.buf[5 + i] = (u8)(umsg->value >> (i << 3));
    }

    /* Add an Assured Write Frame Check Sequence byte */
    ret = peci_aw_fcs(req, 8 + umsg->tx_len, &aw_fcs);
    if (ret) {
        goto out;
    }

    req->tx.buf[5 + i] = 0x80 ^ aw_fcs;

    ret = peci_request_xfer_retry(req);

out:
    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

/**
 * peci_cmd_rd_ia_msr
 * drivers\peci\peci-core.c: peci_cmd_rd_ia_msr()
 */
static int peci_cmd_rd_ia_msr(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_rd_ia_msr_msg *umsg;
    struct peci_request *req;
    u8 domain_id;
    int ret;

    pr_debug("Reading IA MSR from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.rd_ia_msr);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    req = peci_request_alloc(device, PECI_RDIAMSR_WRITE_LEN, PECI_RDIAMSR_READ_LEN);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    req->tx.buf[0] = PECI_RDIAMSR_CMD;
    req->tx.buf[1] = domain_id << 1; /* Domain ID [7:1] | Retry bit [0] */
    req->tx.buf[2] = umsg->thread_id;
    put_unaligned_le16(umsg->address, &req->tx.buf[3]);

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(&umsg->value, &req->rx.buf[1], sizeof(uint64_t));
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

/**
 * peci_cmd_wr_ia_msr
 * drivers\peci\peci-core.c: peci_cmd_wr_ia_msr()
 */
static int peci_cmd_wr_ia_msr(struct peci_device *device, peci_tool_info_t *info)
{
    return -ENOSYS; /* Not implemented yet */
}

/**
 * peci_cmd_rd_ia_msrex
 * drivers\peci\peci-core.c: peci_cmd_rd_ia_msrex()
 */
static int peci_cmd_rd_ia_msrex(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_rd_ia_msrex_msg *umsg;
    struct peci_request *req;
    u8 domain_id;
    int ret;

    pr_debug("Reading IA MSR Extended from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.rd_ia_msrex);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    req = peci_request_alloc(device, PECI_RDIAMSREX_WRITE_LEN, PECI_RDIAMSREX_READ_LEN);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    req->tx.buf[0] = PECI_RDIAMSREX_CMD;
    req->tx.buf[1] = domain_id << 1; /* Domain ID [7:1] | Retry bit [0] */
    put_unaligned_le16(umsg->thread_id, &req->tx.buf[2]);
    put_unaligned_le16(umsg->address, &req->tx.buf[4]);

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(&umsg->value, &req->rx.buf[1], sizeof(uint64_t));
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

static u32 __get_pci_addr(u8 bus, u8 dev, u8 func, u16 reg)
{
    return reg | PCI_DEVID(bus, PCI_DEVFN(dev, func)) << 12;
}

/**
 * peci_cmd_rd_pci_cfg
 * drivers\peci\peci-core.c: peci_cmd_rd_pci_cfg()
 */
static int peci_cmd_rd_pci_cfg(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_rd_pci_cfg_msg *umsg;
    struct peci_request *req;
    u8 domain_id;
    u32 address;
    int ret;

    pr_debug("Reading PCI configuration from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.rd_pci_cfg);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    req = peci_request_alloc(device, PECI_RDPCICFG_WRITE_LEN, PECI_RDPCICFG_READ_LEN);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    address = __get_pci_addr(umsg->bus, umsg->device, umsg->function, umsg->reg);
    req->tx.buf[0] = PECI_RDPCICFG_CMD;
    req->tx.buf[1] = domain_id << 1;      /* Domain ID [7:1] | Retry bit [0] */
    put_unaligned_le32(address, &req->tx.buf[2]);

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(&umsg->pci_config, &req->rx.buf[1], 4);
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

/**
 * peci_cmd_wr_pci_cfg
 * drivers\peci\peci-core.c: peci_cmd_wr_pci_cfg()
 */
static int peci_cmd_wr_pci_cfg(struct peci_device *device, peci_tool_info_t *info)
{
    return -ENOSYS; /* Not implemented yet */
}

/**
 * peci_cmd_rd_pci_cfg_local
 * drivers\peci\peci-core.c: peci_cmd_rd_pci_cfg_local()
 */
static int peci_cmd_rd_pci_cfg_local(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_rd_pci_cfg_local_msg *umsg;
    struct peci_request *req;
    u8 domain_id;
    u32 address;
    int ret;

    pr_debug("Reading local PCI configuration from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.rd_pci_cfg_local);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    /* Per the PECI spec, the read length must be a byte, word, or dword */
    if (umsg->rx_len != 1 && umsg->rx_len != 2 && umsg->rx_len != 4) {
        DEBUG_ERROR("Invalid read length, rx_len: %d\n", umsg->rx_len);
        return -EINVAL;
    }

    req = peci_request_alloc(device, PECI_RDPCICFGLOCAL_WRITE_LEN,
            PECI_RDPCICFGLOCAL_READ_LEN_BASE + umsg->rx_len);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    address = __get_pci_addr(umsg->bus, umsg->device, umsg->function, umsg->reg);
    req->tx.buf[0] = PECI_RDPCICFGLOCAL_CMD;
    req->tx.buf[1] = domain_id << 1;   /* Domain ID [7:1] | Retry bit [0] */
    put_unaligned_le24(address, &req->tx.buf[2]);

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(umsg->pci_config, &req->rx.buf[1], umsg->rx_len);
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

static int peci_cmd_wr_pci_cfg_local(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_wr_pci_cfg_local_msg *umsg;
    struct peci_request *req;
    u8 aw_fcs, domain_id;
    u32 address;
    int ret, i;

    pr_debug("Writing local PCI configuration to PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.wr_pci_cfg_local);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    /* Per the PECI spec, the write length must be a byte, word, or dword */
    if (umsg->tx_len != 1 && umsg->tx_len != 2 && umsg->tx_len != 4) {
        DEBUG_ERROR("Invalid write length, tx_len: %d\n", umsg->tx_len);
        return -EINVAL;
    }

    req = peci_request_alloc(device, PECI_WRPCICFGLOCAL_WRITE_LEN_BASE + umsg->tx_len,
            PECI_WRPCICFGLOCAL_READ_LEN);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    address = __get_pci_addr(umsg->bus, umsg->device, umsg->function, umsg->reg);

    req->tx.buf[0] = PECI_WRPCICFGLOCAL_CMD;
    req->tx.buf[1] = domain_id << 1;   /* Domain ID [7:1] | Retry bit [0] */
    put_unaligned_le24(address, &req->tx.buf[2]);
    for (i = 0; i < umsg->tx_len; i++) {
        req->tx.buf[5 + i] = (u8)(umsg->value >> (i << 3));
    }

    /* Add an Assured Write Frame Check Sequence byte */
    ret = peci_aw_fcs(req, 8 + umsg->tx_len, &aw_fcs);
    if (ret) {
        goto out;
    }

    req->tx.buf[5 + i] = 0x80 ^ aw_fcs;

    ret = peci_request_xfer_retry(req);
out:
    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

static int peci_cmd_rd_end_pt_cfg(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_rd_end_pt_cfg_msg *umsg;
    struct peci_request *req;
    u8 tx_size, domain_id;
    u32 address;
    int ret;

    pr_debug("Reading endpoint configuration from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.rd_end_pt_cfg);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    switch (umsg->msg_type) {
    case PECI_ENDPTCFG_TYPE_LOCAL_PCI:
    case PECI_ENDPTCFG_TYPE_PCI:
        /*
         * Per the PECI spec, the read length must be a byte, word,
         * or dword
         */
        if (umsg->rx_len != 1 && umsg->rx_len != 2 && umsg->rx_len != 4) {
            DEBUG_ERROR("Invalid read length, rx_len: %d\n", umsg->rx_len);
            return -EINVAL;
        }

        req = peci_request_alloc(device, PECI_RDENDPTCFG_PCI_WRITE_LEN,
                PECI_RDENDPTCFG_READ_LEN_BASE + umsg->rx_len);
        if (!req) {
            DEBUG_ERROR("peci_request_alloc fail.\n");
            return -ENOMEM;
        }

        address = __get_pci_addr(umsg->params.pci_cfg.bus, umsg->params.pci_cfg.device, 
                    umsg->params.pci_cfg.function, umsg->params.pci_cfg.reg);
        req->tx.buf[0] = PECI_RDENDPTCFG_CMD;
        req->tx.buf[1] = domain_id << 1;	   /* Domain ID [7:1] | Retry bit [0] */
        req->tx.buf[2] = umsg->msg_type;	   /* Message Type */
        req->tx.buf[3] = 0x00;			   /* Endpoint ID */
        req->tx.buf[4] = 0x00;			   /* Reserved */
        req->tx.buf[5] = 0x00;			   /* Reserved */
        req->tx.buf[6] = PECI_ENDPTCFG_ADDR_TYPE_PCI; /* Addr Type */
        req->tx.buf[7] = umsg->params.pci_cfg.seg; /* PCI Segment */
        put_unaligned_le32(address, &req->tx.buf[8]);
        break;

    case PECI_ENDPTCFG_TYPE_MMIO:
        /*
         * Per the PECI spec, the read length must be a byte, word,
         * dword, or qword
         */
        if (umsg->rx_len != 1 && umsg->rx_len != 2 &&
            umsg->rx_len != 4 && umsg->rx_len != 8) {
            DEBUG_ERROR("Invalid read length, rx_len: %d\n", umsg->rx_len);
            return -EINVAL;
        }
        /*
         * Per the PECI spec, the address type must specify either DWORD
         * or QWORD
         */
        if (umsg->params.mmio.addr_type != PECI_ENDPTCFG_ADDR_TYPE_MMIO_D 
                && umsg->params.mmio.addr_type != PECI_ENDPTCFG_ADDR_TYPE_MMIO_Q) {
            DEBUG_ERROR("Invalid address type, addr_type: %d\n", umsg->params.mmio.addr_type);
            return -EINVAL;
        }

        if (umsg->params.mmio.addr_type == PECI_ENDPTCFG_ADDR_TYPE_MMIO_D) {
            tx_size = PECI_RDENDPTCFG_MMIO_D_WRITE_LEN;
        } else {
            tx_size = PECI_RDENDPTCFG_MMIO_Q_WRITE_LEN;
        }

        req = peci_request_alloc(device, tx_size, PECI_RDENDPTCFG_READ_LEN_BASE + umsg->rx_len);
        if (!req) {
            DEBUG_ERROR("peci_request_alloc fail.\n");
            return -ENOMEM;
        }

        req->tx.buf[0] = PECI_RDENDPTCFG_CMD;
        req->tx.buf[1] = domain_id << 1;	      /* Domain ID [7:1] | Retry bit [0] */
        req->tx.buf[2] = umsg->msg_type;	      /* Message Type */
        req->tx.buf[3] = 0x00;			      /* Endpoint ID */
        req->tx.buf[4] = 0x00;			      /* Reserved */
        req->tx.buf[5] = umsg->params.mmio.bar;       /* BAR # */
        req->tx.buf[6] = umsg->params.mmio.addr_type; /* Address Type */
        req->tx.buf[7] = umsg->params.mmio.seg;       /* PCI Segment */
        req->tx.buf[8] = PCI_DEVFN(umsg->params.mmio.device, umsg->params.mmio.function);
        req->tx.buf[9] = umsg->params.mmio.bus; /* PCI Bus */

        if (umsg->params.mmio.addr_type == PECI_ENDPTCFG_ADDR_TYPE_MMIO_D) {
            put_unaligned_le32(umsg->params.mmio.offset, &req->tx.buf[10]);
        } else {
            put_unaligned_le64(umsg->params.mmio.offset, &req->tx.buf[10]);
        }

        break;

    default:
        return -EINVAL;
    }

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(umsg->data, &req->rx.buf[1], umsg->rx_len);
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

static int peci_cmd_wr_end_pt_cfg(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_wr_end_pt_cfg_msg *umsg;
    struct peci_request *req;
    u8 tx_size, aw_fcs, domain_id;
    int ret, i, idx;
    u32 address;

    pr_debug("Writing endpoint configuration to PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.wr_end_pt_cfg);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    switch (umsg->msg_type) {
    case PECI_ENDPTCFG_TYPE_LOCAL_PCI:
    case PECI_ENDPTCFG_TYPE_PCI:
        /*
         * Per the PECI spec, the write length must be a byte, word,
         * or dword
         */
        if (umsg->tx_len != 1 && umsg->tx_len != 2 && umsg->tx_len != 4) {
            DEBUG_ERROR("Invalid write length, tx_len: %d\n", umsg->tx_len);
            return -EINVAL;
        }

        req = peci_request_alloc(device, PECI_WRENDPTCFG_PCI_WRITE_LEN_BASE + umsg->tx_len,
                PECI_WRENDPTCFG_READ_LEN);
        if (!req) {
            DEBUG_ERROR("peci_request_alloc fail.\n");
            return -ENOMEM;
        }

        address = __get_pci_addr(umsg->params.pci_cfg.bus, umsg->params.pci_cfg.device, 
                    umsg->params.pci_cfg.function, umsg->params.pci_cfg.reg);
        req->tx.buf[0] = PECI_WRENDPTCFG_CMD;
        req->tx.buf[1] = domain_id << 1;        /* Domain ID [7:1] | Retry bit [0] */
        req->tx.buf[2] = umsg->msg_type;        /* Message Type */
        req->tx.buf[3] = 0x00;                  /* Endpoint ID */
        req->tx.buf[4] = 0x00;                  /* Reserved */
        req->tx.buf[5] = 0x00;                  /* Reserved */
        req->tx.buf[6] = PECI_ENDPTCFG_ADDR_TYPE_PCI;   /* Addr Type */
        req->tx.buf[7] = umsg->params.pci_cfg.seg;      /* PCI Segment */
        put_unaligned_le32(address, &req->tx.buf[8]);
        for (i = 0; i < umsg->tx_len; i++) {
            req->tx.buf[12 + i] = (u8)(umsg->value >> (i << 3));
        }

        /* Add an Assured Write Frame Check Sequence byte */
        ret = peci_aw_fcs(req, 15 + umsg->tx_len, &aw_fcs);
        if (ret) {
            goto out;
        }

        req->tx.buf[12 + i] = 0x80 ^ aw_fcs;
        break;

    case PECI_ENDPTCFG_TYPE_MMIO:
        /*
         * Per the PECI spec, the write length must be a byte, word,
         * dword, or qword
         */
        if (umsg->tx_len != 1 && umsg->tx_len != 2 && umsg->tx_len != 4 && umsg->tx_len != 8) {
            DEBUG_ERROR("Invalid write length, tx_len: %d\n", umsg->tx_len);
            return -EINVAL;
        }
        /*
         * Per the PECI spec, the address type must specify either DWORD
         * or QWORD
         */
        if (umsg->params.mmio.addr_type != PECI_ENDPTCFG_ADDR_TYPE_MMIO_D 
                && umsg->params.mmio.addr_type != PECI_ENDPTCFG_ADDR_TYPE_MMIO_Q) {
            DEBUG_ERROR("Invalid address type, addr_type: %d\n", umsg->params.mmio.addr_type);
            return -EINVAL;
        }

        if (umsg->params.mmio.addr_type == PECI_ENDPTCFG_ADDR_TYPE_MMIO_D) {
            tx_size = PECI_WRENDPTCFG_MMIO_D_WRITE_LEN_BASE + umsg->tx_len;
        } else {
            tx_size = PECI_WRENDPTCFG_MMIO_Q_WRITE_LEN_BASE + umsg->tx_len;
        }
        req = peci_request_alloc(device, tx_size, PECI_WRENDPTCFG_READ_LEN);
        if (!req) {
            DEBUG_ERROR("peci_request_alloc fail.\n");
            return -ENOMEM;
        }

        req->tx.buf[0] = PECI_WRENDPTCFG_CMD;
        req->tx.buf[1] = domain_id << 1;	      /* Domain ID [7:1] | Retry bit [0] */
        req->tx.buf[2] = umsg->msg_type;	      /* Message Type */
        req->tx.buf[3] = 0x00;			      /* Endpoint ID */
        req->tx.buf[4] = 0x00;			      /* Reserved */
        req->tx.buf[5] = umsg->params.mmio.bar;       /* BAR # */
        req->tx.buf[6] = umsg->params.mmio.addr_type; /* Address Type */
        req->tx.buf[7] = umsg->params.mmio.seg;       /* PCI Segment */
        /* Function/Device */
        req->tx.buf[8] = PCI_DEVFN(umsg->params.mmio.device, umsg->params.mmio.function);
        req->tx.buf[9] = umsg->params.mmio.bus; /* PCI Bus */
        if (umsg->params.mmio.addr_type == PECI_ENDPTCFG_ADDR_TYPE_MMIO_D) {
            put_unaligned_le32(umsg->params.mmio.offset, &req->tx.buf[10]);
            idx = 14;
        } else {
            put_unaligned_le64(umsg->params.mmio.offset, &req->tx.buf[10]);
            idx = 18;
        }

        for (i = 0; i < umsg->tx_len; i++) {
            req->tx.buf[idx + i] = (u8)(umsg->value >> (i << 3));
        }

        /* Add an Assured Write Frame Check Sequence byte */
        ret = peci_aw_fcs(req, idx + 3 + umsg->tx_len, &aw_fcs);
        if (ret) {
            goto out;
        }

        req->tx.buf[idx + i] = 0x80 ^ aw_fcs;
        break;

    default:
        return -EINVAL;
    }

    ret = peci_request_xfer_retry(req);

out:
    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

static int peci_cmd_crashdump_disc(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_crashdump_disc_msg *umsg;
    struct peci_request *req;
    u8 domain_id;
    int ret;

    pr_debug("Performing crashdump discovery on PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.crashdump_disc);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    /* Per the EDS, the read length must be a byte, word, or qword */
    if (umsg->rx_len != 1 && umsg->rx_len != 2 && umsg->rx_len != 8) {
        DEBUG_ERROR("Invalid read length, rx_len: %d\n", umsg->rx_len);
        return -EINVAL;
    }

    req = peci_request_alloc(device, PECI_CRASHDUMP_DISC_WRITE_LEN,
            PECI_CRASHDUMP_DISC_READ_LEN_BASE + umsg->rx_len);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        return -ENOMEM;
    }

    req->tx.buf[0] = PECI_CRASHDUMP_CMD;
    req->tx.buf[1] = domain_id << 1; /* Domain ID [7:1] | Retry bit [0] */
    req->tx.buf[2] = PECI_CRASHDUMP_DISC_VERSION;
    req->tx.buf[3] = PECI_CRASHDUMP_DISC_OPCODE;
    req->tx.buf[4] = umsg->subopcode;
    req->tx.buf[5] = umsg->param0;
    put_unaligned_le16(umsg->param1, &req->tx.buf[6]);
    req->tx.buf[8] = umsg->param2;

    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(umsg->data, &req->rx.buf[1], umsg->rx_len);
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

static int peci_cmd_crashdump_get_frame(struct peci_device *device, peci_tool_info_t *info)
{
    struct peci_crashdump_get_frame_msg *umsg;
    struct peci_request *req;
    u8 domain_id;
    int ret;

    pr_debug("Getting crashdump frame from PECI device at controller %d, address 0x%02x\n", 
        info->controller_id, info->addr);

    umsg = &(info->cmd_info.crashdump_get_frame);
    domain_id = (info->msg_len == sizeof(*umsg)) ? umsg->domain_id : 0;

    /* Per the EDS, the read length must be a qword or dqword */
    if (umsg->rx_len != 8 && umsg->rx_len != 16) {
        DEBUG_ERROR("Invalid read length, rx_len: %d\n", umsg->rx_len);
        pr_err("Invalid read length for crashdump get frame: %d\n", umsg->rx_len);
        return -EINVAL;
    }

    req = peci_request_alloc(device, PECI_CRASHDUMP_GET_FRAME_WRITE_LEN,
            PECI_CRASHDUMP_GET_FRAME_READ_LEN_BASE + umsg->rx_len);
    if (!req) {
        DEBUG_ERROR("peci_request_alloc fail.\n");
        pr_err("Failed to allocate PECI request for crashdump get frame\n");
        return -ENOMEM;
    }

    req->tx.buf[0] = PECI_CRASHDUMP_CMD;
    req->tx.buf[1] = domain_id << 1; /* Domain ID [7:1] | Retry bit [0] */
    req->tx.buf[2] = PECI_CRASHDUMP_GET_FRAME_VERSION;
    req->tx.buf[3] = PECI_CRASHDUMP_GET_FRAME_OPCODE;
    put_unaligned_le16(umsg->param0, &req->tx.buf[4]);
    put_unaligned_le16(umsg->param1, &req->tx.buf[6]);
    put_unaligned_le16(umsg->param2, &req->tx.buf[8]);
    ret = peci_request_xfer_retry(req);
    if (!ret) {
        memcpy(umsg->data, &req->rx.buf[1], umsg->rx_len);
    }

    umsg->cc = req->rx.buf[0];
    peci_request_free(req);

    return ret;
}

typedef int (*peci_cmd_fn_type)(struct peci_device *, peci_tool_info_t *);

static const peci_cmd_fn_type peci_cmd_fn[PECI_CMD_MAX] = {
    peci_cmd_xfer,
    peci_cmd_ping,
    peci_cmd_get_dib,
    peci_cmd_get_temp,
    peci_cmd_rd_pkg_cfg,
    peci_cmd_wr_pkg_cfg,
    peci_cmd_rd_ia_msr,
    peci_cmd_wr_ia_msr,
    peci_cmd_rd_ia_msrex,
    peci_cmd_rd_pci_cfg,
    peci_cmd_wr_pci_cfg,
    peci_cmd_rd_pci_cfg_local,
    peci_cmd_wr_pci_cfg_local,
    peci_cmd_rd_end_pt_cfg,
    peci_cmd_wr_end_pt_cfg,
    peci_cmd_crashdump_disc,
    peci_cmd_crashdump_get_frame,
};

static long peci_tool_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    peci_tool_info_t info;
    struct peci_device *peci_dev;
    int func_id;

    /* Verify the IOCTL command */
    if (_IOC_TYPE(cmd) != PECI_IOC_BASE) {
        return -ENOTTY;
    }

    func_id = _IOC_NR(cmd);
    if (func_id >= PECI_CMD_MAX || func_id < PECI_CMD_XFER) {
        DEBUG_ERROR("cmd error: cmd = 0x%x, func_id = %d\n", cmd, func_id);
        pr_err("Invalid IOCTL command %x\n", cmd);
        return -ENOTTY;
    }

    if (copy_from_user(&info, (void __user *)arg, sizeof(peci_tool_info_t))) {
        DEBUG_ERROR("Copy from user failed\n");
        pr_err("Failed to copy data from user\n");
        return -EFAULT;
    }

    /* get peci device */
    peci_dev = peci_tool_get_device(info.controller_id, info.addr);
    if (peci_dev == NULL) {
        DEBUG_ERROR("PECI device %d-%02x not found\n", info.controller_id, info.addr);
        pr_err("PECI device not found for controller_id: %d, addr: %02x\n", 
            info.controller_id, info.addr);
        return -ENODEV;
    }

    info.cmd_ret = peci_cmd_fn[func_id](peci_dev, &info);
    peci_tool_put_device(peci_dev);
    if (copy_to_user((void __user *)arg, &info, sizeof(peci_tool_info_t))) {
        DEBUG_ERROR("Copy to user failed\n");
        pr_err("Failed to copy data to user\n");
        return -EFAULT;
    }

    return 0;
}

static int peci_tool_open(struct inode *inode, struct file *file)
{
    DEBUG_INFO("PECI Tool device opened\n");
    return 0;
}

static int peci_tool_release(struct inode *inode, struct file *file)
{
    DEBUG_INFO("PECI Tool device released\n");
    return 0;
}

static const struct file_operations peci_tool_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = peci_tool_ioctl,
    .open = peci_tool_open,
    .release = peci_tool_release,
};

static int __init peci_tool_init(void)
{
    int ret;
    struct device *dev;

    pr_info("Initializing PECI Tool driver...\n");
    /* alloc devno */
    ret = alloc_chrdev_region(&peci_tool_dev, PECI_TOOL_MINOR, 1, PECI_TOOL_DEV_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate char device region: %d\n", ret);
        return ret;
    }

    /* init char device */
    cdev_init(&peci_tool_cdev, &peci_tool_fops);
    peci_tool_cdev.owner = THIS_MODULE;
    ret = cdev_add(&peci_tool_cdev, peci_tool_dev, 1);
    if (ret < 0) {
        pr_err("Failed to add char device: %d\n", ret);
        goto cdev_fail;
    }

    /* create device class */
    peci_tool_class = class_create(PECI_TOOL_DEV_NAME);
    if (IS_ERR(peci_tool_class)) {
        ret = PTR_ERR(peci_tool_class);
        pr_err("Failed to create device class: %d\n", ret);
        goto create_class_fail;
    }

    dev = device_create(peci_tool_class, NULL, peci_tool_dev, NULL, PECI_TOOL_DEV_NAME);
    if (IS_ERR(dev)) {
        ret = PTR_ERR(dev);
        pr_err("Failed to create device: %d\n", ret);
        goto create_device_fail;
    }

    crc8_populate_msb(peci_crc8_table, PECI_CRC8_POLYNOMIAL);

    DEBUG_VERBOSE("PECI Tool driver loaded (major: %d, minor: %d)\n",
            MAJOR(peci_tool_dev), MINOR(peci_tool_dev));
    DEBUG_VERBOSE("Device file created: /dev/%s\n", PECI_TOOL_DEV_NAME);
    pr_info("PECI Tool driver initialized successfully\n");

    return 0;
create_device_fail:
    class_destroy(peci_tool_class);
create_class_fail:
    cdev_del(&peci_tool_cdev);
cdev_fail:
    unregister_chrdev_region(peci_tool_dev, 1);
    return ret;
}

static void __exit peci_tool_exit(void)
{
    pr_info("Exiting PECI Tool driver...\n");

    /* destroy device file and class  */
    device_destroy(peci_tool_class, peci_tool_dev);
    class_destroy(peci_tool_class);

    /* delete char device */
    cdev_del(&peci_tool_cdev);

    /* free devno */
    unregister_chrdev_region(peci_tool_dev, 1);

    pr_info("PECI Tool driver exited successfully\n");
}

module_init(peci_tool_init);
module_exit(peci_tool_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PECI Tool Kernel Driver"); 
MODULE_AUTHOR("support");
