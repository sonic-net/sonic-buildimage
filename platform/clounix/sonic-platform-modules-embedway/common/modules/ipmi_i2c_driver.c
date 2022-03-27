/*
 * Virtual i2c adpater, transfer by ipmi(OEM i2c cmd)
 1.简单i2c sensor(操作只需一条i2c ioctl)直接基于该adapter加载device驱动，
   i2c ioctl的i2c_bus_lock可以保证跟BMC侧的device驱动不会冲突
 2.复杂i2c sensor(如xdpe122/raa228涉及切page，操作需要多条i2c ioctl)，如果加载device驱动会跟BMC侧冲突，
   只能从ipmitool sensor list抓取，再给到对应/sys/bus/i2c节点
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include "ipmi_def.h"

static int log_level = LOG_INFO;
module_param_named(loglevel, log_level, int, 0640);
MODULE_PARM_DESC(loglevel, "driver log level, 0:off 1:err 2:info 3:debug.");

//i2c
#define I2C_MAX_BUS 27
#define IPMI_I2C_MSG_MAX 255

struct ipmi_i2c_device {
	struct ipmi_user *user;
	struct ipmi_user_hndl user_hndl;
	struct i2c_adapter adap[I2C_MAX_BUS];
};

typedef struct {
	u8 adap_bus;
	u8 bmc_bus;
} bus_map;

static struct ipmi_i2c_device ipmi_i2c_data;
static bus_map bus_map_list[I2C_MAX_BUS] = {
	{.adap_bus=1 ,.bmc_bus=20 },
	{.adap_bus=2 ,.bmc_bus=21 },
	{.adap_bus=3 ,.bmc_bus=22 },
	{.adap_bus=4 ,.bmc_bus=23 },
	{.adap_bus=5 ,.bmc_bus=24 },
	{.adap_bus=6 ,.bmc_bus=25 },
	{.adap_bus=7 ,.bmc_bus=26 },
	{.adap_bus=8 ,.bmc_bus=27 },
	{.adap_bus=9 ,.bmc_bus=100},
	{.adap_bus=10,.bmc_bus=101},
	{.adap_bus=11,.bmc_bus=102},
	{.adap_bus=12,.bmc_bus=103},
	{.adap_bus=13,.bmc_bus=104},
	{.adap_bus=14,.bmc_bus=105},
	{.adap_bus=15,.bmc_bus=106},
	{.adap_bus=16,.bmc_bus=107},
	{.adap_bus=17,.bmc_bus=140},
	{.adap_bus=18,.bmc_bus=141},
	{.adap_bus=19,.bmc_bus=142},
	{.adap_bus=20,.bmc_bus=143},
	{.adap_bus=21,.bmc_bus=144},
	{.adap_bus=22,.bmc_bus=145},
	{.adap_bus=23,.bmc_bus=146},
	{.adap_bus=24,.bmc_bus=147},
	{.adap_bus=25,.bmc_bus=150},
	{.adap_bus=26,.bmc_bus=151},
	{.adap_bus=27,.bmc_bus=152}
};

/************************* IPMI_I2C 通信部分 **************************/

// IPMI_I2C消息处理回调
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data)
{
    int size;
	recv_user_data *user = (recv_user_data *)msg->user_msg_data;

	if (user) {
		//for(i=0;i <msg->msg.data_len;i++ ) {
		//	log_dbg(IPMI_I2C, "%02x ",msg->msg.data[i]);
		//}
		if (msg->msg.data_len > 0) {
			if (msg->msg.data[0] != 0) {
				log_dbg(IPMI_I2C, "Command not completed normally\n");
				user->recv_len = -1;
			} else {
				if (msg->msg.data_len > 1) {
					size = msg->msg.data_len - 1;
					if (size > user->read_byte)
						size = user->read_byte;
					memcpy(user->data, &msg->msg.data[1], size);
					user->recv_len = size;
				} else {
					//write operation with no recv
					user->recv_len = 0;
				}
			}
		} else {
			log_dbg(IPMI_I2C, "ipmi recv msg invalid len\n");
		}
		complete(&user->comp);
	}
    ipmi_free_recv_msg(msg);
}

#define BUS_IDX 0
#define ADDR_IDX 1
#define R_BYTE_IDX 2
#define CMD_IDX 3
#define W_DATA_IDX 4
/*
for ipmi i2c
	cmd[0] = bus
	cmd[1] = slave addr
	cmd[2] = read byte(fixed to 0 for write opearation)
	cmd[3...] = write data
	EX1:read 10 byte from EEPROM(0x57) addr(0x000F) on bus 23
	->23 0x57 10 0x0 0x1
	EX1:write 10 byte(0x11~0XAA) to EEPROM(0x57) addr(0x000F) on bus 23
	->23 0x57 0 0x0 0x1 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88 0x99 0xAA
*/
static int ipmi_i2c_req_wait(struct ipmi_user *user, u8 *w_data, u8 w_data_len, u8 *r_data, u8 *r_data_len)
{
	recv_user_data user_data;
	int ret;
	int i = 0;

    struct ipmi_system_interface_addr addr = {
        .addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
        .channel = IPMI_BMC_CHANNEL,
        .lun = 0
    };
    
    struct kernel_ipmi_msg msg = {
        .netfn = IPMI_NETFN_OEM_I2C,
        .cmd = IPMI_OEM_I2C_CMD,
        .data = w_data,
        .data_len = w_data_len
    };

    if (w_data == NULL || r_data == NULL || r_data_len == NULL || user == NULL) {
		return -EINVAL;
    }

    if (w_data_len < (R_BYTE_IDX + 1)) {
		return -EINVAL;
    }

	log_dbg(IPMI_I2C, "ipmi TX:");
	if (log_level >= LOG_DEBUG) {
		for (i=0;i< w_data_len;i++) {
			log_dbg_cont("%02x ",w_data[i]);
		}
	}
	log_dbg_cont("\n");

    user_data.data = r_data;
    user_data.read_byte = w_data[R_BYTE_IDX];
    init_completion(&user_data.comp);
    ret = ipmi_request_supply_msgs(user,
                                  (struct ipmi_addr *)&addr,
                                  0,
                                  &msg,
                                  &user_data,
                                  NULL,
                                  NULL,
                                  0);
	if (ret) {
		return ret;
	}
	wait_for_completion(&user_data.comp);
	if (user_data.recv_len < 0) {
		return -EIO;
	} else {
		*r_data_len = (u8)user_data.recv_len;
	}

	log_dbg(IPMI_I2C, "ipmi RX:");
	if (log_level >= LOG_DEBUG) {
		for (i=0;i< *r_data_len;i++) {
			log_dbg_cont("%02x ",r_data[i]);
		}
	}
	log_dbg_cont("\n\n");
	
	return ret;
}

static int ipmi_xfer(struct i2c_adapter *adap, u16 addr,
		       unsigned short flags, char read_write, u8 command,
		       int size, union i2c_smbus_data *data)
{
		
	int ret;
	u8 w_buf[I2C_SMBUS_BLOCK_MAX+W_DATA_IDX] = {0};
	u8 r_buf[I2C_SMBUS_BLOCK_MAX] = {0};
	u8 w_len,r_len;
	__le16 word;
	struct ipmi_user *user = (struct ipmi_user *)adap->algo_data;
	bus_map *map = (bus_map *)i2c_get_adapdata(adap);

	log_dbg(IPMI_I2C, "[SMBUS] %s addr 0x%x flags 0x%x cmd 0x%x size %d\n",
		read_write == I2C_SMBUS_WRITE ? "write" : "read",
		addr, flags, command, size);

	w_len = 0;
	w_buf[BUS_IDX] = map->bmc_bus;w_len++;
	w_buf[ADDR_IDX] = (u8)addr;w_len++;	

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (I2C_SMBUS_READ == read_write) {
			w_buf[R_BYTE_IDX] = 1;w_len++;
		} else {
			w_buf[R_BYTE_IDX] = 0;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
		}			
		break;
	case I2C_SMBUS_BYTE_DATA:
		if (I2C_SMBUS_READ == read_write) {
			w_buf[R_BYTE_IDX] = 1;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
		} else {
			w_buf[R_BYTE_IDX] = 0;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
			w_buf[W_DATA_IDX] = data->byte;w_len++;
		}
		break;
	case I2C_SMBUS_WORD_DATA:
		word = cpu_to_le16(data->word);
		if (I2C_SMBUS_READ == read_write) {
			w_buf[R_BYTE_IDX] = 2;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
		} else {
			w_buf[R_BYTE_IDX] = 0;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
			memcpy(&w_buf[W_DATA_IDX], (u8 *)&word, 2);
			w_len += 2;
		}
		break;
	case I2C_SMBUS_PROC_CALL:
		size = I2C_SMBUS_WORD_DATA;
		read_write = I2C_SMBUS_READ;
		word = cpu_to_le16(data->word);

		w_buf[R_BYTE_IDX] = 2;w_len++;
		w_buf[CMD_IDX] = command;w_len++;
		memcpy(&w_buf[W_DATA_IDX], (u8 *)&word, 2);
		w_len += 2;
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (I2C_SMBUS_READ == read_write) {
			w_buf[R_BYTE_IDX] = data->block[0];w_len++;
			w_buf[CMD_IDX] = command;w_len++;
		} else {
			w_buf[R_BYTE_IDX] = 0;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
			if (data->block[0] > I2C_SMBUS_BLOCK_MAX - 1) {
				return -EINVAL;
			}
			memcpy(&w_buf[W_DATA_IDX], data->block + 1, data->block[0]);
			w_len += data->block[0];
		}
		break;
	case I2C_SMBUS_BLOCK_DATA:
		if (I2C_SMBUS_READ == read_write) {
			w_buf[R_BYTE_IDX] = I2C_SMBUS_BLOCK_MAX;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
		} else {
			w_buf[R_BYTE_IDX] = 0;w_len++;
			w_buf[CMD_IDX] = command;w_len++;
			if (data->block[0] > I2C_SMBUS_BLOCK_MAX - 1) {
				return -EINVAL;
			}
			memcpy(&w_buf[W_DATA_IDX], data->block, data->block[0] + 1);
			w_len += data->block[0];
		}

		break;
	case I2C_SMBUS_BLOCK_PROC_CALL:
		size = I2C_SMBUS_BLOCK_DATA;
		read_write = I2C_SMBUS_READ;

		w_buf[R_BYTE_IDX] = I2C_SMBUS_BLOCK_MAX;w_len++;
		w_buf[CMD_IDX] = command;w_len++;
		if (data->block[0] > I2C_SMBUS_BLOCK_MAX - 1) {
			return -EINVAL;
		}
		memcpy(&w_buf[W_DATA_IDX], data->block, data->block[0] + 1);
		w_len += data->block[0];
		break;
	default:
		log_err(IPMI_I2C, "Unsupported transaction %d\n", size);
		return -EOPNOTSUPP;
	}

	ret = ipmi_i2c_req_wait(user, w_buf, w_len, r_buf, &r_len);
	if (ret < 0) 
		return -EIO;

	if (I2C_SMBUS_WRITE == read_write) {
		ret = 0;
		goto normal;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
	case I2C_SMBUS_BYTE_DATA:
		data->byte = r_buf[0];
		break;
	case I2C_SMBUS_WORD_DATA:
		data->word = le16_to_cpup((__le16 *)r_buf);
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (r_len > I2C_SMBUS_BLOCK_MAX) {
			ret = -EINVAL;
			goto normal;
		}
		memcpy(data->block + 1, r_buf, r_len);
		break;
	case I2C_SMBUS_BLOCK_DATA:
		if (r_len > I2C_SMBUS_BLOCK_MAX) {
			ret = -EPROTO;
			goto normal;
		}
		memcpy(data->block, r_buf, r_len);
		break;
	}
	ret = 0;
	
normal:
	return ret;
}

static int ipmi_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
			  int num)
{
    int ret;
	u8 w_buf[I2C_SMBUS_BLOCK_MAX+CMD_IDX] = {0};
	u8 *r_buf;
	u8 w_len,r_len;
	struct ipmi_user *user = (struct ipmi_user *)adap->algo_data;
	bus_map *map = (bus_map *)i2c_get_adapdata(adap);
	u8 read_length = 0;

	log_dbg(IPMI_I2C, "I2C %d messages\n", num);

	w_len = 0;
	w_buf[BUS_IDX] = map->bmc_bus;w_len++;
	w_buf[ADDR_IDX] = (u8)msgs->addr;w_len++;
	
	if (num == 1) {
		if (msgs->flags & I2C_M_RD) {
		
			log_dbg(IPMI_I2C, "I2C read %#04x len %d\n",
				msgs->addr, msgs->len);
		
			if (msgs->len< 1 || msgs->len > IPMI_I2C_MSG_MAX)
				return -EINVAL;
			w_buf[R_BYTE_IDX] = msgs->len;w_len++;
			
			read_length = msgs->len;
			r_buf = msgs->buf;
		} else {
		
			log_dbg(IPMI_I2C, "I2C write %#04x len %d\n",
				msgs->addr, msgs->len);
		
			if (msgs->len< 1 || msgs->len > I2C_SMBUS_BLOCK_MAX)
				return -EINVAL;
			w_buf[R_BYTE_IDX] = 0;w_len++;
			memcpy(&w_buf[CMD_IDX], msgs->buf, msgs->len);
		   	w_len += msgs->len;
		   	
		   	r_buf = msgs->buf;
		}
	} else if (num == 2 &&
		   msgs[0].addr == msgs[1].addr &&
		   !(msgs[0].flags & I2C_M_RD) && (msgs[1].flags & I2C_M_RD)) {
		
		log_dbg(IPMI_I2C, "I2C write-read %#04x wlen %d rlen %d\n",
			msgs[0].addr, msgs[0].len, msgs[1].len);
		
		if (msgs[0].len< 1 || msgs[0].len > I2C_SMBUS_BLOCK_MAX)
			return -EINVAL;
		if (msgs[1].len< 1 || msgs[1].len > IPMI_I2C_MSG_MAX)
			return -EINVAL;
		w_buf[R_BYTE_IDX] = msgs[1].len;w_len++;
		memcpy(&w_buf[CMD_IDX], msgs[0].buf, msgs[0].len);
	   	w_len += msgs[0].len;

		read_length = msgs[1].len;
		r_buf = msgs[1].buf;
	} else {
		log_err(IPMI_I2C, "Multi-message I2C transactions not supported\n");
		return -EOPNOTSUPP;
	}

   	ret = ipmi_i2c_req_wait(user, w_buf, w_len, r_buf, &r_len);
   	if (ret < 0) 
	   	return -EIO;

	if (r_len < read_length) {
		log_err(IPMI_I2C, "Short read: %d(except: %d)\n", r_len, read_length);
	}

	/* return the number of transferred messages */
	ret = num;
   
	return ret;
}

static u32 ipmi_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C |
		I2C_FUNC_SMBUS_BYTE |
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_BLOCK_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK |
		I2C_FUNC_SMBUS_PROC_CALL |
		I2C_FUNC_SMBUS_BLOCK_PROC_CALL;
}

static struct i2c_algorithm ipmi_i2c_algo = {
    .smbus_xfer = ipmi_xfer,
    .master_xfer = ipmi_i2c_xfer,
    .functionality = ipmi_i2c_func,
};

/************************* 模块初始化和退出 **************************/

static int __init ipmi_i2c_init(void)
{
    int ret,i;
    //u8 data[I2C_SMBUS_BLOCK_MAX],len = 0;

    // 初始化设备结构
    memset(&ipmi_i2c_data, 0, sizeof(ipmi_i2c_data));

    // 初始化IPMI_I2C用户
    ipmi_i2c_data.user_hndl.ipmi_recv_hndl = ipmi_msg_handler;
    ret = ipmi_create_user(0, &ipmi_i2c_data.user_hndl, NULL, &ipmi_i2c_data.user);
    if (ret) {
        log_err(IPMI_I2C, "Failed to create ipmi user: %d\n", ret);
        return ret;
    }

	for (i = 0 ; i < I2C_MAX_BUS; i ++) {
		ipmi_i2c_data.adap[i].owner = THIS_MODULE;
		ipmi_i2c_data.adap[i].class = I2C_CLASS_DEPRECATED;
		ipmi_i2c_data.adap[i].nr = bus_map_list[i].adap_bus;
		sprintf(ipmi_i2c_data.adap[i].name, "bmc_i2c_%d", bus_map_list[i].bmc_bus);
		ipmi_i2c_data.adap[i].retries = 5;
		ipmi_i2c_data.adap[i].algo = &ipmi_i2c_algo;
		i2c_set_adapdata(&ipmi_i2c_data.adap[i], &bus_map_list[i]);
		ipmi_i2c_data.adap[i].algo_data = ipmi_i2c_data.user;
		
		ret = i2c_add_numbered_adapter(&ipmi_i2c_data.adap[i]);
		if (ret) {
			log_err(IPMI_I2C, "Failed to add i2c adapter: %d\n", ret);
			goto err_out;
		}
		
	}

#if 0
    data[0]=1;
    data[1]=0x62;
    data[2]=8;
    data[3]=0x0;
    len=4;
    ipmi_i2c_req_wait(&ipmi_i2c_data, data, len, &len, data);
#endif

	log_info(IPMI_I2C, "IPMI DEV Driver loaded\n");
	return ret;
	
err_out:
	ipmi_destroy_user(ipmi_i2c_data.user);
    return ret;
}

static void __exit ipmi_i2c_exit(void)
{
	int i;
	
	for (i = 0; i < I2C_MAX_BUS; i++ ) {
		i2c_del_adapter(&ipmi_i2c_data.adap[i]);
	}

    ipmi_destroy_user(ipmi_i2c_data.user);
    log_info(IPMI_I2C, "IPMI DEV Driver unloaded\n");
}

module_init(ipmi_i2c_init);
module_exit(ipmi_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhuang.yong");
MODULE_DESCRIPTION("IPMI DEV Driver");
MODULE_VERSION("1.0");

