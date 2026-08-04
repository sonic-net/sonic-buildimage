#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define CPUPLD_I2C_ADDR   0x31

enum cpld_type {
    cpld,
};

struct platform_data {
    int reg_addr;
    struct i2c_client *client;
};

enum{
    BUS0 = 0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,
    BUS5,
    BUS6,
    BUS7,
    BUS8,
};

enum{
    CPU_PCB_NUM = 0,
    CPUPLD_VER_TYPE,
    CPUPLD_VER,
    BDXDE_PLAT_RST,
    BDXDE_SLP3_STAT,
    BDXDE_SLP4_STAT,
    BDXDE_CPU_RST,
    CPLD_DEBUG_MODE,
    APWROK_STAT,
    EDGE_PROCHOT_SIG_DIS,
    PSU_THERMAL_STAT,    
    PR_THERMAL_STAT,
    ME_DRIVE_SIG_EN,
    CPU_THERMAL_STAT,
    DDR_THERMAL_STAT,
    SYS_THERMAL_STAT,
    DEBUG_LED3_EN,
    DEBUG_LED2_EN,
    DEBUG_LED1_EN,
    DEBUG_LED0_EN,
    CPU_STANDBY_MODE,
    CPLD_RST,
    MB_POWER_STAT,
    BIOS1_SPI_WP,    
    BIOS2_SPI_WP,
    BIOS_MUX_SEL,
    GBE_SPI_WP,
    PCH_THERMTRIP_EN,
    ID_EEPROM_EN,
    CPU_I2C_MUX_EN,
    CPU_I2C_MUX_SEL,
    PSU_FAN_INTR,
    WD_TIMER,
    WD_EN,
    WD_CLEAR_FLAG,
};


static unsigned char cpupld_reg_addr;

static ssize_t cpupld_reg_value_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    struct platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[cpld].client, cpupld_reg_addr);

    return scnprintf(buf, PAGE_SIZE, "0x%02x\n", ret);
}

static ssize_t cpupld_reg_value_store(struct device *dev, struct device_attribute *attr,
                                      const char *buf, size_t count)
{
    unsigned long data;
    int err;
    struct platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    i2c_smbus_write_byte_data(pdata[cpld].client, cpupld_reg_addr, data);

    return count;
}

static ssize_t cpupld_reg_addr_show(struct device *dev, struct device_attribute *devattr, char *buf)
{

    return scnprintf(buf, PAGE_SIZE, "0x%02x\n", cpupld_reg_addr);
}

static ssize_t cpupld_reg_addr_store(struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t count)
{
    unsigned long data;
    int err;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }
    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }
    cpupld_reg_addr = data;

    return count;
}


static ssize_t cpupld_data_show(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct platform_data *pdata = dev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    bool hex_fmt = 0;
    char desc[256] = {0};

    select = attr->index;
    switch(select) {
        case CPU_PCB_NUM:
            offset = 0x0;
            hex_fmt = 1;
            scnprintf(desc, PAGE_SIZE, "\nCPU Borad PCB Number.\n");
            break;
        case CPUPLD_VER_TYPE:
            offset = 0x1;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nCPUPLD Version Type.\n");
            break;
        case CPUPLD_VER:
            offset = 0x1;
            mask = 0x7F;
            scnprintf(desc, PAGE_SIZE, "\nCPUPLD Version.\n");
            break;
        case BDXDE_PLAT_RST:
            offset = 0x9;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Platform Reset State\n“0” = Platform Not Reset State.\n");
            break;
        case BDXDE_SLP3_STAT:
            offset = 0x9;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = CPU at S3 State\n“0” = CPU Not at S3 State.\n");
            break;
        case BDXDE_SLP4_STAT:
            offset = 0x9;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = CPU at S4Sstate\n“0” = CPU Not at S4 State.\n");
            break;
        case BDXDE_CPU_RST:
            offset = 0x9;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = CPU Not Reset State\n“0” = CPU Reset State.\n");
            break;
        case CPLD_DEBUG_MODE:
            offset = 0x9;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nCPLD Power Sequence\n“1” = Debug Mode\n“0” = Normal Mode.\n");
            break;
        case APWROK_STAT:
            offset = 0xA;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = APWROK Stable\n“0” = APWROK Unstable.\n");
            break;
        case EDGE_PROCHOT_SIG_DIS:
            offset = 0xB;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable Power Supply Thermal Signal\n“0” = Enable Power Supply Thermal Signal.\n");
            break;
        case PSU_THERMAL_STAT:
            offset = 0xB;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Power Supply Normal Temperature\n“0” = Power Supply Over Temperature.\n");
            break;
        case PR_THERMAL_STAT:
            offset = 0xB;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Power Rail Normal Temperature\n“0” = Power Rail Over Temperature.\n");
            break;
        case ME_DRIVE_SIG_EN:
            offset = 0xB;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable System Thermal Alarm to CPU\n“0” = System Thermal Alarm to CPU.\n");
            break;
        case CPU_THERMAL_STAT:
            offset = 0xB;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = CPU Disomic Normal Temperature\n“0” = CPU Disomic Over Temperatur.\n");
            break;
        case DDR_THERMAL_STAT:
            offset = 0xB;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = DDR Normal Temperature\n“0” = DDR Over Temperature.\n");
            break;
        case SYS_THERMAL_STAT:
            offset = 0xC;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = System Normal Temperature.\n“0” = System Over Temperature.\n");
            break;
        case DEBUG_LED3_EN:
            offset = 0xD;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable Debug LED3\n“0” = Enable Debug LED3.\n");
            break;
        case DEBUG_LED2_EN:
            offset = 0xD;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable Debug LED2\n“0” = Enable Debug LED2.\n");
            break;
       case DEBUG_LED1_EN:
            offset = 0xD;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable Debug LED1\n“0” = Enable Debug LED1.\n");
            break;
       case DEBUG_LED0_EN:
            offset = 0xD;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable Debug LED0\n“0” = Enable Debug LED0.\n");
            break;
       case CPU_STANDBY_MODE:
            offset = 0x11;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = CPU Power Stanby Not Ready\n“0” = CPU Power Stanby Ready.\n");
            break;
       case CPLD_RST:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation\n“0” = CPLD Reset.\n");
            break;
       case MB_POWER_STAT:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Power Rail Good\n“0” = Power Rail Failed.\n");
            break;
       case BIOS2_SPI_WP:
            offset = 0x13;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable BIOS2 SPI Write Protect\n“0” = Enable BIOS2 SPI Write Protect.\n");
            break;
       case BIOS1_SPI_WP:
            offset = 0x13;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable BIOS1 SPI Write Protect\n“0” = Enable BIOS1 SPI Write Protect.\n");
            break;
       case BIOS_MUX_SEL:
            offset = 0x13;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Primary BIOS\n“0” = Backup BIOS.\n");
            break;
       case GBE_SPI_WP:
            offset = 0x13;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable GBE SPI Write Protect\n“0” = Enable GBE SPI Write Protect.\n");
            break;
       case PCH_THERMTRIP_EN:
            offset = 0x14;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Thermal Trip Not Occured\n“0” = Thermal Trip Occured.\n");
            break;
       case ID_EEPROM_EN:
            offset = 0x14;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable ID EEPROM Write Protect\n“0” = Enable ID EEPROM Write Protect.\n");
            break;
       case CPU_I2C_MUX_EN:
            offset = 0x14;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable CPU I2C Mux\n“0” = Disable CPU I2C Mux.\n");
            break;
       case CPU_I2C_MUX_SEL:
            offset = 0x14;
            shift = 0;
            mask = 0x3;
            scnprintf(desc, PAGE_SIZE, "\n“3” = Select MB Panel Port.\n“2” = Select MB SWPLD.\n“1” = Select MB Mux.\n“0” = Select ONIE EEPROM.\n");
            break;
       case PSU_FAN_INTR:
            offset = 0x15;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU Fan Interrupt Occured\n“0” = PSU Fan Interrupt Not Occured.\n");
            break;
       case WD_TIMER:
            offset = 0x1E;
            shift = 3;
            mask = 0x38;
            scnprintf(desc, PAGE_SIZE, "\n“5” = Timer 60 sec.\n“4” = Timer 50 sec.\n“3” = Timer 40 sec.\n“2” = Timer 30 sec.\n“1” = Timer 20 sec.\n“0” = Timer 15 sec.\n");
            break;
       case WD_EN:
            offset = 0x1E;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable Watchdog Function\n“0” = Enable Watchdog Function.\n");
            break;
       case WD_CLEAR_FLAG:
            offset = 0x1E;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Watchdog Timer Flag Clear\n“0” = Watchdog Timer Flag Not Clear.\n");
            break;
    }    

    value = i2c_smbus_read_byte_data(pdata[cpld].client, offset);
    value = (value & mask) >> shift;
    if(hex_fmt) {
        return scnprintf(buf, PAGE_SIZE, "0x%02x%s", value, desc);
    } else {
        return scnprintf(buf, PAGE_SIZE, "%d%s", value, desc);
    }
}

static ssize_t cpupld_data_store(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct platform_data *pdata = dev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    int err = 0;
    unsigned long data;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    switch (attr->index) {
        case DEBUG_LED3_EN:
            offset = 0xD;
            shift = 3;
            mask = (1 << shift);
            break;
        case DEBUG_LED2_EN:
            offset = 0xD;
            shift = 2;
            mask = (1 << shift);
            break;
        case DEBUG_LED1_EN:
            offset = 0xD;
            shift = 1;
            mask = (1 << shift);
            break;
        case DEBUG_LED0_EN:
            offset = 0xD;
            shift = 0;
            mask = (1 << shift);
            break;
        case CPLD_RST:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            break;
        case MB_POWER_STAT:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            break;
        case BIOS2_SPI_WP:
            offset = 0x13;
            shift = 3;
            mask = (1 << shift);
            break;
        case BIOS1_SPI_WP:
            offset = 0x13;
            shift = 2;
            mask = (1 << shift);
            break;
        case BIOS_MUX_SEL:
            offset = 0x13;
            shift = 1;
            mask = (1 << shift);
            break;
        case GBE_SPI_WP:
            offset = 0x13;
            shift = 0;
            mask = (1 << shift);
            break;
        case PCH_THERMTRIP_EN:
            offset = 0x14;
            shift = 4;
            mask = (1 << shift);
            break;
        case ID_EEPROM_EN:
            offset = 0x14;
            shift = 3;
            mask = (1 << shift);
            break;
        case CPU_I2C_MUX_EN:
            offset = 0x14;
            shift = 2;
            mask = (1 << shift);
            break;
        case CPU_I2C_MUX_SEL:
            offset = 0x14;
            shift = 0;
            mask = 0x3;
            break;
        case WD_TIMER:
            offset = 0x1E;
            shift = 3;
            mask = 0x38;
            break;
        case WD_EN:
            offset = 0x1E;
            shift = 2;
            mask = (1 << shift);
            break;
        case WD_CLEAR_FLAG:
            offset = 0x1E;
            shift = 0;
            mask = (1 << shift);
            break;
    }

    value = i2c_smbus_read_byte_data(pdata[cpld].client, offset);
    data = (value & ~mask) | (data << shift);
    i2c_smbus_write_byte_data(pdata[cpld].client, offset, data);

    return count;
}

static DEVICE_ATTR(cpupld_reg_value, S_IRUGO | S_IWUSR, cpupld_reg_value_show, cpupld_reg_value_store);
static DEVICE_ATTR(cpupld_reg_addr,  S_IRUGO | S_IWUSR, cpupld_reg_addr_show,  cpupld_reg_addr_store);

/* offset 0x0 */
static SENSOR_DEVICE_ATTR(cpu_pcb_num,           S_IRUGO,         cpupld_data_show,        NULL,            CPU_PCB_NUM            );
/* offset 0x1 */
static SENSOR_DEVICE_ATTR(cpupld_ver_type,       S_IRUGO,         cpupld_data_show,        NULL,            CPUPLD_VER_TYPE        );
static SENSOR_DEVICE_ATTR(cpupld_ver,            S_IRUGO,         cpupld_data_show,        NULL,            CPUPLD_VER             );
#if 0
/* offset 0x5 */
static SENSOR_DEVICE_ATTR(p1v2_vddq_en,          S_IRUGO,         cpupld_data_show,        NULL,            P1V2_VDDQ_EN           );
static SENSOR_DEVICE_ATTR(p1v5_pch_en,           S_IRUGO,         cpupld_data_show,        NULL,            P1V5_PCH_EN            );
static SENSOR_DEVICE_ATTR(p2v5_vpp_en,           S_IRUGO,         cpupld_data_show,        NULL,            P2V5_VPP_EN            );
static SENSOR_DEVICE_ATTR(pvccin_en,             S_IRUGO,         cpupld_data_show,        NULL,            PVCCIN_EN              );
static SENSOR_DEVICE_ATTR(pvccioin_en,           S_IRUGO,         cpupld_data_show,        NULL,            PVCCIOIN_EN            );
static SENSOR_DEVICE_ATTR(pvcckrhv_en,           S_IRUGO,         cpupld_data_show,        NULL,            PVCCKRHV_EN            );   
static SENSOR_DEVICE_ATTR(pvccscfusesus_en,      S_IRUGO,         cpupld_data_show,        NULL,            PVCCSCFUSESUS_EN       );
static SENSOR_DEVICE_ATTR(vr_p3v3_en,            S_IRUGO,         cpupld_data_show,        NULL,            VR_P3V3_EN             );
/* offset 0x6 */
static SENSOR_DEVICE_ATTR(cpu_sys_power,         S_IRUGO,         cpupld_data_show,        NULL,            CPU_SYS_POWER          );
static SENSOR_DEVICE_ATTR(p0v6_vtt_dimm_en,      S_IRUGO,         cpupld_data_show,        NULL,            P0V6_VTT_DIMM_EN       );
static SENSOR_DEVICE_ATTR(p1v05_pch_en,          S_IRUGO,         cpupld_data_show,        NULL,            P1V05_PCH_EN           );
/* offser 0x7 */
static SENSOR_DEVICE_ATTR(p1v5_pch_pwrgd,        S_IRUGO,         cpupld_data_show,        NULL,            P1V5_PCH_GOOD          );
static SENSOR_DEVICE_ATTR(p2v5_vpp_pwrgd,        S_IRUGO,         cpupld_data_show,        NULL,            P2V5_VPP_GOOD          );
static SENSOR_DEVICE_ATTR(pch_pwr_pwrgd,         S_IRUGO,         cpupld_data_show,        NULL,            PCH_PWR_GOOD           );
static SENSOR_DEVICE_ATTR(pvccin_pwrgd,          S_IRUGO,         cpupld_data_show,        NULL,            PVCCIN_GOOD            );
static SENSOR_DEVICE_ATTR(pvccioin_pwrgd,        S_IRUGO,         cpupld_data_show,        NULL,            PVCCIOIN_GOOD          );
static SENSOR_DEVICE_ATTR(pvcckrhv_pwrgd,        S_IRUGO,         cpupld_data_show,        NULL,            PVCCKRHV_GOOD          );
static SENSOR_DEVICE_ATTR(pvccscfusesus_pwrgd,   S_IRUGO,         cpupld_data_show,        NULL,            PVCCSCFUSESUS_GOOD     );
static SENSOR_DEVICE_ATTR(vr_p3v3_pwrgd,         S_IRUGO,         cpupld_data_show,        NULL,            VR_P3V3_GOOD           );
/*  offset 0x8 */
static SENSOR_DEVICE_ATTR(bdxde_lan_pwrgd,       S_IRUGO,         cpupld_data_show,        NULL,            BDXDE_LAN_GOOD         );
static SENSOR_DEVICE_ATTR(CPU0_pwrgd,            S_IRUGO,         cpupld_data_show,        NULL,            CPU0_GOOD              );
static SENSOR_DEVICE_ATTR(p0v6_vtt_dimm_pwrgd,   S_IRUGO,         cpupld_data_show,        NULL,            P0V6_VTT_DIMM_GOOD     );
static SENSOR_DEVICE_ATTR(p1v05_procio_pwrgd,    S_IRUGO,         cpupld_data_show,        NULL,            P1V05_PROCIO_GOOD      );
static SENSOR_DEVICE_ATTR(p1v2_vddq_pwrgd,       S_IRUGO,         cpupld_data_show,        NULL,            P1V2_VDDQ_GOOD         );
#endif
/* offset 0x9 */
static SENSOR_DEVICE_ATTR(bdxde_plat_rst,        S_IRUGO,         cpupld_data_show,        NULL,            BDXDE_PLAT_RST         );
static SENSOR_DEVICE_ATTR(bdxde_slp3_stat,       S_IRUGO,         cpupld_data_show,        NULL,            BDXDE_SLP3_STAT        );
static SENSOR_DEVICE_ATTR(bdxde_slp4_stat,       S_IRUGO,         cpupld_data_show,        NULL,            BDXDE_SLP4_STAT        );
static SENSOR_DEVICE_ATTR(bdxde_cpu_rst,         S_IRUGO,         cpupld_data_show,        NULL,            BDXDE_CPU_RST          );
static SENSOR_DEVICE_ATTR(cpld_debug_mode,       S_IRUGO,         cpupld_data_show,        NULL,            CPLD_DEBUG_MODE        );
/* offset 0xA */
static SENSOR_DEVICE_ATTR(apwrok_stat,           S_IRUGO,         cpupld_data_show,        NULL,            APWROK_STAT            );
//static SENSOR_DEVICE_ATTR(cpu_standby_mode,      S_IRUGO,         cpupld_data_show,        NULL,            CPU_STANDBY_MODE       );
/* offset 0xB */
static SENSOR_DEVICE_ATTR(edge_prochot_sig_dis,  S_IRUGO,         cpupld_data_show,        NULL,            EDGE_PROCHOT_SIG_DIS    );
static SENSOR_DEVICE_ATTR(psu_thermal_stat,      S_IRUGO,         cpupld_data_show,        NULL,            PSU_THERMAL_STAT        );
static SENSOR_DEVICE_ATTR(pr_thermal_stat,       S_IRUGO,         cpupld_data_show,        NULL,            PR_THERMAL_STAT         );
static SENSOR_DEVICE_ATTR(me_drv_sig_en,         S_IRUGO,         cpupld_data_show,        NULL,            ME_DRIVE_SIG_EN         );
static SENSOR_DEVICE_ATTR(cpu_thermal_stat,      S_IRUGO,         cpupld_data_show,        NULL,            CPU_THERMAL_STAT        );
static SENSOR_DEVICE_ATTR(ddr_thermal_stat,      S_IRUGO,         cpupld_data_show,        NULL,            DDR_THERMAL_STAT        );
/* offset 0xC */
static SENSOR_DEVICE_ATTR(sys_thermal_stat,      S_IRUGO,         cpupld_data_show,        NULL,            SYS_THERMAL_STAT        );
/* offset 0xD */
static SENSOR_DEVICE_ATTR(debug_led3_en,         S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     DEBUG_LED3_EN           );
static SENSOR_DEVICE_ATTR(debug_led2_en,         S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     DEBUG_LED2_EN           );
static SENSOR_DEVICE_ATTR(debug_led1_en,         S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     DEBUG_LED1_EN           );
static SENSOR_DEVICE_ATTR(debug_led0_en,         S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     DEBUG_LED0_EN           );
/* offset 0x11 */
static SENSOR_DEVICE_ATTR(cpu_standby_mode,      S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     CPU_STANDBY_MODE        );
static SENSOR_DEVICE_ATTR(cpld_rst,              S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     CPLD_RST                );
/* offset 0x12 */
static SENSOR_DEVICE_ATTR(mb_power_stat,         S_IRUGO,         cpupld_data_show,        NULL,            MB_POWER_STAT           );
/* offset 0x13 */
static SENSOR_DEVICE_ATTR(bios1_spi_wp,          S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     BIOS1_SPI_WP            );
static SENSOR_DEVICE_ATTR(bios2_spi_wp,          S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     BIOS2_SPI_WP            );
static SENSOR_DEVICE_ATTR(bios_mux_sel,          S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     BIOS_MUX_SEL            );
static SENSOR_DEVICE_ATTR(gbe_spi_wp,            S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     GBE_SPI_WP              );
/* offset 0x14 */
static SENSOR_DEVICE_ATTR(pch_thermtrip_en,      S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     PCH_THERMTRIP_EN        );
static SENSOR_DEVICE_ATTR(id_eeprom_wp,          S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     ID_EEPROM_EN            );
static SENSOR_DEVICE_ATTR(cpu_i2c_mux_en,        S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     CPU_I2C_MUX_EN          );
static SENSOR_DEVICE_ATTR(cpu_i2c_mux_sel,       S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     CPU_I2C_MUX_SEL         );
/* offset 0x15 */
static SENSOR_DEVICE_ATTR(psu_fan_intr,          S_IRUGO,         cpupld_data_show,        NULL,            PSU_FAN_INTR            );
/* offset 0x1E */
static SENSOR_DEVICE_ATTR(wd_timer,              S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     WD_TIMER                );
static SENSOR_DEVICE_ATTR(wd_en,                 S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     WD_EN                   );
static SENSOR_DEVICE_ATTR(wd_clear_flag,         S_IRUGO|S_IWUSR, cpupld_data_show,  cpupld_data_store,     WD_CLEAR_FLAG           );


static struct attribute *agc032_cpupld_attrs[] = {
    &dev_attr_cpupld_reg_value.attr,
    &dev_attr_cpupld_reg_addr.attr,
    &sensor_dev_attr_cpu_pcb_num.dev_attr.attr,
    &sensor_dev_attr_cpupld_ver_type.dev_attr.attr,
    &sensor_dev_attr_cpupld_ver.dev_attr.attr,
    &sensor_dev_attr_bdxde_plat_rst.dev_attr.attr,
    &sensor_dev_attr_bdxde_slp3_stat.dev_attr.attr,
    &sensor_dev_attr_bdxde_slp4_stat.dev_attr.attr,
    &sensor_dev_attr_bdxde_cpu_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_debug_mode.dev_attr.attr,
    &sensor_dev_attr_apwrok_stat.dev_attr.attr,
    &sensor_dev_attr_edge_prochot_sig_dis.dev_attr.attr,
    &sensor_dev_attr_psu_thermal_stat.dev_attr.attr,
    &sensor_dev_attr_pr_thermal_stat.dev_attr.attr,
    &sensor_dev_attr_me_drv_sig_en.dev_attr.attr,
    &sensor_dev_attr_cpu_thermal_stat.dev_attr.attr,
    &sensor_dev_attr_ddr_thermal_stat.dev_attr.attr,
    &sensor_dev_attr_sys_thermal_stat.dev_attr.attr,
    &sensor_dev_attr_debug_led3_en.dev_attr.attr,
    &sensor_dev_attr_debug_led2_en.dev_attr.attr,
    &sensor_dev_attr_debug_led1_en.dev_attr.attr,
    &sensor_dev_attr_debug_led0_en.dev_attr.attr,
    &sensor_dev_attr_cpu_standby_mode.dev_attr.attr,
    &sensor_dev_attr_cpld_rst.dev_attr.attr,
    &sensor_dev_attr_mb_power_stat.dev_attr.attr,
    &sensor_dev_attr_bios1_spi_wp.dev_attr.attr,
    &sensor_dev_attr_bios2_spi_wp.dev_attr.attr,
    &sensor_dev_attr_bios_mux_sel.dev_attr.attr,
    &sensor_dev_attr_gbe_spi_wp.dev_attr.attr,
    &sensor_dev_attr_pch_thermtrip_en.dev_attr.attr,
    &sensor_dev_attr_id_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_cpu_i2c_mux_en.dev_attr.attr,
    &sensor_dev_attr_cpu_i2c_mux_sel.dev_attr.attr,
    &sensor_dev_attr_psu_fan_intr.dev_attr.attr,
    &sensor_dev_attr_wd_timer.dev_attr.attr,
    &sensor_dev_attr_wd_en.dev_attr.attr,
    &sensor_dev_attr_wd_clear_flag.dev_attr.attr,
    NULL, 
};

static struct attribute_group agc032_cpupld_attr_group = {
    .attrs = agc032_cpupld_attrs,
};

static int __init cpupld_probe(struct platform_device *pdev)
{
    struct platform_data *pdata;
    struct i2c_adapter *parent;
    int rv;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPUPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS0);
    if (!parent) {
        printk(KERN_ERR "Parent adapter (%d) not found\n",BUS0);
        return -ENODEV;
    }

    pdata[cpld].client = i2c_new_dummy(parent, pdata[cpld].reg_addr);
    if (!pdata[cpld].client) {
        printk(KERN_ERR "Fail to create dummy i2c client for addr %d\n", pdata[cpld].reg_addr);
        goto error;
    }
#if 0
    /* set default cpu_i2c_mux 0x14 be 0x1 */
    rv = i2c_smbus_write_byte_data(pdata[cpld].client, 0x14, 0x1);
    if (rv < 0) {
           printk(KERN_WARNING, "Error: Failed to set addr 0x14.\n");
           goto error;
    }
#endif
    /* /sys/device/platform */
    rv = sysfs_create_group(&pdev->dev.kobj, &agc032_cpupld_attr_group);
    if (rv){
         printk(KERN_ERR "Fail to create cpupld attribute group");
        goto error;
    }
    return 0;

error:
    i2c_unregister_device(pdata[cpld].client);
    i2c_put_adapter(parent);
    return -ENODEV;
}


static int __exit cpupld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &agc032_cpupld_attr_group);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else {
        if (pdata[cpld].client) {
            if (!parent) {
                parent = (pdata[cpld].client)->adapter;
            }
            i2c_unregister_device(pdata[cpld].client);
        }
    }
    i2c_put_adapter(parent);
    return 0;
}


static struct platform_driver cpupld_driver = {
    .probe  = cpupld_probe,
    .remove = __exit_p(cpupld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-agc032-cpupld",
    },
};


static struct platform_data agc032_cpupld_platform_data[] = {
    [cpld] = {
        .reg_addr = CPUPLD_I2C_ADDR,
    },
};

static void device_release(struct device *dev)
{
    return;
}

static struct platform_device cpupld_agc032 = {
    .name               = "delta-agc032-cpupld",
    .id                 = 0,
    .dev                = {
                .platform_data   = agc032_cpupld_platform_data,
                .release         = device_release
    },
};

/* module initialization */
static int __init delta_agc032_cpupld_init(void)
{
    int rc = 0;
    printk(KERN_INFO "CPUPLD module initializating\n");

    /* register CPUPLD driver */
    rc = platform_driver_register(&cpupld_driver);
    if (rc < 0) {
        printk(KERN_ERR "Fail to register CPUPLD driver, rc = %d\n", rc);
        goto error_register_driver;
    }

    /* register CPUPLD device */
    rc = platform_device_register(&cpupld_agc032);
    if (rc) {
        printk(KERN_ERR "Fail to create cpupld device, rc = %d\n", rc);
        goto error_register_device;
    }
    return 0;

error_register_device:
    platform_driver_unregister(&cpupld_driver);
error_register_driver:
    return rc;
}

static void __exit delta_agc032_cpupld_exit(void)
{
    platform_device_unregister(&cpupld_agc032);
    platform_driver_unregister(&cpupld_driver);
}

module_init(delta_agc032_cpupld_init);
module_exit(delta_agc032_cpupld_exit);

MODULE_DESCRIPTION("DNI agc032 CPLD Platform Support");
MODULE_AUTHOR("James Ke <James.ke@deltaww.com>");
MODULE_LICENSE("GPL");
