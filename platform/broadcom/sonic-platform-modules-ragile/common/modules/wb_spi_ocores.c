/*
 * wb_spi_ocores.c
 * ko to create ocores spi adapter
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "wb_spi_ocores.h"

#define SPIOC_WAIT_SCH        (5)
#define SPIOC_CONF            (0x00)
#define SPIOC_LSBF            BIT(0)    /* 0:MSB 1:LSB */
#define SPIOC_IDLE_LOW        BIT(1)
#define SPIOC_INTREN          BIT(2)    /* 0:enable 1:disabel */
#define SPIOC_DIV_MASK        (0xf0)
#define SPIOC_MAX_DIV         (0x0E)
#define SPIOC_DIV(div)        (((div) & 0x0f) << 4)

#define SPIOC_STS             (0x01)
#define SPIOC_INTR_STS        BIT(0)
#define SPIOC_BUSY_STS        BIT(1)
#define SPIOC_RXNUM_SHIFT     (4)
#define SPIOC_RXNUM_MASK      (0xf << SPIOC_RXNUM_SHIFT)
/* Just for read */
#define SPIOC_RXNUM(reg)      (((reg) & SPIOC_RXNUM_MASK) >> SPIOC_RXNUM_SHIFT )

#define SPIOC_TXTOT_NUM       (0x02)
#define SPIOC_TXNUM(reg)      ((reg) & 0x0f)
#define SPIOC_TOTNUM(reg)     (((reg) & 0x0f) << 4)

#define SPIOC_TXCTL           (0x03)
#define SPIOC_CSLV            BIT(0) /* 0:Deassert SPICS 1:Laeve SPICS */
#define SPIOC_TRSTART         BIT(1)
#define SPIOC_CSID_SHIFT      (5)
#define SPIOC_CSID_MASK       (0x7 << SPIOC_CSID_SHIFT)
/* Just for write */
#define SPIOC_CSID(id)        (((id) << SPIOC_CSID_SHIFT) & SPIOC_CSID_MASK)

/* Just single byte */
#define SPIOC_RX(i)           ((0x4) + i)
#define SPIOC_TX(i)           ((0x4) + i)

#define SPIOC_MAX_LEN         ((unsigned int)8)
#define SPIOC_TXRX_MAX_LEN    ((unsigned int)7)

#define MODEBITS              (SPI_CPHA |SPI_CPOL | SPI_LSB_FIRST |SPI_CS_HIGH)

#define REG_IO_WIDTH_1        (1)
#define REG_IO_WIDTH_2        (2)
#define REG_IO_WIDTH_4        (4)

#define SYMBOL_I2C_DEV_MODE   (1)
#define FILE_MODE             (2)
#define SYMBOL_PCIE_DEV_MODE  (3)
#define SYMBOL_IO_DEV_MODE    (4)

int g_spi_oc_debug = 0;
int g_spi_oc_error = 0;

module_param(g_spi_oc_debug, int, S_IRUGO | S_IWUSR);
module_param(g_spi_oc_error, int, S_IRUGO | S_IWUSR);

#define SPI_OC_VERBOSE(fmt, args...) do {                                        \
    if (g_spi_oc_debug) { \
        printk(KERN_INFO "[OC_SPI_BUS][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SPI_OC_ERROR(fmt, args...) do {                                        \
    if (g_spi_oc_error) { \
        printk(KERN_ERR "[OC_SPI_BUS][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct spioc {
    /* bitbang has to be first */
    struct spi_bitbang bitbang;
    int irq;
    struct completion done;
    unsigned int reamin_len;
    unsigned int cur_pos;
    unsigned int cur_len;
    const u8 *txp;
    u8 *rxp;
    u8 chip_select;
    void (*setreg)(struct spioc *spioc, int reg, u32 value);
    u32 (*getreg)(struct spioc *spioc, int reg);
    uint32_t bus_num;
    const char *dev_name;
    uint32_t reg_access_mode;
    uint32_t base_addr;
    uint32_t reg_shift;
    uint32_t reg_io_width;
    uint32_t num_chipselect;
    uint32_t freq;
    uint32_t big_endian;
    struct device *dev;
    int transfer_busy_flag;
};

extern int i2c_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int i2c_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int pcie_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int pcie_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int io_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int io_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);

static int oc_spi_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        SPI_OC_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        SPI_OC_ERROR("kernel_read failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }

    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int oc_spi_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{

    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        SPI_OC_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        SPI_OC_ERROR("kernel_write failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }

    vfs_fsync(filp, 1);
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int oc_spi_reg_write(struct spioc *spioc, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;

    switch (spioc->reg_access_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_write(spioc->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = oc_spi_file_write(spioc->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_write(spioc->dev_name, pos, val, size);
        break;
    case SYMBOL_IO_DEV_MODE:
        ret = io_device_func_write(spioc->dev_name, pos, val, size);
        break;
    default:
        SPI_OC_ERROR("err func_mode, write failed.\n");
        return -EINVAL;
    }

    return ret;
}

static int oc_spi_reg_read(struct spioc *spioc, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;

    switch (spioc->reg_access_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_read(spioc->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = oc_spi_file_read(spioc->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_read(spioc->dev_name, pos, val, size);
        break;
    case SYMBOL_IO_DEV_MODE:
        ret = io_device_func_read(spioc->dev_name, pos, val, size);
        break;
    default:
        SPI_OC_ERROR("err func_mode, read failed.\n");
        return -EINVAL;
    }

    return ret;
}

static void oc_spi_setreg_8(struct spioc *spioc, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_1];
    u32 pos;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);

    buf_tmp[0] = (value & 0Xff);
    oc_spi_reg_write(spioc, pos, buf_tmp, REG_IO_WIDTH_1);
    return;
}

static void oc_spi_setreg_16(struct spioc *spioc, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 pos;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);

    buf_tmp[0] = (value & 0Xff);
    buf_tmp[1] = (value >> 8) & 0xff;
    oc_spi_reg_write(spioc, pos, buf_tmp, REG_IO_WIDTH_2);
    return;
}

static void oc_spi_setreg_32(struct spioc *spioc, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 pos;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);

    buf_tmp[0] = (value & 0xff);
    buf_tmp[1] = (value >> 8) & 0xff;
    buf_tmp[2] = (value >> 16) & 0xff;
    buf_tmp[3] = (value >> 24) & 0xff;

    oc_spi_reg_write(spioc, pos, buf_tmp, REG_IO_WIDTH_4);
    return;
}

static void oc_spi_setreg_16be(struct spioc *spioc, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 pos;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);

    buf_tmp[0] = (value >> 8) & 0xff;
    buf_tmp[1] = (value & 0Xff);
    oc_spi_reg_write(spioc, pos, buf_tmp, REG_IO_WIDTH_2);
    return;
}

static void oc_spi_setreg_32be(struct spioc *spioc, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 pos;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);

    buf_tmp[0] = (value >> 24) & 0xff;
    buf_tmp[1] = (value >> 16) & 0xff;
    buf_tmp[2] = (value >> 8) & 0xff;
    buf_tmp[3] = (value & 0xff);
    oc_spi_reg_write(spioc, pos, buf_tmp, REG_IO_WIDTH_4);
    return;
}

static inline u32 oc_spi_getreg_8(struct spioc *spioc, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_1];
    u32 value, pos;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    oc_spi_reg_read(spioc, pos, buf_tmp, REG_IO_WIDTH_1);
    value = buf_tmp[0];

    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);

    return value;
}

static inline u32 oc_spi_getreg_16(struct spioc *spioc, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 value, pos;
    int i;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    mem_clear(buf_tmp, sizeof(buf_tmp));
    oc_spi_reg_read(spioc, pos, buf_tmp, REG_IO_WIDTH_2);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_2 ; i++) {
        value |= buf_tmp[i] << (8 * i);
    }

    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);
    return value;
}

static inline u32 oc_spi_getreg_32(struct spioc *spioc, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 value, pos;
    int i;

    pos = spioc->base_addr + (reg << spioc->reg_shift);
    mem_clear(buf_tmp, sizeof(buf_tmp));
    oc_spi_reg_read(spioc, pos, buf_tmp, REG_IO_WIDTH_4);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_4 ; i++) {
        value |= buf_tmp[i] << (8 * i);
    }
    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);
    return value;
}

static inline u32 oc_spi_getreg_16be(struct spioc *spioc, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 value, pos;
    int i;

    pos = spioc->base_addr + (reg << spioc->reg_shift);

    mem_clear(buf_tmp, sizeof(buf_tmp));
    oc_spi_reg_read(spioc, pos, buf_tmp, REG_IO_WIDTH_2);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_2 ; i++) {
        value |= buf_tmp[i] << (8 * (REG_IO_WIDTH_2 -i - 1));
    }

    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);
    return value;
}

static inline u32 oc_spi_getreg_32be(struct spioc *spioc, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 value, pos;
    int i;

    pos = spioc->base_addr + (reg << spioc->reg_shift);

    mem_clear(buf_tmp, sizeof(buf_tmp));
    oc_spi_reg_read(spioc, pos, buf_tmp, REG_IO_WIDTH_4);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_4 ; i++) {
        value |= buf_tmp[i] << (8 * (REG_IO_WIDTH_4 -i - 1));
    }

    SPI_OC_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
        spioc->dev_name, spioc->reg_access_mode, pos, value);
    return value;

}

static inline void oc_spi_setreg(struct spioc *spioc, int reg, u32 value)
{
    spioc->setreg(spioc, reg, value);
    return;
}

static inline u32 oc_spi_getreg(struct spioc *spioc, int reg)
{
    return spioc->getreg(spioc, reg);
}

static int spioc_get_clkdiv(struct spioc *spioc, u32 speed)
{
    u32 rate, div;

    rate = spioc->freq;
    SPI_OC_VERBOSE("clk get rate:%u, speed:%u\n", rate, speed);
    /* fs = fw/((DIV+2)*2) */

    if (speed > (rate / 4)) {
        div = 0;
        SPI_OC_VERBOSE("spi device speed[%u] more than a quarter of clk rate[%u].\n",
            speed, rate);
        return div;
    }
    div = (rate/(2 * speed)) - 2;
    if (div > SPIOC_MAX_DIV) {
        SPI_OC_ERROR("Unsupport spi device speed, div:%u.\n", div);
        return -1;
    }
    SPI_OC_VERBOSE("DIV is:0x%x\n", div);
    return div;
}

static inline int spioc_wait_trans(struct spioc *spioc, const unsigned long timeout)
{
    unsigned long j;
    unsigned int sch_time;
    u8 reg;

    j = jiffies + timeout;
    sch_time = SPIOC_WAIT_SCH;
    while (1) {
        reg = oc_spi_getreg(spioc, SPIOC_STS);
        if (!(reg & SPIOC_BUSY_STS)) {
            SPI_OC_VERBOSE("wait ok!\n");
            break;
        }

        if (time_after(jiffies, j)) {
            return -ETIMEDOUT;
        }

        usleep_range(sch_time, sch_time + 1);
    }

    return 0;
}

static void spioc_chipselect(struct spi_device *spi, int is_active)
{
    struct spioc *spioc;
    u8 tx_conf;
    int ret;

    spioc = spi_master_get_devdata(spi->master);
    spioc->transfer_busy_flag = 0;
    ret = spioc_wait_trans(spioc, msecs_to_jiffies(100));
    if (ret < 0) {
        SPI_OC_ERROR("spi transfer is busy, ret=%d.\n", ret);
        spioc->transfer_busy_flag = 1;
        return;
    }
    spioc->chip_select = spi->chip_select;
    SPI_OC_VERBOSE("spioc_chipselect:%u, value:%d.\n", spioc->chip_select, is_active);
    tx_conf = 0;
    tx_conf |= SPIOC_CSID(spioc->chip_select);
    if (is_active) {
        tx_conf |= SPIOC_CSLV;
    }

    SPI_OC_VERBOSE("tx_config:[0x%x]\n", tx_conf);
    oc_spi_setreg(spioc, SPIOC_TXCTL, tx_conf);
    return;
}

static void spioc_copy_tx(struct spioc *spioc)
{
    const u8 *src;
    int i;

    if (!spioc->txp) {
        SPI_OC_ERROR("spioc->txp is NULL.\n");
        return;
    }

    src = (u8 *)spioc->txp + spioc->cur_pos;
    SPI_OC_VERBOSE("current tx len:0x%x, tx pos:[0x%x]\n", spioc->cur_len, spioc->cur_pos);

    for (i = 0; i < spioc->cur_len; i++) {
        SPI_OC_VERBOSE("write %d, val:[0x%x]\n", i, src[i]);
        oc_spi_setreg(spioc, SPIOC_TX(i), src[i]);
    }
}

static void spioc_copy_rx(struct spioc *spioc)
{
    u8 *dest;
    int i;

    if (!spioc->rxp) {
        SPI_OC_ERROR("spioc->rxp is NULL.\n");
        return;
    }

    dest = (u8 *)spioc->rxp + spioc->cur_pos;
    SPI_OC_VERBOSE("current rx len:0x%x, rx pos:[0x%x]\n", spioc->cur_len, spioc->cur_pos);

    for (i = 0; i < spioc->cur_len; i++) {
        dest[i] = oc_spi_getreg(spioc, SPIOC_RX(i));
        SPI_OC_VERBOSE("read %d, val:[0x%x]\n", i, dest[i]);
    }
}

static int spioc_setup_transfer(struct spi_device *spi, struct spi_transfer *transfer)
{
    struct spioc *spioc;
    u8 ctrl;
    u32 hz;
    int div;

    spioc = spi_master_get_devdata(spi->master);
    ctrl = 0;

    if (spi->mode & SPI_LSB_FIRST) {
        ctrl |= SPIOC_LSBF;
    }

    if (!(spi->mode & SPI_CPOL)) {
        ctrl |= SPIOC_IDLE_LOW;
    }

    if (spioc->irq < 0) {

        ctrl |= SPIOC_INTREN;
    }

    if (transfer != NULL) {
        hz = transfer->speed_hz;

        if (hz == 0) {
            hz = spi->max_speed_hz;
        }
    } else {
        hz = spi->max_speed_hz;
    }

    if (hz == 0) {
        SPI_OC_ERROR("Unsupport zero speed.\n");
        return -EINVAL;
    }

    div = spioc_get_clkdiv(spioc, hz);
    if (div < 0) {
        SPI_OC_ERROR("get div error, div:%d.\n", div);
        return -EINVAL;
    }
    ctrl |= SPIOC_DIV(div);

    SPI_OC_VERBOSE("ctrl:[0x%x].\n", ctrl);

    oc_spi_setreg(spioc, SPIOC_CONF, ctrl);
    return 0;
}

static int spioc_spi_setup(struct spi_device *spi)
{
    struct spioc *spioc;

    if (!(spi->mode & SPI_CPHA)) {
        SPI_OC_ERROR("Unsupport spi device mde:0x%x, SPI_CPHA must be 1.\n", spi->mode);
        return -EINVAL;
    }

    spioc = spi_master_get_devdata(spi->master);
    if (spi->chip_select >= spioc->num_chipselect) {
        SPI_OC_ERROR("Spi device chipselect:%u, more than max chipselect:%u.\n",
            spi->chip_select, spioc->num_chipselect);
        return -EINVAL;
    }
    SPI_OC_VERBOSE("Support spi device mode:0x%x, chip_select:%u.\n",
        spi->mode, spi->chip_select);
    return 0;
}

static int spioc_transfer_start(struct spioc *spioc)
{
    u8 tx_conf;
    int ret;

    tx_conf = oc_spi_getreg(spioc, SPIOC_TXCTL);
    tx_conf |= SPIOC_TRSTART;

    SPI_OC_VERBOSE("tx_config:[0x%x]\n", tx_conf);
    oc_spi_setreg(spioc, SPIOC_TXCTL, tx_conf);

    ret = spioc_wait_trans(spioc, msecs_to_jiffies(100));
    return ret;
}

static int spioc_tx_start_one(struct spioc *spioc)
{
    unsigned int txlen;
    u8 txreg;
    int ret;

    if (!spioc->reamin_len) {
        SPI_OC_VERBOSE("spioc txlen:[0x0]\n");
        return 0;
    }

    spioc->cur_len = spioc->reamin_len > SPIOC_MAX_LEN ? SPIOC_MAX_LEN : spioc->reamin_len;

    txlen = spioc->cur_len;
    spioc->reamin_len -= txlen;
    SPI_OC_VERBOSE("txlen:[0x%x], tx len remain:[0x%x]\n", txlen, spioc->reamin_len);

    spioc_copy_tx(spioc);

    /* when we only send, txlen == totlen */
    txreg = SPIOC_TXNUM(txlen) | SPIOC_TOTNUM(txlen);
    SPI_OC_VERBOSE("txreg:[0x%x]\n", txreg);
    oc_spi_setreg(spioc, SPIOC_TXTOT_NUM, txreg);

    ret = spioc_transfer_start(spioc);
    if (ret) {
        SPI_OC_ERROR("spioc tx rx poll wait for transfer timeout.\n");
        return ret;
    }

    if (spioc->reamin_len) {
        spioc->cur_pos += txlen;
        SPI_OC_VERBOSE("cur_txpos:[0x%x]\n", spioc->cur_pos);
    }

    return 0;
}

static int spioc_rx_start_one(struct spioc *spioc)
{
    unsigned int rxlen;
    u8 txtnum;
    int ret;

    if (!spioc->reamin_len) {
        SPI_OC_VERBOSE("spioc reamin_len:[0x0]\n");
        return 0;
    }

    spioc->cur_len = spioc->reamin_len > SPIOC_MAX_LEN ? SPIOC_MAX_LEN : spioc->reamin_len;

    rxlen = spioc->cur_len;
    spioc->reamin_len -= rxlen;
    SPI_OC_VERBOSE("rxlen:[0x%x], rx len remain:[0x%x]\n", rxlen, spioc->reamin_len);

    /* when we only receive, rxnum=totnum. txnum=0 */
    txtnum = SPIOC_TOTNUM(rxlen);
    SPI_OC_VERBOSE("tx total reg:0x%x\n", txtnum);
    oc_spi_setreg(spioc, SPIOC_TXTOT_NUM, txtnum);

    ret = spioc_transfer_start(spioc);
    if (ret) {
        SPI_OC_ERROR("spioc tx rx poll wait for transfer timeout.\n");
        return ret;
    }

    spioc_copy_rx(spioc);

    if (spioc->reamin_len) {
        spioc->cur_pos += rxlen;
        SPI_OC_VERBOSE("cur_rxpos:[0x%x]\n", spioc->cur_pos);
    }

    return 0;
}

static int spioc_tx_rx_start_one(struct spioc *spioc)
{
    unsigned int txlen, total_len;
    u8 txreg;
    int ret;

    if (!spioc->reamin_len) {
        SPI_OC_VERBOSE("spioc reamin_len:[0x0]\n");
        return 0;
    }

    spioc->cur_len = spioc->reamin_len > SPIOC_TXRX_MAX_LEN ? SPIOC_TXRX_MAX_LEN : spioc->reamin_len;

    txlen = spioc->cur_len;
    spioc->reamin_len -= txlen;
    SPI_OC_VERBOSE("tx len:[0x%x], tx len remain:[0x%x]\n", txlen, spioc->reamin_len);

    spioc_copy_tx(spioc);

    total_len = 2 * txlen;      /* total_len=txlen + rxlen; rxlen=txlen */
    txreg = SPIOC_TXNUM(txlen) | SPIOC_TOTNUM(total_len);
    SPI_OC_VERBOSE("txreg:[0x%x]\n", txreg);
    oc_spi_setreg(spioc, SPIOC_TXTOT_NUM, txreg);

    ret = spioc_transfer_start(spioc);
    if (ret) {
        SPI_OC_ERROR("spioc tx rx poll wait for transfer timeout.\n");
        return ret;
    }

    spioc_copy_rx(spioc);
    if (spioc->reamin_len) {
        spioc->cur_pos += txlen;
        SPI_OC_VERBOSE("cur_txrx pos:[0x%x]\n", spioc->cur_pos);
    }
    return 0;
}

static int spioc_spi_txrx_bufs(struct spi_device *spi, struct spi_transfer *t)
{
    struct spioc *spioc;
    int ret , len;

    if(!t->len || (!t->tx_buf && !t->rx_buf)) {
        SPI_OC_ERROR("params error, tx_buf and rx_buf may both NULL, transfer len:0x%x.\n",
            t->len);
        return 0;
    }

    spioc = spi_master_get_devdata(spi->master);
    if (spioc->transfer_busy_flag) {
        ret = -EBUSY;
        goto err;
    }

    spioc->txp = t->tx_buf;
    spioc->rxp = t->rx_buf;
    spioc->reamin_len = t->len;
    spioc->cur_len = 0;
    spioc->cur_pos = 0;
    len = t->len;
    ret = 0;
    if (spioc->irq >= 0) {
        /* use interrupt driven data transfer */
        if (t->tx_buf && t->rx_buf) {
            spioc_tx_rx_start_one(spioc);
            wait_for_completion(&spioc->done);
        } else if (t->tx_buf) {
            spioc_tx_start_one(spioc);
            wait_for_completion(&spioc->done);

        } else {
            spioc_rx_start_one(spioc);
            wait_for_completion(&spioc->done);
        }
    } else {
        if (t->tx_buf && t->rx_buf) {
            SPI_OC_VERBOSE("start tx rx, len:0x%x\n", t->len);
            while (spioc->reamin_len) {
                ret = spioc_tx_rx_start_one(spioc);
                if (ret) {
                    goto err;
                }
            }
        } else if (t->tx_buf) {
            SPI_OC_VERBOSE("start tx, txlen:0x%x\n", t->len);
            while (spioc->reamin_len) {
                ret = spioc_tx_start_one(spioc);
                if (ret) {
                    goto err;
                }
            }
        } else {
            SPI_OC_VERBOSE("start rx, rxlen:0x%x\n", t->len);
            while (spioc->reamin_len) {
                ret = spioc_rx_start_one(spioc);
                if (ret) {
                    goto err;
                }
            }
        }
    }
    SPI_OC_VERBOSE("return num: 0x%x\n", len);
    return len;
err:
    return ret;
}

static irqreturn_t spioc_spi_irq(int irq, void *dev)
{
    struct spioc *spioc;

    spioc = dev;
    /* gooooohi, interrupt status bit judgment is not done */

    if (spioc->txp && spioc->rxp) {
        if (!spioc->reamin_len) {
            complete(&spioc->done);
        } else {
            spioc_tx_rx_start_one(spioc);
        }
    } else if (spioc->txp) {
        if (!spioc->reamin_len) {
            complete(&spioc->done);
        } else {
            spioc_tx_start_one(spioc);
        }
    } else if (spioc->rxp){
        if (!spioc->reamin_len) {
            complete(&spioc->done);
        } else {
            spioc_rx_start_one(spioc);
        }
    }

    return IRQ_HANDLED;
}

static int ocores_spi_config_init(struct spioc *spioc)
{
    int ret = 0;
    struct device *dev;
    spi_ocores_device_t *spi_ocores_device;

    dev = spioc->dev;
    if (dev->of_node) {
        ret += of_property_read_string(dev->of_node, "dev_name", &spioc->dev_name);
        ret += of_property_read_u32(dev->of_node, "dev_base", &spioc->base_addr);
        ret += of_property_read_u32(dev->of_node, "reg_shift", &spioc->reg_shift);
        ret += of_property_read_u32(dev->of_node, "reg_io_width", &spioc->reg_io_width);
        ret += of_property_read_u32(dev->of_node, "clock-frequency", &spioc->freq);
        ret += of_property_read_u32(dev->of_node, "reg_access_mode", &spioc->reg_access_mode);
        ret += of_property_read_u32(dev->of_node, "num_chipselect", &spioc->num_chipselect);

        if (ret != 0) {
            SPI_OC_ERROR("dts config error, ret:%d.\n", ret);
            ret = -ENXIO;
            return ret;
        }
    } else {
        if (spioc->dev->platform_data == NULL) {
            SPI_OC_ERROR("platform data config error.\n");
            ret = -ENXIO;
            return ret;
        }
        spi_ocores_device = spioc->dev->platform_data;
        spioc->bus_num = spi_ocores_device->bus_num;
        spioc->dev_name = spi_ocores_device->dev_name;
        spioc->big_endian = spi_ocores_device->big_endian;
        spioc->base_addr = spi_ocores_device->dev_base;
        spioc->reg_shift = spi_ocores_device->reg_shift;
        spioc->reg_io_width = spi_ocores_device->reg_io_width;
        spioc->freq = spi_ocores_device->clock_frequency;
        spioc->reg_access_mode = spi_ocores_device->reg_access_mode;
        spioc->num_chipselect = spi_ocores_device->num_chipselect;
    }

    SPI_OC_VERBOSE("name:%s, base:0x%x, reg_shift:0x%x, io_width:0x%x, clock-frequency:0x%x.\n",
        spioc->dev_name, spioc->base_addr, spioc->reg_shift, spioc->reg_io_width, spioc->freq);
    SPI_OC_VERBOSE("reg access mode:%u, num_chipselect:%u.\n",
        spioc->reg_access_mode, spioc->num_chipselect);
    return ret;
}

static int spioc_probe(struct platform_device *pdev)
{
    struct spi_master *master;
    struct spioc *spioc;
    int ret;
    bool be;

    ret = -1;
    master = spi_alloc_master(&pdev->dev, sizeof(struct spioc));
    if (!master) {
        dev_err(&pdev->dev, "Failed to alloc spi master.\n");
        goto out;
    }

    spioc = spi_master_get_devdata(master);
    platform_set_drvdata(pdev, spioc);

    spioc->dev = &pdev->dev;
    ret = ocores_spi_config_init(spioc);
    if (ret != 0) {
        dev_err(spioc->dev, "Failed to get ocores spi dts config.\n");
        goto free;
    }

    if (spioc->dev->of_node) {
        if (of_property_read_u32(spioc->dev->of_node, "big_endian", &spioc->big_endian)) {

            be = 0;
        } else {
            be = spioc->big_endian;
        }
    } else {
        be = spioc->big_endian;
    }

    if (spioc->reg_io_width == 0) {
        spioc->reg_io_width = 1; /* Set to default value */
    }

    if (!spioc->setreg || !spioc->getreg) {
        switch (spioc->reg_io_width) {
        case REG_IO_WIDTH_1:
            spioc->setreg = oc_spi_setreg_8;
            spioc->getreg = oc_spi_getreg_8;
            break;

        case REG_IO_WIDTH_2:
            spioc->setreg = be ? oc_spi_setreg_16be : oc_spi_setreg_16;
            spioc->getreg = be ? oc_spi_getreg_16be : oc_spi_getreg_16;
            break;

        case REG_IO_WIDTH_4:
            spioc->setreg = be ? oc_spi_setreg_32be : oc_spi_setreg_32;
            spioc->getreg = be ? oc_spi_getreg_32be : oc_spi_getreg_32;
            break;

        default:
            dev_err(spioc->dev, "Unsupported I/O width (%d)\n", spioc->reg_io_width);
            ret = -EINVAL;
            goto free;
        }
    }

    /* master state */
    master->num_chipselect = spioc->num_chipselect;
    master->mode_bits = MODEBITS;
    master->setup = spioc_spi_setup;
    if (spioc->dev->of_node) {
        master->dev.of_node = pdev->dev.of_node;
    } else {
        master->bus_num = spioc->bus_num;
    }

    /* setup the state for the bitbang driver */
    spioc->bitbang.master = master;
    spioc->bitbang.setup_transfer = spioc_setup_transfer;
    spioc->bitbang.chipselect = spioc_chipselect;
    spioc->bitbang.txrx_bufs = spioc_spi_txrx_bufs;

    /* gooooohi need revision */
    spioc->irq = platform_get_irq(pdev, 0);
    if (spioc->irq >= 0) {
        SPI_OC_VERBOSE("spi oc use irq, irq number:%d.\n", spioc->irq);
        init_completion(&spioc->done);
        ret = devm_request_irq(&pdev->dev, spioc->irq, spioc_spi_irq, 0,
                               pdev->name, spioc);
        if (ret) {
            dev_err(spioc->dev, "Failed to request irq:%d.\n", spioc->irq);
            goto free;
        }
    }

    ret = spi_bitbang_start(&spioc->bitbang);
    if (ret) {
        dev_err(spioc->dev, "Failed to start spi bitbang, ret:%d.\n", ret);
        goto free;
    }
    dev_info(spioc->dev, "registered spi-%d for %s with base address:0x%x success.\n",
        master->bus_num, spioc->dev_name, spioc->base_addr);

    return ret;
free:
    spi_master_put(master);
out:
    return ret;
}

static int spioc_remove(struct platform_device *pdev)
{
    struct spioc *spioc;
    struct spi_master *master;

    spioc = platform_get_drvdata(pdev);
    master = spioc->bitbang.master;
    spi_bitbang_stop(&spioc->bitbang);
    platform_set_drvdata(pdev, NULL);
    spi_master_put(master);

    return 0;
}

static const struct of_device_id spioc_match[] = {
    { .compatible = "wb-spi-oc", },
    {},
};
MODULE_DEVICE_TABLE(of, spioc_match);

static struct platform_driver spioc_driver = {
    .probe = spioc_probe,
    .remove = spioc_remove,
    .driver = {
        .name  = "wb-spioc",
        .owner = THIS_MODULE,
        .of_match_table = spioc_match,
    },
};

module_platform_driver(spioc_driver);

MODULE_DESCRIPTION("spi open core adapter driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
