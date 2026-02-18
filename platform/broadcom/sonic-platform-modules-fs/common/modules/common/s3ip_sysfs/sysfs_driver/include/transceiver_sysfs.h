#ifndef _TRANSCEIVER_SYSFS_H_
#define _TRANSCEIVER_SYSFS_H_

struct s3ip_sysfs_transceiver_drivers_s {
    int (*get_eth_number)(void);
    ssize_t (*get_transceiver_power_on_status)(char *buf, size_t count);
    int (*set_transceiver_power_on_status)(int status);
    ssize_t (*get_transceiver_present_status)(char *buf, size_t count);
    ssize_t (*get_eth_power_on_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_power_on_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_tx_fault_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_tx_disable_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_tx_disable_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_present_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_rx_los_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_reset_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_reset_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_low_power_mode_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_low_power_mode_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_interrupt_status)(unsigned int eth_index, char *buf, size_t count);
    int (*get_eth_eeprom_size)(unsigned int eth_index);
    ssize_t (*read_eth_eeprom_data)(unsigned int eth_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_eth_eeprom_data)(unsigned int eth_index, char *buf, loff_t offset, size_t count);
    ssize_t (*get_eth_optoe_type)(unsigned int sff_index, int *optoe_type, char *buf, size_t count);
    ssize_t (*set_eth_optoe_type)(unsigned int sff_index, int optoe_type);
    ssize_t (*get_eth_i2c_bus)(unsigned int eth_index, char *buf, size_t count);
	ssize_t (*get_eth_power_group)(unsigned int eth_index, int *power_group);
};

extern int s3ip_sysfs_sff_drivers_register(struct s3ip_sysfs_transceiver_drivers_s *drv);
extern void s3ip_sysfs_sff_drivers_unregister(void);
#define SINGLE_TRANSCEIVER_PRESENT_DEBUG_FILE       "/etc/sonic/.present_eth_%d"
#endif /*_TRANSCEIVER_SYSFS_H_ */
