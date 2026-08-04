#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/platform_data/pca953x.h>
#include <linux/platform_data/pca954x.h>
#include <linux/platform_data/at24.h>
#include <linux/delay.h>
#include "bms_i2c.h"

/* Don't include MAC_AVS_1V */
#define BMS_SB_I2C_CLIENT_NUM 35
#define BMS_SB_ADAPTER_BASE 2
#define BMS_SB_STAGE2_MUX_BUS_BASE 6


static struct i2c_client *bms_sb_clients[BMS_SB_I2C_CLIENT_NUM] = {NULL};
static int bms_sb_client_index = 0;

enum bms_sb_switch_stage2_mux0_bus {
    I2C_STAGE2_MUX0_CHAN0 = 0,
    I2C_STAGE2_MUX0_CHAN1,
    I2C_STAGE2_MUX0_CHAN2,
    I2C_STAGE2_MUX0_CHAN3,
    I2C_STAGE2_MUX0_CHAN4,
    I2C_STAGE2_MUX0_CHAN5,
    I2C_STAGE2_MUX0_CHAN6,
    I2C_STAGE2_MUX0_CHAN7,
};

enum bms_sb_switch_stage2_mux1_bus {
    I2C_STAGE2_MUX1_CHAN8 = 0,
    I2C_STAGE2_MUX1_CHAN9,
    I2C_STAGE2_MUX1_CHAN10,
    I2C_STAGE2_MUX1_CHAN11,
    I2C_STAGE2_MUX1_CHAN12,
    I2C_STAGE2_MUX1_CHAN13,
    I2C_STAGE2_MUX1_CHAN14,
    I2C_STAGE2_MUX1_CHAN15,
};

enum bms_sb_switch_stage2_mux2_bus {
    I2C_STAGE2_MUX2_CHAN16 = 0,
    I2C_STAGE2_MUX2_CHAN17,
    I2C_STAGE2_MUX2_CHAN18,
    I2C_STAGE2_MUX2_CHAN19,
    I2C_STAGE2_MUX2_CHAN20,
    I2C_STAGE2_MUX2_CHAN21,
    I2C_STAGE2_MUX2_CHAN22,
    I2C_STAGE2_MUX2_CHAN23,
};

enum bms_sb_switch_stage2_mux3_bus {
    I2C_STAGE2_MUX3_CHAN24 = 0,
    I2C_STAGE2_MUX3_CHAN25,
    I2C_STAGE2_MUX3_CHAN26,
    I2C_STAGE2_MUX3_CHAN27,
    I2C_STAGE2_MUX3_CHAN28,
    I2C_STAGE2_MUX3_CHAN29,
    I2C_STAGE2_MUX3_CHAN30,
    I2C_STAGE2_MUX3_CHAN31,
};

static struct pca954x_platform_mode pmode_pca9548_mux[] = {
    { .adap_id = BMS_SB_ADAPTER_BASE + 0, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 1, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 2, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 3, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 4, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 5, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 6, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 7, },
};

static struct pca954x_platform_data platdata_pca9548_mux = {
    .modes = pmode_pca9548_mux,
    .num_modes = ARRAY_SIZE(pmode_pca9548_mux),
};

static struct pca954x_platform_mode pmode_stage1_pca9548_mux0[] = {
    { .adap_id = BMS_SB_ADAPTER_BASE + 8, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 9, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 10, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 11, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 12, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 13, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 14, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 15, },
};

static struct pca954x_platform_data platdata_stage1_pca9548_mux0 = {
    .modes = pmode_stage1_pca9548_mux0,
    .num_modes = ARRAY_SIZE(pmode_stage1_pca9548_mux0),
};

static struct pca954x_platform_mode pmode_stage1_pca9548_mux1[] = {
    { .adap_id = BMS_SB_ADAPTER_BASE + 16, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 17, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 18, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 19, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 20, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 21, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 22, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 23, },
};

static struct pca954x_platform_data platdata_stage1_pca9548_mux1 = {
    .modes = pmode_stage1_pca9548_mux1,
    .num_modes = ARRAY_SIZE(pmode_stage1_pca9548_mux1),
};

static struct pca954x_platform_mode pmode_stage1_pca9548_mux2[] = {
    { .adap_id = BMS_SB_ADAPTER_BASE + 24, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 25, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 26, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 27, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 28, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 29, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 30, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 31, },
};

static struct pca954x_platform_data platdata_stage1_pca9548_mux2 = {
    .modes = pmode_stage1_pca9548_mux2,
    .num_modes = ARRAY_SIZE(pmode_stage1_pca9548_mux2),
};

static struct pca954x_platform_mode pmode_stage1_pca9548_mux3[] = {
    { .adap_id = BMS_SB_ADAPTER_BASE + 32, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 33, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 34, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 35, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 36, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 37, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 38, },
    { .adap_id = BMS_SB_ADAPTER_BASE + 39, },
};

static struct pca954x_platform_data platdata_stage1_pca9548_mux3 = {
    .modes = pmode_stage1_pca9548_mux3,
    .num_modes = ARRAY_SIZE(pmode_stage1_pca9548_mux3),
};

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

static __init struct i2c_client *bms_sb_setup_eeprom_24c04(
        struct i2c_adapter *adap)
{
    struct i2c_board_info info_spd = {
        I2C_BOARD_INFO("24c04", 0x50),
    };

    return i2c_new_device(adap, &info_spd);
}

static __init struct i2c_client *bms_sb_setup_tmp75(
        struct i2c_adapter *adap, int addr)
{
    struct i2c_board_info info_spd = {
        I2C_BOARD_INFO("tmp75", addr),
    };

    return i2c_new_device(adap, &info_spd);
}

static __init struct i2c_client *bms_sb_setup_switch(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("pca9548", 0x70),
        .platform_data = &platdata_pca9548_mux,
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_stage1_mux0(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("pca9548", 0x71),
        .platform_data = &platdata_stage1_pca9548_mux0,
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_stage1_mux1(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("pca9548", 0x72),
        .platform_data = &platdata_stage1_pca9548_mux1,
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_stage1_mux2(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("pca9548", 0x73),
        .platform_data = &platdata_stage1_pca9548_mux2,
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_stage1_mux3(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("pca9548", 0x74),
        .platform_data = &platdata_stage1_pca9548_mux3,
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_slave_cpld(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("slave_cpld", 0x33),
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_master_cpld(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("master_cpld", 0x32),
    };

    return i2c_new_device(adap, &info);
}

static __init struct i2c_client *bms_sb_setup_sff8436(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("sff8436", 0x50),
    };

    return i2c_new_device(adap, &info);
}

static int __init bms_sb_setup_devices_ismt(void)
{
    struct i2c_adapter *adap;
    int adap_num = find_i2c_adapter_num(I2C_ADAPTER_ISMT);

    if (adap_num < 0)
        return adap_num;

    adap = i2c_get_adapter(adap_num);
    if (!adap) {
        pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        return 0;
    }

    bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_switch(adap);
    bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_master_cpld(adap);
    bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_slave_cpld(adap);

    return 0;
}

static int __init bms_sb_setup_devices_stage1(void)
{
    struct i2c_adapter *adap;
    int adap_num;
    int parent_num;

    parent_num = find_i2c_adapter_num(I2C_ADAPTER_ISMT);
    if (parent_num < 0)
        return parent_num;

    /* Mux chan0 steup */
    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN0);
    if (adap_num >= 0){
        adap = i2c_get_adapter(adap_num);
        if(adap) {
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_eeprom_24c04(adap);
        }else{
            pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        }
    }else{
        pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN0);
    }

    /* Mux chan1 connect to fan board */

    /* Mux chan2 steup */
    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN2);
    if (adap_num >= 0){
        adap = i2c_get_adapter(adap_num);
        if(adap) {
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_tmp75(adap, 0x4a);
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_tmp75(adap, 0x4b);
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_tmp75(adap, 0x4c);
        }else{
            pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        }
    }else{
        pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN2);
    }

    /* Mux chan3 connect to power board */

    /* Mux chan4 setup for i2c mux0 */
    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN4);
    if (adap_num >= 0){
        adap = i2c_get_adapter(adap_num);
        if(adap) {
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_stage1_mux0(adap);
        }else{
            pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        }
    }else{
        pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN4);
    }

    /* Mux chan5 setup for i2c mux1 */
    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN5);
    if (adap_num >= 0){
        adap = i2c_get_adapter(adap_num);
        if(adap) {
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_stage1_mux1(adap);
        }else{
            pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        }
    }else{
        pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN5);
    }

    /* Mux chan6 setup for i2c mux2 */
    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN6);
    if (adap_num >= 0){
        adap = i2c_get_adapter(adap_num);
        if(adap) {
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_stage1_mux2(adap);
        }else{
            pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        }
    }else{
        pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN6);
    }

    /* Mux chan7 setup for i2c mux3 */
    adap_num = find_i2c_mux_adapter_num(parent_num, I2C_STAGE1_MUX_CHAN7);
    if (adap_num >= 0){
        adap = i2c_get_adapter(adap_num);
        if(adap) {
            bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_stage1_mux3(adap);
        }else{
            pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        }
    }else{
        pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN7);
    }

    return 0;
}

static int __init bms_sb_setup_devices_stage2(void)
{
    struct i2c_adapter *adap;
    int adap_num;
    int i2c_base = sizeof(bms_i2c_adapter_names) / sizeof(bms_i2c_adapter_names[0]);
    int i;

	/* stage2 mux0 chan0~7 install sff8436 */
    for(i = I2C_STAGE2_MUX0_CHAN0; i <= I2C_STAGE2_MUX0_CHAN7; i++){
        adap_num = find_i2c_mux_adapter_num(I2C_STAGE1_MUX_CHAN4 + i2c_base, I2C_STAGE2_MUX0_CHAN0 + i);
        if (adap_num >= 0){
            adap = i2c_get_adapter(adap_num);
            if(adap) {
                bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_sff8436(adap);
            }else{
                pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
            }
        }else{
            pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN4 + i2c_base);
        }
    }

    /* stage2 mux1 chan0~7 install sff8436 */
    for(i = I2C_STAGE2_MUX1_CHAN8; i <= I2C_STAGE2_MUX1_CHAN15; i++){
        adap_num = find_i2c_mux_adapter_num(I2C_STAGE1_MUX_CHAN5 + i2c_base, I2C_STAGE2_MUX1_CHAN8 + i);
        if (adap_num >= 0){
            adap = i2c_get_adapter(adap_num);
            if(adap) {
                bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_sff8436(adap);
            }else{
                pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
            }
        }else{
            pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN5 + i2c_base);
        }
    }

    /* stage2 mux2 chan0~7 install sff8436 */
    for(i = I2C_STAGE2_MUX2_CHAN16; i <= I2C_STAGE2_MUX2_CHAN23; i++){
        adap_num = find_i2c_mux_adapter_num(I2C_STAGE1_MUX_CHAN6 + i2c_base, I2C_STAGE2_MUX2_CHAN16 + i);
        if (adap_num >= 0){
            adap = i2c_get_adapter(adap_num);
            if(adap) {
                bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_sff8436(adap);
            }else{
                pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
            }
        }else{
            pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN6 + i2c_base);
        }
    }

    /* stage2 mux3 chan0~7 install sff8436 */
    for(i = I2C_STAGE2_MUX3_CHAN24; i <= I2C_STAGE2_MUX3_CHAN31; i++){
        adap_num = find_i2c_mux_adapter_num(I2C_STAGE1_MUX_CHAN7 + i2c_base, I2C_STAGE2_MUX3_CHAN24 + i);
        if (adap_num >= 0){
            adap = i2c_get_adapter(adap_num);
            if(adap) {
                bms_sb_clients[bms_sb_client_index++] = bms_sb_setup_sff8436(adap);
            }else{
                pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
            }
        }else{
            pr_err("%s failed to find i2c mux adap number %d.\n", __func__, I2C_STAGE1_MUX_CHAN7 + i2c_base);
        }
    }
}

static int __init bms_sb_i2c_init(void)
{
    /* Initial bms_sb_slients array. */
    memset(bms_sb_clients, 0x0, BMS_SB_I2C_CLIENT_NUM);

    bms_sb_setup_devices_ismt();
    mdelay(200);
    bms_sb_setup_devices_stage1();
    mdelay(200);
    bms_sb_setup_devices_stage2();

    return 0;
}

static void __exit bms_sb_i2c_exit(void){
    int i;

    for (i=(bms_sb_client_index-1); i>=0; i--) {
        if (bms_sb_clients[i]) {
            i2c_unregister_device(bms_sb_clients[i]);
            bms_sb_clients[i] = NULL;
        }
    }

    bms_sb_client_index = 0;

}

module_init(bms_sb_i2c_init);
module_exit(bms_sb_i2c_exit);

MODULE_DESCRIPTION("mitac_ly1200_32x_sb_i2c driver");
MODULE_AUTHOR("Eddy Weng <eddy.weng@mic.com.tw>");
MODULE_LICENSE("GPL");

