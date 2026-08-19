/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#include "ixgbe.h"

#ifdef HAVE_IXGBE_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/module.h>

static struct dentry *ixgbe_dbg_root;

#define IXGBE_FW_DUMP_BLK_MAX	0xFFFF
#define IXGBE_FW_DUMP_DATA_SIZE	4096
#define IXGBE_FW_DUMP_HDR_SIZE	24
/* dump blk = Cluster header + dump data */
#define IXGBE_FW_DUMP_BLK_SIZE	(IXGBE_FW_DUMP_DATA_SIZE + IXGBE_FW_DUMP_HDR_SIZE)
#define IXGBE_FW_DUMP_FILENAME	"debug_dump"
#define IXGBE_FW_DUMP_LAST_IDX	0xFFFFFFFF
#define IXGBE_FW_DUMP_LAST_ID	0xFF
#define IXGBE_FW_DUMP_LAST_ID2	0xFFFF

static char ixgbe_dbg_reg_ops_buf[256] = "";

/* the ordering in this array is important. it matches the ordering of the
 * values in the FW so the index is the same value as in ice_aqc_fw_logging_mod
 */
static const char * const ixgbe_fwlog_module_string[] = {
	"GENERAL",
	"CTRL",
	"LINK",
	"LINK_TOPO",
	"DNL",
	"I2C",
	"SDP",
	"MDIO",
	"ADMINQ",
	"HDMA",
	"LLDP",
	"DCBX",
	"DCB",
	"XLR",
	"NVM",
	"AUTH",
	"VPD",
	"IOSF",
	"PARSER",
	"SW",
	"SCHEDULER",
	"TXQ",
	"RSVD",
	"POST",
	"WATCHDOG",
	"TASK_DISPATCH",
	"MNG",
	"SYNCE",
	"HEALTH",
	"TSDRV",
	"PFREG",
	"MDLVER",
	"ALL",
};

/* the ordering in this array is important. it matches the ordering of the
 * values in the FW so the index is the same value as in ice_fwlog_level
 */
static const char * const ixgbe_fwlog_level_string[] = {
	"NONE",
	"ERROR",
	"WARNING",
	"NORMAL",
	"VERBOSE",
};

static void ixgbe_print_fwlog_config(struct ixgbe_hw *hw, struct ixgbe_fwlog_cfg *cfg,
				     char **buff, int *size)
{
	char *tmp = *buff;
	int used = *size;
	u16 i, len;

	len = snprintf(tmp, used, "Log_resolution: %d\n", cfg->log_resolution);
	tmp = tmp + len;
	used -= len;
	len = snprintf(tmp, used, "Options: 0x%04x\n", cfg->options);
	tmp = tmp + len;
	used -= len;
	len = snprintf(tmp, used, "\tarq_ena: %s\n",
		       (cfg->options &
		       IXGBE_FWLOG_OPTION_ARQ_ENA) ? "true" : "false");
	tmp = tmp + len;
	used -= len;
	len = snprintf(tmp, used, "\tuart_ena: %s\n",
		       (cfg->options &
		       IXGBE_FWLOG_OPTION_UART_ENA) ? "true" : "false");
	tmp = tmp + len;
	used -= len;
	len = snprintf(tmp, used, "\trunning: %s\n",
		       (cfg->options &
		       IXGBE_FWLOG_OPTION_IS_REGISTERED) ? "true" : "false");
	tmp = tmp + len;
	used -= len;
	len = snprintf(tmp, used, "Module Entries:\n");
	tmp = tmp + len;
	used -= len;

	for (i = 0; i < IXGBE_ACI_FW_LOG_ID_MAX; i++) {
		struct ixgbe_fwlog_module_entry *entry =
			&cfg->module_entries[i];

		/* TODO: Remove this hack when SyncE gets turned on always */
		if (i == IXGBE_ACI_FW_LOG_ID_SYNCE)
			continue;

		len = snprintf(tmp, used, "\tModule: %s, Log Level: %s\n",
			       ixgbe_fwlog_module_string[entry->module_id],
			       ixgbe_fwlog_level_string[entry->log_level]);
		tmp = tmp + len;
		used -= len;
	}

	len = snprintf(tmp, used, "Valid log levels:\n");
	tmp = tmp + len;
	used -= len;

	for (i = 0; i < IXGBE_FWLOG_LEVEL_INVALID; i++) {
		len = snprintf(tmp, used, "\t%s\n", ixgbe_fwlog_level_string[i]);
		tmp = tmp + len;
		used -= len;
	}

	*buff = tmp;
	*size = used;
}

/**
 * ixgbe_fwlog_dump_cfg - Dump current FW logging configuration
 * @hw: pointer to the HW structure
 * @buff: pointer to a buffer to hold the config strings
 * @buff_size: size of the buffer in bytes
 */
static void ixgbe_fwlog_dump_cfg(struct ixgbe_hw *hw, char *buff, int buff_size)
{
	int len;

	len = snprintf(buff, buff_size, "FWLOG Configuration:\n");
	buff = buff + len;
	buff_size -= len;

	ixgbe_print_fwlog_config(hw, &hw->fwlog_cfg, &buff, &buff_size);
}

/**
 * ixgbe_debugfs_parse_cmd_line - Parse the command line that was passed in
 * @src: pointer to a buffer holding the command line
 * @len: size of the buffer in bytes
 * @argv: pointer to store the command line items
 * @argc: pointer to store the number of command line items
 */
static ssize_t ixgbe_debugfs_parse_cmd_line(const char __user *src, size_t len,
					    char ***argv, int *argc)
{
	char *cmd_buf, *cmd_buf_tmp;

	cmd_buf = memdup_user(src, len + 1);
	if (IS_ERR(cmd_buf))
		return PTR_ERR(cmd_buf);
	cmd_buf[len] = '\0';

	/** the cmd_buf has a newline at the end of the command so
	 * remove it
	 */
	cmd_buf_tmp = strchr(cmd_buf, '\n');
	if (cmd_buf_tmp) {
		*cmd_buf_tmp = '\0';
		len = (size_t)cmd_buf_tmp - (size_t)cmd_buf + 1;
	}

	*argv = argv_split(GFP_KERNEL, cmd_buf, argc);
	if (!*argv)
		return -ENOMEM;

	kfree(cmd_buf);
	return 0;
}

/**
 * ixgbe_dbg_reg_ops_read - Read reg_ops data for debugging
 * @filp: The opened file
 * @buffer: Buffer where the data is written for the user to read
 * @count: Size of the user's buffer
 * @ppos: File position offset
 *
 * This function reads the reg_ops data for debugging purposes from the
 * ixgbe network adapter. It formats the data into a string and writes it to
 * the user's buffer. The function does not allow partial reads and returns
 * an error if the user's buffer is too small to hold the data.
 *
 * Return: Number of bytes read on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_dbg_reg_ops_read(struct file *filp, char __user *buffer,
				    size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: %s\n",
			adapter->netdev->name,
			ixgbe_dbg_reg_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

	kfree(buf);
	return len;
}

/**
 * ixgbe_dbg_reg_ops_write - Write data to reg_ops for debugging
 * @filp: The opened file
 * @buffer: Buffer containing the user's data
 * @count: Length of the user's data
 * @ppos: File position offset
 *
 * This function writes user-provided data to the reg_ops buffer for
 * debugging purposes. It processes specific commands, such as "read" and
 * "write", to perform register operations on the ixgbe network adapter. The
 * function does not allow partial writes and returns an error if the data
 * exceeds the buffer size. If an unknown command is received, it logs
 * available commands.
 *
 * Return: Number of bytes written on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_dbg_reg_ops_write(struct file *filp,
				     const char __user *buffer,
				     size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(ixgbe_dbg_reg_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(ixgbe_dbg_reg_ops_buf,
				     sizeof(ixgbe_dbg_reg_ops_buf)-1,
				     ppos,
				     buffer,
				     count);
	if (len < 0)
		return len;

	ixgbe_dbg_reg_ops_buf[len] = '\0';

	if (strncmp(ixgbe_dbg_reg_ops_buf, "write", 5) == 0) {
		u32 reg, value;
		int cnt;
		cnt = sscanf(&ixgbe_dbg_reg_ops_buf[5], "%x %x", &reg, &value);
		/* check format and bounds check register access */
		if (cnt == 2 && reg <= IXGBE_HFDR) {
			IXGBE_WRITE_REG(&adapter->hw, reg, value);
			value = IXGBE_READ_REG(&adapter->hw, reg);
			e_dev_info("write: 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("write <reg> <value>\n");
		}
	} else if (strncmp(ixgbe_dbg_reg_ops_buf, "read", 4) == 0) {
		u32 reg, value;
		int cnt;
		cnt = sscanf(&ixgbe_dbg_reg_ops_buf[4], "%x", &reg);
		/* check format and bounds check register access */
		if (cnt == 1 && reg <= IXGBE_HFDR) {
			value = IXGBE_READ_REG(&adapter->hw, reg);
			e_dev_info("read 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("read <reg>\n");
		}
	} else {
		e_dev_info("Unknown command %s\n", ixgbe_dbg_reg_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("   read <reg>\n");
		e_dev_info("   write <reg> <value>\n");
	}
	return count;
}

static const struct file_operations ixgbe_dbg_reg_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read =  ixgbe_dbg_reg_ops_read,
	.write = ixgbe_dbg_reg_ops_write,
};

static char ixgbe_dbg_netdev_ops_buf[256] = "";

/**
 * ixgbe_dbg_netdev_ops_read - Read netdev_ops data for debugging
 * @filp: The opened file
 * @buffer: Buffer where the data is written for the user to read
 * @count: Size of the user's buffer
 * @ppos: File position offset
 *
 * This function reads the netdev_ops data for debugging purposes from the
 * ixgbe network adapter. It formats the data into a string and writes it to
 * the user's buffer. The function does not allow partial reads and returns
 * an error if the user's buffer is too small to hold the data.
 *
 * Return: Number of bytes read on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_dbg_netdev_ops_read(struct file *filp,
					 char __user *buffer,
					 size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: %s\n",
			adapter->netdev->name,
			ixgbe_dbg_netdev_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

	kfree(buf);
	return len;
}

/**
 * ixgbe_dbg_netdev_ops_write - Write data to netdev_ops for debugging
 * @filp: The opened file
 * @buffer: Buffer containing the user's data
 * @count: Length of the user's data
 * @ppos: File position offset
 *
 * This function writes user-provided data to the netdev_ops buffer for
 * debugging purposes. It processes specific commands, such as "tx_timeout",
 * to trigger corresponding network device operations. The function does not
 * allow partial writes and returns an error if the data exceeds the buffer
 * size. If an unknown command is received, it logs available commands.
 *
 * Return: Number of bytes written on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_dbg_netdev_ops_write(struct file *filp,
					  const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(ixgbe_dbg_netdev_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(ixgbe_dbg_netdev_ops_buf,
				     sizeof(ixgbe_dbg_netdev_ops_buf)-1,
				     ppos,
				     buffer,
				     count);
	if (len < 0)
		return len;

	ixgbe_dbg_netdev_ops_buf[len] = '\0';

	if (strncmp(ixgbe_dbg_netdev_ops_buf, "tx_timeout", 10) == 0) {
#ifdef HAVE_NET_DEVICE_OPS
#ifdef HAVE_TX_TIMEOUT_TXQUEUE
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev,
							    UINT_MAX);
#else
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev);
#endif
#else
		adapter->netdev->tx_timeout(adapter->netdev);
#endif /* HAVE_NET_DEVICE_OPS */
		e_dev_info("tx_timeout called\n");
	} else {
		e_dev_info("Unknown command: %s\n", ixgbe_dbg_netdev_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("    tx_timeout\n");
	}
	return count;
}

static struct file_operations ixgbe_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ixgbe_dbg_netdev_ops_read,
	.write = ixgbe_dbg_netdev_ops_write,
};

struct ixgbe_cluster_header {
	u32 cluster_id;
	u32 table_id;
	u32 table_len;
	u32 table_offset;
	u32 reserved[2];
};

/**
 * ixgbe_get_last_table_id - get a value that should be used as End of Table
 * @adapter: pointer to adapter struct
 *
 * Different versions of FW may indicate End Of Table by different value. Read
 * FW capabilities and decide what value to use as End of Table.
 *
 * Return: end of table identifier.
 */
static u16 ixgbe_get_last_table_id(struct ixgbe_adapter *adapter)
{
	if (adapter->hw.func_caps.common_cap.next_cluster_id_support ||
	    adapter->hw.dev_caps.common_cap.next_cluster_id_support)
		return IXGBE_FW_DUMP_LAST_ID2;
	else
		return IXGBE_FW_DUMP_LAST_ID;
}

/**
 * ixgbe_debugfs_fw_dump - send request to FW to dump cluster and save to file
 * @adapter: pointer to adapter struct
 * @cluster_id: number or FW cluster to be dumped
 * @read_all_clusters: ignore cluster_id and dump all clusters
 *
 * Create FW configuration binary snapshot. Repeatedly send ACI requests to dump
 * FW cluster, FW responds in 4KB blocks and sets new values for table_id
 * and table_idx. Repeat until all tables in given cluster were read.
 *
 * Return: 0 on success or error code on failure.
 */
static int ixgbe_debugfs_fw_dump(struct ixgbe_adapter *adapter,
				 u16 cluster_id, bool read_all_clusters)
{
	u16 buf_len, next_tbl_id, next_cluster_id, last_tbl_id, tbl_id = 0;
	u32 next_blk_idx, blk_idx = 0, ntw = 0, ctw = 0, offset = 0;
	struct debugfs_blob_wrapper *desc_blob;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_cluster_header header = {};
	struct dentry *pfile;
	u8 *vblk;
	int i;

	desc_blob = devm_kzalloc(dev, sizeof(*desc_blob), GFP_KERNEL);
	if (!desc_blob)
		return -ENOMEM;

	vfree(adapter->ixgbe_cluster_blk);
	adapter->ixgbe_cluster_blk = NULL;

	vblk = vmalloc(IXGBE_FW_DUMP_BLK_MAX * IXGBE_FW_DUMP_BLK_SIZE);
	if (!vblk)
		return -ENOMEM;

	last_tbl_id = ixgbe_get_last_table_id(adapter);

	for (i = 0; i < IXGBE_FW_DUMP_BLK_MAX; i++) {
		int res;

		/* Skip the header bytes */
		ntw += sizeof(struct ixgbe_cluster_header);

		res = ixgbe_aci_get_internal_data(&adapter->hw, cluster_id,
						  tbl_id, blk_idx, vblk + ntw,
						  IXGBE_FW_DUMP_DATA_SIZE,
						  &buf_len, &next_cluster_id,
						  &next_tbl_id, &next_blk_idx);

		if (res) {
			dev_err(dev, "Internal FW error %d while dumping cluster %d\n",
				res, cluster_id);
			devm_kfree(dev, desc_blob);
			vfree(vblk);
			return -EINVAL;
		}
		ntw += buf_len;

		header.cluster_id = cluster_id;
		header.table_id = tbl_id;
		header.table_len = buf_len;
		header.table_offset = offset;

		memcpy(vblk + ctw, &header, sizeof(header));
		ctw = ntw;
		memset(&header, 0, sizeof(header));

		offset += buf_len;

		if (blk_idx == next_blk_idx)
			blk_idx = IXGBE_FW_DUMP_LAST_IDX;
		else
			blk_idx = next_blk_idx;

		if (blk_idx != IXGBE_FW_DUMP_LAST_IDX)
			continue;

		blk_idx = 0;
		offset = 0;

		if (next_cluster_id == IXGBE_FW_DUMP_LAST_ID2)
			break;

		if (next_tbl_id != last_tbl_id)
			tbl_id = next_tbl_id;

		/* End of cluster */
		if (cluster_id != next_cluster_id) {
			if (read_all_clusters) {
				dev_info(dev, "All FW clusters dump - cluster %d appended",
					 cluster_id);
				cluster_id = next_cluster_id;
				tbl_id = 0;
			} else {
				break;
			}
		}
	}

	desc_blob->size = (unsigned long)ntw;
	desc_blob->data = vblk;

	pfile = debugfs_create_blob(IXGBE_FW_DUMP_FILENAME, 0400,
				    adapter->ixgbe_dbg_adapter_fw_cluster, desc_blob);
	if (!pfile)
		return -ENODEV;

	adapter->ixgbe_cluster_blk = vblk;

	if (read_all_clusters)
		dev_info(dev, "Created FW dump of all available clusters in file %s\n",
			 IXGBE_FW_DUMP_FILENAME);
	else
		dev_info(dev, "Created FW dump of cluster %d in file %s\n",
			 cluster_id, IXGBE_FW_DUMP_FILENAME);

	return 0;
}

/**
 * dump_cluster_id_read - show currently set FW cluster id to dump
 * @file: kernel file struct
 * @buf: user space buffer to fill with correct data
 * @len: buf's length
 * @offset: current position in buf
 */
static ssize_t dump_cluster_id_read(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	struct ixgbe_adapter *adapter = file->private_data;
	char tmp_buf[11];
	int ret;

	ret = snprintf(tmp_buf, sizeof(tmp_buf), "%u\n",
		       adapter->fw_dump_cluster_id);

	return simple_read_from_buffer(buf, len, offset, tmp_buf, ret);
}

/**
 * dump_cluster_id_write - set FW cluster id to dump
 * @file: kernel file struct
 * @buf: user space buffer containing data to read
 * @len: buf's length
 * @offset: current position in buf
 */
static ssize_t dump_cluster_id_write(struct file *file, const char __user *buf,
				     size_t len, loff_t *offset)
{
	struct ixgbe_adapter *adapter = file->private_data;
	bool read_all_clusters = false;
	char kbuf[11] = { 0 };
	int bytes_read, err;
	u16 cluster_id;

	bytes_read = simple_write_to_buffer(kbuf, sizeof(kbuf), offset, buf,
					    len);

	if (bytes_read < 0)
		return -EINVAL;

	if (bytes_read == 1 && kbuf[0] == '\n') {
		cluster_id = 0;
		read_all_clusters = true;
	} else {
		err = kstrtou16(kbuf, 10, &cluster_id);
		if (err)
			return err;
	}

	debugfs_lookup_and_remove(IXGBE_FW_DUMP_FILENAME, adapter->ixgbe_dbg_adapter_fw_cluster);
	err = ixgbe_debugfs_fw_dump(adapter, cluster_id, read_all_clusters);
	if (err)
		return err;

	/* Not all cluster IDs are supported in every FW version, save
	 * the value only when FW returned success
	 */
	adapter->fw_dump_cluster_id = cluster_id;

	return bytes_read;
}

static const struct file_operations dump_cluster_id_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = dump_cluster_id_read,
	.write = dump_cluster_id_write,
};

/**
 * ixgbe_debugfs_enable_read - read from 'enable' file
 * @file: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 */
static ssize_t ixgbe_debugfs_enable_read(struct file *file,
					 char __user *buffer, size_t count,
					 loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct ixgbe_hw *hw = &adapter->hw;
	char buff[32] = {};
	int status;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	snprintf(buff, sizeof(buff), "%u\n",
		 (u16)(hw->fwlog_cfg.options &
		 IXGBE_FWLOG_OPTION_IS_REGISTERED) >> 3);

	status = simple_read_from_buffer(buffer, count, ppos, buff,
					 strlen(buff));

	return status;
}

/**
 * ixgbe_debugfs_enable_write - write into 'enable' file
 * @file: the opened file
 * @buf: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 */
static ssize_t ixgbe_debugfs_enable_write(struct file *file, const char __user *buf,
					  size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	ssize_t ret;
	char **argv;
	int argc;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	ret = ixgbe_debugfs_parse_cmd_line(buf, count, &argv, &argc);
	if (ret)
		goto err_copy_from_user;

	if (argc == 1) {
		bool enable;

		ret = kstrtobool(argv[0], &enable);
		if (ret)
			goto enable_write_error;

		if (enable)
			hw->fwlog_cfg.options |= IXGBE_FWLOG_OPTION_ARQ_ENA;
		else
			hw->fwlog_cfg.options &= ~IXGBE_FWLOG_OPTION_ARQ_ENA;

		ret = ixgbe_fwlog_set(hw, &hw->fwlog_cfg);
		if (ret)
			goto enable_write_error;

		if (enable)
			ret = ixgbe_fwlog_register(hw);
		else
			ret = ixgbe_fwlog_unregister(hw);

		if (ret)
			goto enable_write_error;
	} else {
		dev_info(dev, "unknown or invalid command '%s'\n", argv[0]);
		ret = -EINVAL;
		goto enable_write_error;
	}

	/* if we get here, nothing went wrong; return bytes copied */
	ret = (ssize_t)count;

enable_write_error:
	argv_free(argv);
err_copy_from_user:
	/* This function always consumes all of the written input, or produces
	 * an error. Check and enforce this. Otherwise, the write operation
	 * won't complete properly.
	 */
	if (WARN_ON(ret != (ssize_t)count && ret >= 0))
		ret = -EIO;

	return ret;
}

static const struct file_operations ixgbe_debugfs_enable_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ixgbe_debugfs_enable_read,
	.write = ixgbe_debugfs_enable_write,
};

/**
 * ixgbe_debugfs_data_read - Read data from the 'data' debugfs file
 * @file: The opened file
 * @buffer: Buffer where the data is written for the user to read
 * @count: Size of the user's buffer
 * @ppos: File position offset
 *
 * This function reads firmware log data from the 'data' debugfs file for
 * the ixgbe network adapter. It checks if the firmware supports logging and
 * if there is data available in the log ring. The function copies available
 * log data to the user's buffer, handling partial reads and errors
 * gracefully. It updates the file position and returns the number of bytes
 * copied.
 *
 * Return: Number of bytes read on success, or a negative error code if
 *         unsupported.
 */
static ssize_t ixgbe_debugfs_data_read(struct file *file, char __user *buffer,
				       size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct ixgbe_hw *hw = &adapter->hw;
	int data_copied = 0;
	bool done = false;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	if (ixgbe_fwlog_ring_empty(&hw->fwlog_ring))
		return 0;

	while (!ixgbe_fwlog_ring_empty(&hw->fwlog_ring) && !done) {
		struct ixgbe_fwlog_data *log;
		u16 cur_buf_len;

		log = &hw->fwlog_ring.rings[hw->fwlog_ring.head];
		cur_buf_len = log->data_size;

		if (cur_buf_len >= count) {
			done = true;
			continue;
		}

		if (copy_to_user(buffer, log->data, cur_buf_len)) {
			/* if there is an error then bail and return whatever
			 * the driver has copied so far
			 */
			done = true;
			continue;
		}

		data_copied += cur_buf_len;
		buffer += cur_buf_len;
		count -= cur_buf_len;
		*ppos += cur_buf_len;
		ixgbe_fwlog_ring_increment(&hw->fwlog_ring.head,
					   hw->fwlog_ring.size);
	}

	return data_copied;
}

/**
 * ixgbe_debugfs_data_write - Write data to the 'data' debugfs file
 * @file: The opened file
 * @buf: Buffer containing the user's data
 * @count: Length of the user's data
 * @ppos: File position offset
 *
 * This function writes user-provided data to the 'data' debugfs file for
 * the ixgbe network adapter. It processes commands related to firmware log
 * management, such as clearing the log if the firmware log is not running.
 * The function does not allow partial writes and returns an error if the
 * command is invalid or unsupported. It ensures that all input is consumed
 * or an error is returned.
 *
 * Return: Number of bytes written on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_debugfs_data_write(struct file *file, const char __user *buf, size_t count,
					loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	ssize_t ret;
	char **argv;
	int argc;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	ret = ixgbe_debugfs_parse_cmd_line(buf, count, &argv, &argc);
	if (ret)
		goto err_copy_from_user;

	if (argc == 1) {
		if (!(hw->fwlog_cfg.options & IXGBE_FWLOG_OPTION_IS_REGISTERED)) {
			hw->fwlog_ring.head = 0;
			hw->fwlog_ring.tail = 0;
		} else {
			dev_info(dev, "Can't clear FW log data while FW log running\n");
			ret = -EINVAL;
			goto nr_buffs_write_error;
		}
	} else {
		dev_info(dev, "unknown or invalid command '%s'\n", argv[0]);
		ret = -EINVAL;
		goto nr_buffs_write_error;
	}

	/* if we get here, nothing went wrong; return bytes copied */
	ret = (ssize_t)count;

nr_buffs_write_error:
	argv_free(argv);
err_copy_from_user:
	/* This function always consumes all of the written input, or produces
	 * an error. Check and enforce this. Otherwise, the write operation
	 * won't complete properly.
	 */
	if (WARN_ON(ret != (ssize_t)count && ret >= 0))
		ret = -EIO;

	return ret;
}

static const struct file_operations ixgbe_debugfs_data_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ixgbe_debugfs_data_read,
	.write = ixgbe_debugfs_data_write,
};

/**
 * ixgbe_debugfs_nr_buffs_read - Read from the 'nr_buffs' debugfs file
 * @file: The opened file
 * @buffer: Buffer where the data is written for the user to read
 * @count: Size of the user's buffer
 * @ppos: File position offset
 *
 * This function reads the number of buffers from the 'nr_buffs' debugfs
 * file for the ixgbe network adapter. It checks if the firmware supports
 * logging and formats the buffer size into a string for the user to read.
 * The function returns an error if the firmware does not support logging.
 *
 * Return: Number of bytes read on success, or a negative error code if
 *         unsupported.
 */
static ssize_t ixgbe_debugfs_nr_buffs_read(struct file *file,
					   char __user *buffer, size_t count,
					   loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct ixgbe_hw *hw = &adapter->hw;
	char buff[32] = {};
	int status;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	snprintf(buff, sizeof(buff), "%d\n", hw->fwlog_ring.size);

	status = simple_read_from_buffer(buffer, count, ppos, buff,
					 strlen(buff));

	return status;
}

/**
 * ixgbe_debugfs_nr_buffs_write - Write data to the 'nr_buffs' debugfs file
 * @file: The opened file
 * @buf: Buffer containing the user's data
 * @count: Length of the user's data
 * @ppos: File position offset
 *
 * This function writes user-provided data to the 'nr_buffs' debugfs file for
 * the ixgbe network adapter. It processes commands to set the number of
 * buffers for firmware logging, ensuring the value is within bounds and a
 * power of two. The function does not allow partial writes and returns an
 * error if the command is invalid or if firmware logging is active. It
 * ensures that all input is consumed or an error is returned.
 *
 * Return: Number of bytes written on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_debugfs_nr_buffs_write(struct file *file, const char __user *buf,
					    size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	ssize_t ret;
	char **argv;
	int argc;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	ret = ixgbe_debugfs_parse_cmd_line(buf, count, &argv, &argc);
	if (ret)
		goto err_copy_from_user;

	if (argc == 1) {
		s16 nr_buffs;

		ret = kstrtos16(argv[0], 0, &nr_buffs);
		if (ret)
			goto nr_buffs_write_error;

		if (nr_buffs <= 0 || nr_buffs > IXGBE_FWLOG_RING_SIZE_MAX) {
			dev_info(dev, "nr_buffs '%d' is not within bounds. Please use a value between 1 and %d\n",
				 nr_buffs, IXGBE_FWLOG_RING_SIZE_MAX);
			ret = -EINVAL;
			goto nr_buffs_write_error;
		} else if (hweight16(nr_buffs) > 1) {
			dev_info(dev, "nr_buffs '%d' is not a power of 2. Please use a value that is a power of 2.\n",
				 nr_buffs);
			ret = -EINVAL;
			goto nr_buffs_write_error;
		} else if (hw->fwlog_cfg.options &
			   IXGBE_FWLOG_OPTION_IS_REGISTERED) {
			dev_info(dev, "FW logging is currently running. Please disable FW logging to change nr_buffs\n");
			ret = -EINVAL;
			goto nr_buffs_write_error;
		}

		/* free all the buffers and the tracking info and resize */
		ixgbe_fwlog_realloc_rings(hw, nr_buffs);
	} else {
		dev_info(dev, "unknown or invalid command '%s'\n", argv[0]);
		ret = -EINVAL;
		goto nr_buffs_write_error;
	}

	/* if we get here, nothing went wrong; return bytes copied */
	ret = (ssize_t)count;

nr_buffs_write_error:
	argv_free(argv);
err_copy_from_user:
	/* This function always consumes all of the written input, or produces
	 * an error. Check and enforce this. Otherwise, the write operation
	 * won't complete properly.
	 */
	if (WARN_ON(ret != (ssize_t)count && ret >= 0))
		ret = -EIO;

	return ret;
}

static const struct file_operations ixgbe_debugfs_nr_buffs_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read = ixgbe_debugfs_nr_buffs_read,
	.write = ixgbe_debugfs_nr_buffs_write,
};

/**
 * ixgbe_debugfs_module_read - Read from the 'module' debugfs file
 * @file: The opened file
 * @buffer: Buffer where the data is written for the user to read
 * @count: Size of the user's buffer
 * @ppos: File position offset
 *
 * This function reads firmware module configuration data from the 'module'
 * debugfs file for the ixgbe network adapter. It checks if the firmware
 * supports logging and allocates memory to store the configuration data.
 * The function writes the data to the user's buffer and returns an error if
 * the buffer is too small or if memory allocation fails.
 *
 * Return: Number of bytes read on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_debugfs_module_read(struct file *file, char __user *buffer,
					 size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	char *data = NULL;
	int status;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	data = vzalloc(PAGE_SIZE);
	if (!data) {
		dev_warn(dev, "Unable to allocate memory for FW configuration!\n");
		return -ENOMEM;
	}

	ixgbe_fwlog_dump_cfg(hw, data, PAGE_SIZE);

	if (count < strlen(data))
		return -ENOSPC;

	status = simple_read_from_buffer(buffer, count, ppos, data,
					 strlen(data));
	vfree(data);

	return status;
}

/**
 * ixgbe_debugfs_module_write - Write data to the 'module' debugfs file
 * @file: The opened file
 * @buf: Buffer containing the user's data
 * @count: Length of the user's data
 * @ppos: File position offset
 *
 * This function writes user-provided data to the 'module' debugfs file for
 * the ixgbe network adapter. It processes commands to set the log level for
 * specific firmware modules, ensuring the module and log level are valid.
 * The function does not allow partial writes and returns an error if the
 * command is invalid or unsupported. It ensures that all input is consumed
 * or an error is returned.
 *
 * Return: Number of bytes written on success, or a negative error code on
 *         failure.
 */
static ssize_t ixgbe_debugfs_module_write(struct file *file, const char __user *buf,
					  size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	ssize_t ret;
	char **argv;
	int argc;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	ret = ixgbe_debugfs_parse_cmd_line(buf, count, &argv, &argc);
	if (ret)
		goto err_copy_from_user;

	if (argc == 2) {
		int module, log_level;

		module = sysfs_match_string(ixgbe_fwlog_module_string, argv[0]);
		if (module < 0) {
			dev_info(dev, "unknown module '%s'\n", argv[0]);
			ret = -EINVAL;
			goto module_write_error;
		}

		/* TODO: remove this hack when SyncE gets turned on always */
		if (module == IXGBE_ACI_FW_LOG_ID_SYNCE) {
			ret = -EINVAL;
			goto module_write_error;
		}

		log_level = sysfs_match_string(ixgbe_fwlog_level_string, argv[1]);
		if (log_level < 0) {
			dev_info(dev, "unknown log level '%s'\n", argv[1]);
			ret = -EINVAL;
			goto module_write_error;
		}

		/* module is valid because it was range checked using
		 * sysfs_match_string()
		 */
		if (module != IXGBE_ACI_FW_LOG_ID_MAX) {
			ixgbe_pf_fwlog_update_modules(adapter, log_level, 1 << module);
		} else {
			/* the module 'ALL' is a shortcut so that we can set
			 * all of the modules to the same level quickly
			 */
			int i;

			for (i = 0; i < IXGBE_ACI_FW_LOG_ID_MAX; i++)
				ixgbe_pf_fwlog_update_modules(adapter, log_level, 1 << i);
		}
	} else {
		dev_info(dev, "unknown or invalid command '%s'\n", argv[0]);
		ret = -EINVAL;
		goto module_write_error;
	}

	/* if we get here, nothing went wrong; return bytes copied */
	ret = (ssize_t)count;

module_write_error:
	argv_free(argv);
err_copy_from_user:
	/* This function always consumes all of the written input, or produces
	 * an error. Check and enforce this. Otherwise, the write operation
	 * won't complete properly.
	 */
	if (WARN_ON(ret != (ssize_t)count && ret >= 0))
		ret = -EIO;

	return ret;
}

static const struct file_operations ixgbe_dbg_module_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read = ixgbe_debugfs_module_read,
	.write = ixgbe_debugfs_module_write,
};

/**
 * ixgbe_debugfs_resolution_read - Read from the 'resolution' debugfs file
 * @file: The opened file
 * @buffer: Buffer where the data is written for the user to read
 * @count: Size of the user's buffer
 * @ppos: File position offset
 *
 * This function reads the log resolution from the 'resolution' debugfs file
 * for the ixgbe network adapter. It checks if the firmware supports logging
 * and formats the log resolution into a string for the user to read. The
 * function returns an error if the firmware does not support logging.
 *
 * Return: Number of bytes read on success, or a negative error code if
 *         unsupported.
 */
static ssize_t ixgbe_debugfs_resolution_read(struct file *file,
					     char __user *buffer, size_t count,
					     loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct ixgbe_hw *hw = &adapter->hw;
	char buff[32] = {};
	int status;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	snprintf(buff, sizeof(buff), "%d\n",
		 hw->fwlog_cfg.log_resolution);

	status = simple_read_from_buffer(buffer, count, ppos, buff,
					 strlen(buff));

	return status;
}

/**
 * ixgbe_debugfs_resolution_write - Write data to the 'resolution' debugfs file
 * @file: The opened file
 * @buf: Buffer containing the user's data
 * @count: Length of the user's data
 * @ppos: File position offset
 *
 * This function writes user-provided data to the 'resolution' debugfs file
 * for the ixgbe network adapter. It processes commands to set the firmware
 * log resolution, ensuring the value is within valid bounds. The function
 * does not allow partial writes and returns an error if the command is
 * invalid or unsupported. It ensures that all input is consumed or an error
 * is returned.
 *
 * Return: Number of bytes written on success, or a negative error code on
 *         failure.
 */
static ssize_t
ixgbe_debugfs_resolution_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct ixgbe_adapter *adapter = file->private_data;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	ssize_t ret;
	char **argv;
	int argc;

	/* don't allow commands if the FW doesn't support it */
	if (!ixgbe_fwlog_supported(hw))
		return -EOPNOTSUPP;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	ret = ixgbe_debugfs_parse_cmd_line(buf, count, &argv, &argc);
	if (ret)
		goto err_copy_from_user;

	if (argc == 1) {
		s16 resolution;

		ret = kstrtos16(argv[0], 0, &resolution);
		if (ret)
			goto resolution_write_error;

		if (resolution < IXGBE_ACI_FW_LOG_MIN_RESOLUTION ||
		    resolution > IXGBE_ACI_FW_LOG_MAX_RESOLUTION) {
			dev_err(dev, "Invalid FW log resolution %d, value must be between %d - %d\n",
				resolution, IXGBE_ACI_FW_LOG_MIN_RESOLUTION,
				IXGBE_ACI_FW_LOG_MAX_RESOLUTION);
			ret = -EINVAL;
			goto resolution_write_error;
		}

		hw->fwlog_cfg.log_resolution = resolution;
	} else {
		dev_info(dev, "unknown or invalid command '%s'\n", argv[0]);
		ret = -EINVAL;
		goto resolution_write_error;
	}

	/* if we get here, nothing went wrong; return bytes copied */
	ret = (ssize_t)count;

resolution_write_error:
	argv_free(argv);
err_copy_from_user:
	/* This function always consumes all of the written input, or produces
	 * an error. Check and enforce this. Otherwise, the write operation
	 * won't complete properly.
	 */
	if (WARN_ON(ret != (ssize_t)count && ret >= 0))
		ret = -EIO;

	return ret;
}

static const struct file_operations ixgbe_dbg_resolution_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read = ixgbe_debugfs_resolution_read,
	.write = ixgbe_debugfs_resolution_write,
};

/**
 * ixgbe_dbg_adapter_init - Set up the debugfs directory for the adapter
 * @adapter: The adapter that is starting up
 *
 * This function initializes the debugfs directory structure for the given
 * ixgbe network adapter. It creates various debugfs entries for monitoring
 * and controlling the adapter, including firmware logs, module settings,
 * and register operations. The function handles errors in creating these
 * entries and cleans up if any creation fails.
 */
void ixgbe_dbg_adapter_init(struct ixgbe_adapter *adapter)
{
	const char *name = pci_name(adapter->pdev);

	adapter->ixgbe_dbg_adapter_pf = debugfs_create_dir(name, ixgbe_dbg_root);
	if (!adapter->ixgbe_dbg_adapter_pf) {
		e_dev_err("debugfs pf entry for %s failed\n", name);
		return;
	}

	if (ixgbe_is_mac_E6xx(adapter->hw.mac.type)) {
		adapter->ixgbe_dbg_adapter_fw =
			debugfs_create_dir("fwlog", adapter->ixgbe_dbg_adapter_pf);
		if (!adapter->ixgbe_dbg_adapter_fw) {
			e_dev_err("debugfs fwlog entry for %s failed\n", name);
			return;
		}

		adapter->ixgbe_dbg_adapter_fw_cluster =
			debugfs_create_dir("fw", adapter->ixgbe_dbg_adapter_pf);
		if (!adapter->ixgbe_dbg_adapter_fw_cluster) {
			e_dev_err("debugfs fw entry for %s failed\n", name);
			return;
		}

		if (!debugfs_create_file("dump_cluster_id", 0600,
					 adapter->ixgbe_dbg_adapter_fw_cluster,
					 adapter,
					 &dump_cluster_id_fops)) {
			e_dev_err("debugfs dump_cluster_id for %s failed\n", name);
			goto create_failed;
		}

		if (!debugfs_create_file("enable", 0600,
					 adapter->ixgbe_dbg_adapter_fw,
					 adapter,
					 &ixgbe_debugfs_enable_fops)) {
			e_dev_err("debugfs enable for %s failed\n", name);
			goto create_failed;
		}

		if (!debugfs_create_file("data", 0600,
					 adapter->ixgbe_dbg_adapter_fw,
					 adapter,
					 &ixgbe_debugfs_data_fops)) {
			e_dev_err("debugfs data for %s failed\n", name);
			goto create_failed;
		}

		if (!debugfs_create_file("modules", 0600,
					 adapter->ixgbe_dbg_adapter_fw,
					 adapter,
					 &ixgbe_dbg_module_fops)) {
			e_dev_err("debugfs modules for %s failed\n", name);
			goto create_failed;
		}

		if (!debugfs_create_file("resolution", 0600,
					 adapter->ixgbe_dbg_adapter_fw,
					 adapter,
					 &ixgbe_dbg_resolution_fops)) {
			e_dev_err("debugfs resolution for %s failed\n", name);
			goto create_failed;
		}

		if (!debugfs_create_file("nr_buffs", 0600,
					 adapter->ixgbe_dbg_adapter_fw,
					 adapter,
					 &ixgbe_debugfs_nr_buffs_fops)) {
			e_dev_err("debugfs nr_buffs for %s failed\n", name);
			goto create_failed;
		}
	}

	if (!debugfs_create_file("reg", 0600,
				 adapter->ixgbe_dbg_adapter_pf,
				 adapter,
				 &ixgbe_dbg_reg_ops_fops)) {
		e_dev_err("debugfs reg for %s failed\n", name);
		goto create_failed;
	}

	if (!debugfs_create_file("netdev", 0600,
				 adapter->ixgbe_dbg_adapter_pf,
				 adapter,
				 &ixgbe_dbg_netdev_ops_fops)) {
		e_dev_err("debugfs netdev for %s failed\n", name);
		goto create_failed;
	}

	return;

create_failed:
	debugfs_remove_recursive(adapter->ixgbe_dbg_adapter_pf);
	adapter->ixgbe_dbg_adapter_pf = NULL;
}

/**
 * ixgbe_dbg_adapter_exit - Clear the adapter's debugfs entries
 * @adapter: Board private structure
 *
 * This function removes the debugfs entries associated with the given ixgbe
 * network adapter. It recursively deletes the debugfs directory and frees
 * any allocated memory for cluster blocks. This ensures that all debugfs
 * resources are properly cleaned up when the adapter is being shut down or
 * removed.
 */
void ixgbe_dbg_adapter_exit(struct ixgbe_adapter *adapter)
{
	if (adapter->ixgbe_dbg_adapter_pf)
		debugfs_remove_recursive(adapter->ixgbe_dbg_adapter_pf);
	adapter->ixgbe_dbg_adapter_pf = NULL;

	vfree(adapter->ixgbe_cluster_blk);
	adapter->ixgbe_cluster_blk = NULL;
}

/**
 * ixgbe_dbg_init - Create root directory for debugfs entries
 *
 * This function initializes the root directory for debugfs entries related
 * to the ixgbe network driver. It creates a directory named after the
 * driver, which serves as the root for all subsequent debugfs entries. If
 * the directory creation fails, an error message is logged.
 */
void ixgbe_dbg_init(void)
{
	ixgbe_dbg_root = debugfs_create_dir(ixgbe_driver_name, NULL);
	if (ixgbe_dbg_root == NULL)
		pr_err("init of debugfs failed\n");
}

/**
 * ixgbe_dbg_exit - Clean out the driver's debugfs entries
 *
 * This function removes the root directory and all associated debugfs
 * entries for the ixgbe network driver. It ensures that all debugfs
 * resources are properly cleaned up when the driver is being unloaded or
 * shut down.
 */
void ixgbe_dbg_exit(void)
{
	debugfs_remove_recursive(ixgbe_dbg_root);
}

#endif /* HAVE_IXGBE_DEBUG_FS */
