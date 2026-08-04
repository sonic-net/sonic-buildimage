#include <linux/i2c.h>
#include <linux/module.h>
#include "bms_i2c.h"

#define BMS_PB_I2C_CLIENT_NUM 6

static struct i2c_client *bms_pb_clients[BMS_PB_I2C_CLIENT_NUM] = {NULL};
static int bms_pb_client_index = 0;

static int __init __find_i2c_adap(struct device *dev, void *data)
{
    const char *name = data;
    static const char *prefix = "i2c-";
    struct i2c_adapter *adapter;

    if (strncmp(dev_name(dev), prefix, strlen(prefix)) != 0)
    {
        return 0;
    }
    adapter = to_i2c_adapter(dev);

    return (strncmp(adapter->name, name, strlen(name)) == 0);
}

static int __init find_i2c_adapter_num(enum i2c_adapter_type type)
{
    struct device *dev = NULL;
    struct i2c_adapter *adapter;
    const char *name = bms_i2c_adapter_names[type];

    /* find the adapter by name */
    dev = bus_find_device(&i2c_bus_type, NULL, (void *)name,
                  __find_i2c_adap);
    if (!dev) {
        pr_err("%s: i2c adapter %s not found on system.\n",
               __func__, name);
        return -ENODEV;
    }
    adapter = to_i2c_adapter(dev);

    return adapter->nr;
}


static int __init find_i2c_mux_adapter_num(int parent_num, int num)
{
    struct device *dev = NULL;
    struct i2c_adapter *adapter;
    char name[48];

    snprintf(name, sizeof(name), "i2c-%d-mux (chan_id %d)",
         parent_num, num);
    /* find the adapter by name */
    dev = bus_find_device(&i2c_bus_type, NULL, (void *)name,
                  __find_i2c_adap);
    if (!dev) {
        pr_err("%s: i2c adapter %s not found on system.\n",
               __func__, name);
        return -ENODEV;
    }
    adapter = to_i2c_adapter(dev);

    return adapter->nr;
}

static __init struct i2c_client *bms_pb_setup_eeprom_24c01(
        struct i2c_adapter *adap, int addr)
{
    struct i2c_board_info info_spd = {
        I2C_BOARD_INFO("24c01", addr),
    };

    return i2c_new_device(adap, &info_spd);
}


static int __init bms_pb_setup_devices(void)
{
    struct i2c_adapter *adap;
    int adap_num;
    int parent_num;

    parent_num = find_i2c_adapter_num(I2C_ADAPTER_ISMT);
    if (parent_num < 0)
        return parent_num;

    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN3);
    if (adap_num < 0)
        return adap_num;

    adap = i2c_get_adapter(adap_num);
    if (!adap) {
        pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        goto exit;
    }

    bms_pb_clients[bms_pb_client_index++] = bms_pb_setup_eeprom_24c01(adap, 0x50);
    bms_pb_clients[bms_pb_client_index++] = bms_pb_setup_eeprom_24c01(adap, 0x51);

exit:
    return 0;
}

static int __init bms_pb_i2c_init(void)
{
    /* Initial bms_sb_slients array. */
    memset(bms_pb_clients, 0x0, BMS_PB_I2C_CLIENT_NUM);

    bms_pb_setup_devices();
    return 0;
}


static void __exit bms_pb_i2c_exit(void){
    int i;

    for (i=(bms_pb_client_index-1); i>=0; i--) {
        if (bms_pb_clients[i]) {
            i2c_unregister_device(bms_pb_clients[i]);
            bms_pb_clients[i] = NULL;
        }
    }

    bms_pb_client_index = 0;


}


module_init(bms_pb_i2c_init);
module_exit(bms_pb_i2c_exit);


MODULE_DESCRIPTION("mitac_ly1200_32x_pb_i2c driver");
MODULE_AUTHOR("Eddy Weng <eddy.weng@mic.com.tw>");
MODULE_LICENSE("GPL");

