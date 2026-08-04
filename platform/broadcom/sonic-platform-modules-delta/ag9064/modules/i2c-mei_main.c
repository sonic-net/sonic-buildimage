/*
 * MEI-I2C driver
 *
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include "mei_dev.h"
#include "client.h"
#include "i2c-mei_rw.h"

#if (defined CONFIG_I2C_MUX_GPIO || defined CONFIG_I2C_MUX_GPIO_MODULE) && \
		defined CONFIG_DMI
#include <linux/gpio.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#endif

/* PCI Address Constants */
#define SMBBAR		0
#define SMBPCICTL	0x004
#define SMBPCISTS	0x006
#define SMBHSTCFG	0x040
#define TCOBASE		0x050
#define TCOCTL		0x054

/* Other settings */
#define MAX_RETRIES		400

/* I801 command constants */
#define I801_QUICK		0x00
#define I801_BYTE		0x04
#define I801_BYTE_DATA		0x08
#define I801_WORD_DATA		0x0C
#define I801_PROC_CALL		0x10	/* unimplemented */
#define I801_BLOCK_DATA		0x14
#define I801_I2C_BLOCK_DATA	0x18	/* ICH5 and later */

#define HECI_PEC_FLAG       0x80

#define PCI_DEVICE_ID_INTEL_LPT_H      0x8C3A  /* Lynx Point H */

struct mei_i2c_mux_config {
	char *gpio_chip;
	unsigned values[3];
	int n_values;
	unsigned classes[3];
	unsigned gpios[2];		/* Relative to gpio_chip->base */
	int n_gpios;
};

#define FEATURE_SMBUS_PEC	(1 << 0)
#define FEATURE_BLOCK_BUFFER	(1 << 1)
#define FEATURE_BLOCK_PROC	(1 << 2)
#define FEATURE_I2C_BLOCK_READ	(1 << 3)
#define FEATURE_IRQ		(1 << 4)
/* Not really a feature, but it's convenient to handle it as such */
#define FEATURE_IDF		(1 << 15)
#define FEATURE_TCO		(1 << 16)

static const char *mei_i2c_feature_names[] = {
	"SMBus PEC",
	"Block buffer",
	"Block process call",
	"I2C block read",
	"Interrupt",
};

static unsigned int disable_features;
module_param(disable_features, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(disable_features, "Disable selected driver features:\n"
	"\t\t  0x01  disable SMBus PEC\n"
	"\t\t  0x02  disable the block buffer\n"
	"\t\t  0x08  disable the I2C block read functionality\n"
	"\t\t  0x10  don't use interrupts ");


/*MEI SMB Sensor Bus (when MEI_FLAG_SMB_DEV_ADD_FMAT_EXT)*/
#define MEI_SMB_BUS_SMBUS   0x0
#define MEI_SMB_BUS_SMLINK0 0x1
#define MEI_SMB_BUS_SMLINK1 0x2
#define MEI_SMB_BUS_SMLINK2 0x3
#define MEI_SMB_BUS_SMLINK3 0x4
#define MEI_SMB_BUS_SMLINK4 0x5

struct mei_smb_priv{
	struct i2c_adapter adapter;
	unsigned long smba;
	unsigned char original_hstcfg;
	struct pci_dev *pci_dev;
	unsigned int features;
    unsigned char sensorbus;
	/* isr processing */
	wait_queue_head_t waitq;
	u8 status;
	/* Command state used by isr for byte-by-byte block transactions */
	u8 cmd;
	bool is_read;
	int count;
	int len;
	u8 *data;
};


struct mei_i2c_data_ext{
	char Cmd;
	char Flag;
	char Sensor_Bus;
	char Psu_Addr;
	char Mux_Addr;
	char Mux_Channel;
	char Mux_Conf;
	char Reserved;
	char W_Length;
	char R_Length;
	char PMbus_data[21];
};

struct mei_msg{
	struct mei_msg_hdr hdr;
	struct mei_i2c_data_ext data;
};

#define DEBUG_MSG 0
static  int mei_TxRx(u8 sensor_bus, u16 addr,  u8 command, char read_write, int size, union i2c_smbus_data * data, int pec)
{
	struct mei_msg_hdr mei_hdr;
	int rets;
	unsigned char  * recv_buf;
	int retry = 0;
	int len = 0;
	int i = 0;

	struct mei_msg * msg;
//	unsigned char  blen;

	UINT32 timeout, dwTimeout;
        UINT32 blen;
	HECI_DEVICE sHeciDev;

	recv_buf = kmalloc(sizeof(unsigned char) * (32), GFP_KERNEL);
	msg = kmalloc(sizeof(struct mei_msg), GFP_KERNEL);

	dwTimeout = 2000000 / HECI_TIMEOUT_UNIT;

	sHeciDev.Bus  = HECI_BUS;
	sHeciDev.Dev  = HECI_DEV;
	sHeciDev.Fun  = HECI_FUN;
	sHeciDev.Hidm = HECI_HIDM_MSI;
	sHeciDev.Mbar = HECI_MBAR_DEFAULT;
	HeciInit(&sHeciDev, &dwTimeout);

	msg->data.Cmd = 0x0A;
	if(read_write){
		if(size == I2C_SMBUS_WORD_DATA){
			msg->data.Flag = 0x56;
			msg->data.W_Length = 1;
			msg->data.R_Length = 2;
		}
		else if(size == I2C_SMBUS_BYTE_DATA || size == I2C_SMBUS_QUICK){
			msg->data.Flag = 0x52;
			msg->data.W_Length = 1;
			msg->data.R_Length = 1;
		}else if(size == I2C_SMBUS_BYTE){
			msg->data.Flag = 0x50;
			msg->data.W_Length = 0;
			msg->data.R_Length = 1;
		}else if(size == I2C_SMBUS_BLOCK_DATA){
			msg->data.Flag = 0x5A;
			msg->data.W_Length = 1;
		}

	}
	else{
		if(size == I2C_SMBUS_WORD_DATA){
			msg->data.Flag = 0x58;
			msg->data.W_Length = 3;
		}
		else if(size == I2C_SMBUS_BYTE_DATA){
			msg->data.Flag = 0x54;
			msg->data.W_Length = 2;
		}
		else if((size == I2C_SMBUS_BYTE) || (size == I2C_SMBUS_QUICK)){
			msg->data.Flag = 0x50;
			msg->data.W_Length = 1;
		}else if(size == I2C_SMBUS_BLOCK_DATA){
			msg->data.Flag = 0x5C;
			msg->data.W_Length = data->block[0];
		}
		
		msg->data.R_Length = 0x0;
		if(data !=NULL){
			if(size == I2C_SMBUS_WORD_DATA){
				msg->data.PMbus_data[1] = data->word & 0xff;
				msg->data.PMbus_data[2] = (data->word & 0xff00) >> 8;
			}else if(size == I2C_SMBUS_BYTE_DATA){
				msg->data.PMbus_data[1] = data->byte;
			}else if(size == I2C_SMBUS_BLOCK_DATA){
				for (i = 0; i < msg->data.W_Length; i++)
					msg->data.PMbus_data[i+1] = data->block[i+1];
			}

		}else{
			msg->data.PMbus_data[1] = 0;
		}
	}

    if (pec == 1)
        msg->data.Flag |= HECI_PEC_FLAG;
    
	msg->data.Sensor_Bus = sensor_bus;
	msg->data.Psu_Addr =(char) addr << 1;
	msg->data.Mux_Addr = 0x0;
	msg->data.Mux_Channel = 0x0;
	msg->data.Mux_Conf = 0x0;
	msg->data.Reserved = 0x0;
	msg->data.PMbus_data[0] = command;
		
	
	msg->hdr.host_addr = 0;//mei_cl_host_addr(cl);
	msg->hdr.me_addr = 0x20;
	msg->hdr.reserved = 0;
	msg->hdr.msg_complete = 0;
	msg->hdr.internal = 0; //cb->internal;
	msg->hdr.length =  10 + msg->data.W_Length;
	msg->hdr.msg_complete = 1;

#if (DEBUG_MSG)
	printk("Cmd : 0x%02x , Flag : 0x%02x , Sensor_Bus : 0x%02x , Psu_Addr : 0x%02x\n" , msg->data.Cmd, msg->data.Flag, msg->data.Sensor_Bus, msg->data.Psu_Addr);
	printk("Mux_Addr : 0x%02x , Mux_Channel : 0x%02x , Mux_Conf : 0x%02x , W_Length : 0x%02x\n" , msg->data.Mux_Addr, msg->data.Mux_Channel, msg->data.Mux_Conf, msg->data.W_Length);
	printk("R_Length : 0x%02x , PMbus_data[0] : 0x%02x , size : 0x%x\n" , msg->data.R_Length, msg->data.PMbus_data[0], size);
	if(!read_write){
		if(size == I2C_SMBUS_BLOCK_DATA){
			for (i = 0; i < msg->data.W_Length; i++){
				printk("PMbus_data[%d] : 0x%02x , ", i, msg->data.PMbus_data[i]);
			}
			printk("\n");
		}else{
			printk("PMbus_data[1] : 0x%02x , PMbus_data[2] : 0x%02x\n", msg->data.PMbus_data[1], msg->data.PMbus_data[2]);
		}
	}
#endif
	retry = 3;
	while(retry){
		timeout = HECI_SEND_TIMEOUT / HECI_TIMEOUT_UNIT;
		rets = HeciMsgSend(&sHeciDev, &timeout, (HECI_MSG_HEADER *)msg);
		if (rets != 0){
			printk("HeciMsgSend ret: %d\n",rets);
			retry --;
			continue;
		}else{
			break;
		}
	}
	if(read_write)
	{
		if(size == I2C_SMBUS_WORD_DATA){
            blen = 8;
			HeciMsgRecv(&sHeciDev, &timeout, (HECI_MSG_HEADER *)recv_buf, &blen);
        }
		else if(size == I2C_SMBUS_BYTE_DATA || size == I2C_SMBUS_QUICK || size == I2C_SMBUS_BYTE){
            blen = 7;
			HeciMsgRecv(&sHeciDev, &timeout, (HECI_MSG_HEADER *)recv_buf, &blen);
        }
#if (DEBUG_MSG)
		if(size == I2C_SMBUS_BLOCK_DATA){
			printk("recv_len %d hdr: 0x%02x%02x%02x%02x\n", blen, recv_buf[3], recv_buf[2], recv_buf[1], recv_buf[0]);
			for (i = 0; i < blen ; i++){
				printk("0x%02x , ", recv_buf[4 + i]);
			}
			printk("\n");
		}else{
			printk("recv_len %d recv: 0x%02x%02x%02x%02x\n0x%02x , 0x%02x , 0x%02x,  0x%02x \n", blen, recv_buf[3], recv_buf[2], recv_buf[1], recv_buf[0], recv_buf[4], recv_buf[5], recv_buf[6], recv_buf[7]);
		}
#endif		
		if(data !=NULL){
			if(size == I2C_SMBUS_WORD_DATA){
				data->word =  ((recv_buf[7]  << 8) & 0xff00) | (recv_buf[6] & 0xff);
			}
			else if(size == I2C_SMBUS_BYTE_DATA){
				data->byte =  recv_buf[6] & 0xff;
			}
			else if(size == I2C_SMBUS_BLOCK_DATA){
				for (i = 0; i < blen; i++){
					data->block[i] =  recv_buf[6+i] & 0xff;
				}
			}
		}
	}
	else
	{
		blen = 6;
		HeciMsgRecv(&sHeciDev, &timeout, (HECI_MSG_HEADER *)recv_buf, &blen);
#if (DEBUG_MSG)
		printk("recv: 0x%02x%02x%02x%02x , 0x%02x , 0x%02x \n", recv_buf[3], recv_buf[2], recv_buf[1], recv_buf[0], recv_buf[4], recv_buf[5]);
#endif
	}

	rets = recv_buf[5];
	
	kfree(recv_buf);
	kfree(msg);
	if(rets)
		return -1;
	else
		return 0;
}

/* Return negative errno on error. */
static s32 mei_i2c_access(struct i2c_adapter *adap, u16 addr,
		       unsigned short flags, char read_write, u8 command,
		       int size, union i2c_smbus_data *data)
{
	int hwpec;
	int block = 0;
	int ret = 0, xact = 0;
    int pec = 0;
	char byte = 0;
	struct mei_smb_priv *priv = i2c_get_adapdata(adap);

    if (flags & I2C_CLIENT_PEC)
        pec = 1;
    
	switch (size) {
	case I2C_SMBUS_QUICK:

		command = 0;
		read_write = 1;
		ret = mei_TxRx(priv->sensorbus, addr, command, read_write, size, NULL, pec);
		xact = I801_QUICK;
		break;
	case I2C_SMBUS_BYTE:
		if (read_write == I2C_SMBUS_READ)
			command = 0;
		ret = mei_TxRx(priv->sensorbus, addr, command, read_write, size, data, pec);
		xact = I801_BYTE;
		break;
	case I2C_SMBUS_BYTE_DATA:
		ret = mei_TxRx(priv->sensorbus, addr, command, read_write, size, data, pec);
		xact = I801_BYTE_DATA;
		break;
	case I2C_SMBUS_WORD_DATA:
		ret = mei_TxRx(priv->sensorbus, addr, command, read_write, size, data, pec);
		xact = I801_WORD_DATA;
		break;
	case I2C_SMBUS_BLOCK_DATA:
		ret = mei_TxRx(priv->sensorbus, addr, command, read_write, size, data, pec);
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		printk("I2C_SMBUS_I2C_BLOCK_DATA unsupported!!%d\n",size);
		break;
	default:
		dev_err(&priv->pci_dev->dev, "Unsupported transaction %d\n",
			size);
		return -EOPNOTSUPP;
	}

	if (ret)
		return ret;
	return 0;
}

static u32 mei_i2c_func(struct i2c_adapter *adapter)
{
	struct mei_smb_priv *priv = i2c_get_adapdata(adapter);

	return I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE |
	       I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA |
	       I2C_FUNC_SMBUS_BLOCK_DATA ;
}

static const struct i2c_algorithm smbus_algorithm = {
	.smbus_xfer	= mei_i2c_access,
	.functionality	= mei_i2c_func,
};

static const struct pci_device_id mei_i2c_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_LPT_H) },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, mei_i2c_ids);

/* richard + priv_table */
struct mei_smb_priv_table {
        struct mei_smb_priv *priv_tbl[MEI_SMB_BUS_SMLINK4];
        int    count;
};

static int mei_i2c_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	unsigned char temp;
	int err, i;
	struct mei_smb_priv *priv_sml_0, *priv_sml_1, *priv_sml_2, *priv_sml_3, *priv_sml_4, *priv_smb;

//richard + priv_table
	struct mei_smb_priv_table *priv_table;

	priv_table = kzalloc(sizeof(*priv_table), GFP_KERNEL);
	if(!priv_table)
	        return -ENOMEM;

	priv_sml_0= kzalloc(sizeof(struct mei_smb_priv), GFP_KERNEL);
	if (!priv_sml_0)
		return -ENOMEM;
	i2c_set_adapdata(&priv_sml_0->adapter, priv_sml_0);
	priv_sml_0->adapter.owner = THIS_MODULE;
	priv_sml_0->adapter.algo = &smbus_algorithm;
	priv_sml_0->adapter.dev.parent = &pdev->dev;
	priv_sml_0->adapter.retries = 3;
	priv_sml_0->sensorbus = MEI_SMB_BUS_SMLINK0;
	priv_sml_0->pci_dev = pdev;

    	priv_sml_1 = kzalloc(sizeof(*priv_sml_1), GFP_KERNEL);
   	 if (!priv_sml_1)
		return -ENOMEM;
    	i2c_set_adapdata(&priv_sml_1->adapter, priv_sml_1);
	priv_sml_1->adapter.owner = THIS_MODULE;
	priv_sml_1->adapter.algo = &smbus_algorithm;
	priv_sml_1->adapter.dev.parent = &pdev->dev;
	priv_sml_1->adapter.retries = 3;
    	priv_sml_1->sensorbus = MEI_SMB_BUS_SMLINK1;
	priv_sml_1->pci_dev = pdev;

	priv_sml_2 = kzalloc(sizeof(*priv_sml_2), GFP_KERNEL);
	if (!priv_sml_2)
		return -ENOMEM;
    	i2c_set_adapdata(&priv_sml_2->adapter, priv_sml_2);
	priv_sml_2->adapter.owner = THIS_MODULE;
	priv_sml_2->adapter.algo = &smbus_algorithm;
	priv_sml_2->adapter.dev.parent = &pdev->dev;
	priv_sml_2->adapter.retries = 3;
    	priv_sml_2->sensorbus = MEI_SMB_BUS_SMLINK2;
	priv_sml_2->pci_dev = pdev;

	priv_sml_3 = kzalloc(sizeof(*priv_sml_3), GFP_KERNEL);
    	if (!priv_sml_3)
		return -ENOMEM;
	i2c_set_adapdata(&priv_sml_3->adapter, priv_sml_3);
	priv_sml_3->adapter.owner = THIS_MODULE;
	priv_sml_3->adapter.algo = &smbus_algorithm;
	priv_sml_3->adapter.dev.parent = &pdev->dev;
	priv_sml_3->adapter.retries = 3;
    	priv_sml_3->sensorbus = MEI_SMB_BUS_SMLINK3;
	priv_sml_3->pci_dev = pdev;

	priv_sml_4 = kzalloc(sizeof(*priv_sml_4), GFP_KERNEL);
   	 if (!priv_sml_4)
		return -ENOMEM;
	i2c_set_adapdata(&priv_sml_4->adapter, priv_sml_4);
	priv_sml_4->adapter.owner = THIS_MODULE;
	priv_sml_4->adapter.algo = &smbus_algorithm;
	priv_sml_4->adapter.dev.parent = &pdev->dev;
	priv_sml_4->adapter.retries = 3;
	priv_sml_4->sensorbus = MEI_SMB_BUS_SMLINK4;
	priv_sml_4->pci_dev = pdev;

	printk("mei_i2c_probe 0x%x 0x%x\n", pdev->device, pdev->dev.id);

	snprintf(priv_sml_0->adapter.name, sizeof(priv_sml_0->adapter.name),
		"ME-SMLINK0");
	err = i2c_add_adapter(&priv_sml_0->adapter);
	printk("i2c nr : %d \n", priv_sml_0->adapter.nr);
	if (err) {
		dev_err(&pdev->dev, "Failed to add SMBus adapter ME-SMLINK0\n");
		return err;
	}	

	snprintf(priv_sml_1->adapter.name, sizeof(priv_sml_1->adapter.name),
		"ME-SMLINK1");
	err = i2c_add_adapter(&priv_sml_1->adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to add SMBus adapter ME-SMLINK1\n");
		return err;
	}

	snprintf(priv_sml_2->adapter.name, sizeof(priv_sml_2->adapter.name),
		"ME-SMLINK2");
	err = i2c_add_adapter(&priv_sml_2->adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to add SMBus adapter ME-SMLINK2\n");
		return err;
	}	
    
	snprintf(priv_sml_3->adapter.name, sizeof(priv_sml_3->adapter.name),
		"ME-SMLINK3");
	err = i2c_add_adapter(&priv_sml_3->adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to add SMBus adapter ME-SMLINK3\n");
		return err;
	}

	snprintf(priv_sml_4->adapter.name, sizeof(priv_sml_4->adapter.name),
		"ME-SMLINK4");
	err = i2c_add_adapter(&priv_sml_4->adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to add SMBus adapter ME-SMLINK4\n");
		return err;
	}	
    
	priv_table->count = 0;
	priv_table->priv_tbl[priv_table->count++] = priv_sml_0;
	priv_table->priv_tbl[priv_table->count++] = priv_sml_1;
	priv_table->priv_tbl[priv_table->count++] = priv_sml_2;
	priv_table->priv_tbl[priv_table->count++] = priv_sml_3;
	priv_table->priv_tbl[priv_table->count++] = priv_sml_4;

	pci_set_drvdata(pdev, priv_table);

	return 0;
}

static void mei_i2c_remove(struct pci_dev *dev)
{
	struct mei_smb_priv *priv = pci_get_drvdata(dev);
	// richard + priv_table
	struct mei_smb_priv_table *priv_table = pci_get_drvdata(dev);
	int    i;

	for(i=0; i<priv_table->count; i++) {
		i2c_del_adapter(&priv_table->priv_tbl[i]->adapter);
	}
	pci_write_config_byte(dev, SMBHSTCFG, priv->original_hstcfg);

	/*
	 * do not call pci_disable_device(dev) since it can cause hard hangs on
	 * some systems during power-off (eg. Fujitsu-Siemens Lifebook E8010)
	 */
}

#define mei_i2c_suspend NULL
#define mei_i2c_resume NULL

static struct pci_driver mei_i2c_driver = {
	.name		= "mei_i2c",
	.id_table	= mei_i2c_ids,
	.probe		= mei_i2c_probe,
	.remove	= mei_i2c_remove,
	.suspend	= mei_i2c_suspend,
	.resume	= mei_i2c_resume,
};

static int __init mei_i2c_init(void)
{
	int ret = 16;
	u32 status  = 0;
	struct pci_dev *pdev = NULL;
	struct mei_device *dev;
	struct pci_driver *pci_drv;

	return pci_register_driver(&mei_i2c_driver);
}

static void __exit mei_i2c_exit(void)
{
	pci_unregister_driver(&mei_i2c_driver);
}

MODULE_AUTHOR("Delta Networks, Inc.");
MODULE_DESCRIPTION("MEI SMBus driver");
MODULE_LICENSE("GPL");

module_init(mei_i2c_init);
module_exit(mei_i2c_exit);
