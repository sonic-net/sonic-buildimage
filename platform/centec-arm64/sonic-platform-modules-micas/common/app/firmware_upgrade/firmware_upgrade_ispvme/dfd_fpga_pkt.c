/*
 * dfd_fpga_pkt.c
 *
 * FPGA message interaction related interface
 *
 * History
 *    v1.0    support <support> 2013-10-25  Initial version.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/mman.h>

#include "dfd_fpga_pkt.h"
#include "dfd_fpga_debug.h"

#define DFD_FPGA_FAC_MODE_CONFIG_FILE            "/tmp/.factory_disabale_cli_tty"
#if 1
#define DFD_FPGA_PKT_SEND_PKT_TO_FRAME
#endif

void dfd_fpga_pkt_print(uint8_t *buf, int buf_len)
{
    int i;

    for (i = 0; i < buf_len; i++) {
        if ((i % 16) == 0) {
            DFD_DBG("\n");
        }
        DFD_DBG("%02x ", buf[i]);
    }

    DFD_DBG("\n");
    return;
}

static unsigned int littel_endian_byte_to_word32(uint8_t *byte_buf, int len)
{
    uint8_t tmp_buf[4];
    unsigned int word;

    word = 0;

    mem_clear(tmp_buf, 4);
    memcpy(tmp_buf, byte_buf, len < 4 ? len : 4);

    word = tmp_buf[0] | (tmp_buf[1] << 8) | (tmp_buf[2] << 16) | (tmp_buf[3] << 24);

    return word;
}

static int littel_endian_word32_to_byte(uint8_t *byte_buf, int len, unsigned int word)
{
    uint8_t tmp_buf[4];
    int ret;

    if (len < 4) {
        DFD_ERROR("Not enough buf, word32 to byte: len[%d], word[0x%x]\n");
        return -1;
    }

    mem_clear(tmp_buf, 4);
    tmp_buf[0] = word & 0xff;
    tmp_buf[1] = (word >> 8) & 0xff;
    tmp_buf[2] = (word >> 16) & 0xff;
    tmp_buf[3] = (word >> 24) & 0xff;

    memcpy(byte_buf, tmp_buf, 4);

    return 0;
}

static int open_pci_dev(dfd_pci_dev_priv_t *pci_priv, int is_cfg)
{
    int file, ret;
    char filename[DFD_PCI_MAX_NAME_SIZE];

    if (is_cfg) {
        ret = snprintf(filename, DFD_PCI_MAX_NAME_SIZE,
                       "/sys/class/pci_bus/%04x:%02x/device/%04x:%02x:%02x.%d/config",
                       0, pci_priv->pcibus, 0, pci_priv->pcibus, pci_priv->slot, pci_priv->fn);
    } else {
        ret = snprintf(filename, DFD_PCI_MAX_NAME_SIZE,
                       "/sys/class/pci_bus/%04x:%02x/device/%04x:%02x:%02x.%d/resource%d",
                       0, pci_priv->pcibus, 0, pci_priv->pcibus, pci_priv->slot, pci_priv->fn,
                       pci_priv->bar);
    }

    filename[ret] = '\0';
    if ((file = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        DFD_ERROR("Error: Could not open file %s\n", filename);
    }

    return file;
}

/**
 * dfd_fpga_buf_read - provide FPGA write register interface (address must be four-byte aligned)
 * @dst:  The data structure of sslot and unit corresponding to the target chip
 * @addr: address (four-byte alignment). For the byte alignment required by the specific FPGA address, please refer to the FPGA manual
 * @buf:  Buffer for reading data
 * @wr_len:  the length of reading data,please refer to the FPGA manual for the length of the specific FPGA address requirements
 * return: return if success,else return -1
 */
int dfd_fpga_pci_read(dfd_pci_dev_priv_t *pci_priv, int offset, uint8_t *buf, int rd_len)
{
    int ret, fd;
    unsigned int data;
    uint8_t *ptr, *ptr_data;
    struct stat sb;
    int i;
    int len, align;

    if ((pci_priv == NULL) || (buf == NULL)) {
        DFD_ERROR("pci_prive or read buf is null.\n");
        return -1;
    }

    if ((pci_priv->align < 1) || (offset & (pci_priv->align - 1)) || (rd_len & (pci_priv->align - 1))) {
        DFD_ERROR("offset[%d] or rd_len[%d] don't align[%d].\n", offset, rd_len, pci_priv->align);
        return -1;
    }

    if ((fd = open_pci_dev(pci_priv, 0)) < 0) {
        return -1;
    }

    if ((ret = fstat(fd, &sb)) == -1) {
        DFD_ERROR("Error: Could not fstat : %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (offset +  rd_len >= sb.st_size) {
        DFD_ERROR("Error: offset is out of range\n");
        close(fd);
        return -1;
    }

    if ((ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        DFD_ERROR("Error: Could not mmap : %s  or resource is IO\n", strerror(errno));
        close(fd);
        return -1;
    }

    align = pci_priv->align;
    len = rd_len;
    ret = 0;
    i = 0;
    ptr_data = ptr + offset;

    while((i < len) && (ret == 0)){
        if (align == 4) {
            data = *((volatile unsigned int *)(ptr_data + i));
            ret = littel_endian_word32_to_byte(buf + i, len - i, data);
            i += 4;
        } else {
            ret = -1;
        }
    }

    munmap(ptr, sb.st_size);
    close(fd);
    return ret;

}

/**
 * dfd_fpga_buf_write -provide FPGA write register interface (address must be four-byte aligned)
 * @dst:  The data structure of sslot and unit corresponding to the target chip
 * @addr: address (four-byte alignment). For the byte alignment required by the specific FPGA address, please refer to the FPGA manual
 * @buf:  Buffer for reading data
 * @wr_len:  the length of reading data,please refer to the FPGA manual for the length of the specific FPGA address requirements
 * return: return if success,else return -1
 */

int dfd_fpga_pci_write(dfd_pci_dev_priv_t *pci_priv, int offset, uint8_t *buf, int wr_len)
{
    int ret, fd;
    unsigned int data;
    uint8_t *ptr, *ptr_data;
    struct stat sb;
    int i;
    int len, align;

    if ((pci_priv == NULL) || (buf == NULL)) {
        DFD_ERROR("pci_prive or write buf is null.\n");
        return -1;
    }

    if ((pci_priv->align < 1) || (offset & (pci_priv->align - 1)) || (wr_len & (pci_priv->align - 1))) {
        DFD_ERROR("offset[%d] or rd_len[%d] don't align[%d].\n", offset, wr_len, pci_priv->align);
        return -1;
    }

    if ((fd = open_pci_dev(pci_priv, 0)) < 0) {
        return -1;
    }

    if ((ret = fstat(fd, &sb)) == -1) {
        DFD_ERROR("Error: Could not fstat : %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (offset +  wr_len >= sb.st_size) {
        DFD_ERROR("Error: offset is out of range\n");
        close(fd);
        return -1;
    }

    if ((ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        DFD_ERROR("Error: Could not mmap : %s  or resource is IO\n", strerror(errno));
        close(fd);
        return -1;
    }

    align = pci_priv->align;
    len = wr_len;
    ret = 0;
    i = 0;
    ptr_data = ptr + offset;

    while((i < len) && (ret == 0)){
        if (align == 4) {
            data = littel_endian_byte_to_word32(buf + i,len - i);
             *((volatile unsigned int *)(ptr_data + i)) = data;
            i += 4;
        } else {
            ret = -1;
        }
    }

    munmap(ptr, sb.st_size);
    close(fd);
    return ret;

}

/**
 * dfd_fpga_read_word -provide FPGA read register interface (address must be four-byte aligned)
 * @addr: address (four-byte alignment)
 * @val:  the returned number of reading
 * return: return 0 if success,else return failure
 */
int dfd_fpga_read_word(dfd_pci_dev_priv_t *pci_priv, int addr, int *val)
{
    int ret, i;
    uint8_t tmp[DFD_FPGA_PKT_WORD_LEN];

    if ((pci_priv == NULL) || (val == NULL) || (addr & 0x03)) {
        DFD_ERROR("Input para invalid pci_priv %p val %p addr 0x%x.\n", pci_priv, val, addr);
        return -1;
    }

    ret = dfd_fpga_pci_read(pci_priv, addr, tmp, DFD_FPGA_PKT_WORD_LEN);
    if (ret) {
        DFD_ERROR("dfd_fpga_pci_read addr 0x%x failed ret %d.\n", addr, ret);
        return ret;
    }

    *val = littel_endian_byte_to_word32(tmp,DFD_FPGA_PKT_WORD_LEN);
    for (i = 0; i < DFD_FPGA_PKT_WORD_LEN; i++) {
        DFD_VERBOS("tmp[%d]: 0x%x.\n", i, tmp[i]);
    }
    DFD_VERBOS("dfd_fpga_read_word addr 0x%x val 0x%x.\n", addr, *val);

    return 0;
}

/**
 * dfd_fpga_write_word -provide FPGA write register interface (address must be four-byte aligned)
 * @addr:  address (four-byte alignment)
 * @val:   Data written
 * return: return 0 if success,else return failure
 */
int dfd_fpga_write_word(dfd_pci_dev_priv_t *pci_priv, int addr, int val)
{
    int ret, i;
    uint8_t tmp[DFD_FPGA_PKT_WORD_LEN];

    if ((pci_priv == NULL) || (addr & 0x03)) {
        DFD_ERROR("Input para invalid pci_priv %p addr 0x%x.\n", pci_priv, addr);
        return -1;
    }

    littel_endian_word32_to_byte(tmp, DFD_FPGA_PKT_WORD_LEN, val);
    for (i = 0; i < DFD_FPGA_PKT_WORD_LEN; i++) {
        DFD_VERBOS("tmp[%d]: 0x%x.\n", i, tmp[i]);
    }

    ret = dfd_fpga_pci_write(pci_priv, addr, tmp, DFD_FPGA_PKT_WORD_LEN);
    if (ret) {
        DFD_ERROR("dfd_fpga_pci_write addr 0x%x failed ret %d.\n", addr, ret);
        return ret;
    }

    DFD_VERBOS("dfd_fpga_write_word addr 0x%x val 0x%x.\n", addr, val);
    return 0;
}