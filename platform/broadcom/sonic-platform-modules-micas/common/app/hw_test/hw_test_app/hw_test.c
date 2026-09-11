#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <search.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <hw_test.h>
#include <hw_i2c/hw_i2c.h>
#include <hw_mtd/hw_mtdflash.h>
#include <hw_dram/hw_dram.h>
#include <hw_pci/hw_pci.h>
#include <hw_usb/hw_usb.h>
#include <hw_misc/hw_misc.h>
#include <hw_emmc/hw_emmc.h>

static struct hw_applet hw_applets[] = {
    /* I2C */
    HWTEST_APPLET(i2c_wr, "i2c_wr slave_addr offset data [data_len] [times] [offset_len]"),
    HWTEST_APPLET(i2c_rd, "i2c_rd slave_addr offset [data_len] [times] [offset_len]"),
    HWTEST_APPLET(i2c_chk, "i2c_chk slave_addr offset data [data_len] [times]"),
    HWTEST_APPLET(i2c_reset, "not support"),
    HWTEST_APPLET(pca9548_rd, "pca9548 i2c_bus slave_addr offset data [data_len] [times]"),
    HWTEST_APPLET(pca9548_wr, "pca9548 i2c_bus slave_addr offset data [data_len] [times]"),
    HWTEST_APPLET(rtc_rd, "rtc_rd rtc year month day hour minute second"),
    HWTEST_APPLET(rtc_wr, "rtc_wr rtc year month day hour minute second"),
    /* NorFlash */
    HWTEST_APPLET(nor_wr, "nor_wr start_addr end_addr data1 data2 [times]"),
    HWTEST_APPLET(nor_rd, "nor_rd start_addr end_addr data1 data2 [times]"),
    HWTEST_APPLET(nor_chk, "nor_chk start_addr end_addr data1 data2 [times]"),
    /* NandFlash */
    HWTEST_APPLET(nand_wr, "nand_wr start_addr end_addr data1 data2 [times]"),
    HWTEST_APPLET(nand_rd, "nand_rd start_addr end_addr data1 data2 [times]"),
    HWTEST_APPLET(nand_chk, "nand_chk start_addr end_addr data1 data2 [times]"),
    /* Dram */
    HWTEST_APPLET(dram_wr, "not support"),
    HWTEST_APPLET(dram_rd, "not support"),
    HWTEST_APPLET(dram_chk, "not support"),
    HWTEST_APPLET(dram_test, "dram_test [simple|complex some|complex all]"),
    /* PCI */
    HWTEST_APPLET(pci_wr, "pci_wr bus slot fn bar offset data [times]"),
    HWTEST_APPLET(pci_rd, "pci_rd bus slot fn bar offset data [times]"),
    HWTEST_APPLET(pci_dump, "pci_dump bus slot fn bar offset data [times]"),
    HWTEST_APPLET(pci_cfg_wr, "pci_cfg_wr bus slot fn offset data [times]"),
    HWTEST_APPLET(pci_cfg_rd, "pci_cfg_rd bus slot fn offset data [times]"),
    HWTEST_APPLET(pci_chk, "pci_chk bus slot fn bar offset data [times]"),
    /* USB */
    HWTEST_APPLET(usb_write, "not support"),
    HWTEST_APPLET(usb_read, "not support"),
    HWTEST_APPLET(usb_check, "not support"),
    /* eMMC */
    HWTEST_APPLET(emmc_test, "emmc_test"),
    /* MISC */
    HWTEST_APPLET(reload, "not support"),
    HWTEST_APPLET(sys_info, "not support"),
    HWTEST_APPLET(mem_dump, "not support"),
    HWTEST_APPLET(cache_flush, "not support"),
    HWTEST_APPLET(reg_wr64, "reg_wr64  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_rd64, "reg_rd64  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_wr32, "reg_wr32  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_rd32, "reg_rd32  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_wr16, "reg_wr16  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_rd16, "reg_rd16  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_wr8, "reg_wr8  reg_addr [reg_data]"),
    HWTEST_APPLET(reg_rd8, "reg_rd8  reg_addr [reg_data]"),
    /* phydev */
    HWTEST_APPLET(phydev_list, "phydev_list"),
    HWTEST_APPLET(phydev_rd, "phydev_rd  phy_index reg_addr"),
    HWTEST_APPLET(phydev_wr, "phydev_wr  phy_index reg_addr reg_data"),
    /* mdio bus */
    HWTEST_APPLET(mdiodev_list, "mdiodev_list"),
    HWTEST_APPLET(mdiodev_rd, "mdiodev_rd  mdio_index phyaddr reg_addr"),
    HWTEST_APPLET(mdiodev_wr, "mdiodev_wr  mdio_index phyaddr reg_addr reg_data"),
    /* hw help */
    HWTEST_APPLET(hw_help, "help")
};

static size_t hw_num_applets = (sizeof(hw_applets) / sizeof(struct hw_applet));

static void error_msg_and_die(const char *s, ...)
{
    va_list p;

    va_start(p, s);
    fflush(stdout);
    vfprintf(stderr, s, p);
    va_end(p);
    fputs("\r\n", stderr);
    exit(1);
}

static int applet_name_compare (const void *x, const void *y)
{
  const char *name = x;
  const struct hw_applet *applet = y;

  return strcmp (name, applet->name);
}

struct hw_applet *find_applet_by_name (const char *name)
{
  return lfind(name, hw_applets, &hw_num_applets, sizeof(struct hw_applet),
                  applet_name_compare);
}

static void show_usage ()
{
    size_t i;
    for (i = 0; i < hw_num_applets; i++) {
        printf("%-20s\t\t\t%s\r\n", hw_applets[i].name, hw_applets[i].help);
    }
    exit (0);
}

int hw_help_main(int argc, char **argv)
{
    argc = argc;
    argv = argv;
    show_usage();
    return 0;
}

static void run_applet_by_name (const char *name, int argc, char **argv)
{
    struct hw_applet *applet_using;

    if (argc == 0) {
        show_usage ();
    }

    if(!strncmp(name, "hw_test", 7)) {
        run_applet_by_name(argv[1], argc - 1, argv + 1);
    }

    applet_using = find_applet_by_name(name);
    if(applet_using) {
        if(argc == 2 && !strcmp(argv[1], "--help")) {
            show_usage ();
        }
        exit ((*(applet_using->main)) (argc, argv));
    }
}

int main(int argc, char **argv)
{
    char *applet_name, *s;
    applet_name = argv[0];

    for (s = applet_name; *s; ) {
        if (*(s++) == '/') {
            applet_name = s;
        }
    }

    run_applet_by_name(applet_name, argc, argv);
    error_msg_and_die("%s: applet not found", applet_name);
    return 0;
}