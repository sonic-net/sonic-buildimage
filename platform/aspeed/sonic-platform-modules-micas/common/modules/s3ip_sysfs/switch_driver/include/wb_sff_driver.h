#ifndef _WB_SFF_DRIVER_H_
#define _WB_SFF_DRIVER_H_

#define SFF_OPTOE_TYPE_PATH             "/sys/bus/i2c/devices/%d-0050/dev_class"
#define OPTOE_TYPE_PATH_SIZE            (64)
#define OPTOE_TYPE_RW_LEN               (1)
#define DFD_E2PROM_MAX_LEN              (256)
#define DFD_E2PROM_PAGE_LEN             (128)
#define PORT_TYPE_OFFSET                (0)
#define PORT_TEMP_MAX                   (125)
#define PORT_TEMP_MIN                   (-20)
#define SFF_POLLING_RETRY_TIMES         (3)
#define SFF_POLLING_RETRY_MSLEEP        (10)

/* 1 means return power on when some eths are power on and other are power down*/
#define SFF_POWER_STATUS_MIX_DEFAULT_VALUE   1

#define SFF_MODULE_STATUS_MASK          (0x7)
#define SFF_TX_FAULT_SUP_MASK           (0x1)
#define SFF_TX_LOS_SUP_MASK             (0x2)
#define SFF_TX_CDR_LOL_SUP_MASK         (0x4)
#define SFF_RX_LOS_SUP_MASK             (0x2)
#define SFF_RX_CDR_LOL_SUP_MASK         (0x4)
#define SFF_HOST_SNR_SUP_MASK           (0x10)
#define SFF_MEDIA_SNR_SUP_MASK          (0x20)
#define SFF_TX_BIAS_SUP_MASK            (0x1)
#define SFF_TX_POWER_SUP_MASK           (0x2)
#define SFF_RX_POWER_SUP_MASK           (0x4)
#define SFF_BIAS_FACTOR_SUP_MASK        (0x18)
#define SFF_BIAS_FACTOR_X2              (0x08)
#define SFF_BIAS_FACTOR_X4              (0x10)
#define SFF_SNR_SELSET_VALUE            (0x6)
#define SFF_LANE_NUM                    (8)
#define SFF_GET_VALUE_LEN_MAX           (SFF_LANE_NUM * 2)

/* sff i2c bus status table */
typedef enum {
    SFF_I2C_STATUS_NORMAL           = 0,
    SFF_I2C_STATUS_ABNORMAL         = 1,
} sff_i2c_bus_status_t;

typedef struct {
    wb_sff_attr_t attr_type;
    int sfp_offset;
    int qsfp_offset;
    int cmis_offset;
} sff_attr_offset_map_t;

extern dfd_debug_data_key_map_t sff_dbg_key_table[WB_SFF_ATTR_END];
extern dfd_sysfs_func_map_t sff_func_table[WB_SFF_ATTR_END];

/**
 * dfd_get_sff_power_status_mix_default_value 
 * return: Success : the value present the power-on states of all Ethernet ports are inconsistent,
 *       : Failed: A negative value is returned
 */
int dfd_get_sff_power_status_mix_default_value(void);

/**
 * dfd_set_sff_cpld_info - Example Set the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @value: Writes the value to the register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value);


/**
 * dfd_get_sff_cpld_info_value - Obtain the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @sysfs_value: get the sysfs_decode_value
 * 
 * return: Success: Returns 0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_cpld_info_value(unsigned int sff_index, int cpld_reg_type, int *sysfs_value);

/**
 * dfd_get_sff_cpld_info - Obtain the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, unsigned int cpld_reg_type, char *buf, size_t count);

/**
 * dfd_get_sff_e2_page_info - Obtain the optical module E2 page info 
 * @sff_index: Optical module number, starting from 1
 * @page_type: 0-low page(offset 0~127); 1-high page(offset 128~255)
 * @buf: Optical module E2 receives low/high page information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_e2_page_info(unsigned int sff_index, int page_type, char *buf, size_t count);

/**
 * dfd_get_sff_temp - Obtain the optical module temp info 
 * @sff_index: Optical module number, starting from 1
 * @page_type: 0-low page(offset 0~127); 1-high page(offset 128~255)
 * @buf: Optical module E2 receives low/high page information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_temp(unsigned int sff_index, char *buf, size_t count);

/**
 * dfd_get_single_eth_optoe_type - get sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_optoe_type(unsigned int sff_index, int *optoe_type);

/**
 * dfd_set_single_eth_optoe_type - set sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_set_single_eth_optoe_type(unsigned int sff_index, int optoe_type);

/**
 * dfd_get_single_eth_i2c_bus - get sff i2c_bus
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_i2c_bus(unsigned int sff_index, char *buf, size_t count);

/**
 * dfd_get_single_eth_cage_type - get sff cage_type
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_cage_type(unsigned int sff_index, char *buf, size_t count);

/**
 * dfd_get_single_eth_power_group - get sff port power group
 * @sff_index: Optical module number, starting from 1
 * power_group: power group id
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_power_group(unsigned int sff_index, int *power_group);

/**
 * dfd_get_single_eth_port_bus_status - get sff bus status
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_port_bus_status(unsigned int sff_index, char *buf, size_t count);

/**
 * dfd_set_sff_power_group_state - Set the power state of an SFP power group.
 * @group: Power group number, starting from 1.
 * @group_state: Power state to set for the group (0 for off, 1 for on).
 *
 * Returns: 0 on success, or a negative error code on failure.
 */
int dfd_set_sff_power_group_state(int group, int group_state);

/**
 * dfd_get_sff_power_group_state - Obtain the power group state.
 * @group: Power group number, starting from 1.
 * @buf: Buffer to store the power group state information.
 * @count: buf length.
 * return: Success: Returns the length of fill buf.
 *        Failed: A negative value is returned.
 */
ssize_t dfd_get_sff_power_group_state(int group, char *buf, size_t count);

ssize_t dfd_get_transceiver_power_on_status_str(char *buf, size_t count);

int dfd_set_sff_cpld_value(unsigned int sff_index, unsigned int cpld_reg_type, void * val, unsigned int len);

ssize_t dfd_get_sff_rx_los(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_tx_disable(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_tx_fault(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_tx_los(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_tx_cdr_lol(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_rx_cdr_lol(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_module_status(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_datapath_status(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_host_snr(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_media_snr(unsigned int sff_index, unsigned int type, char *buf, size_t count);
ssize_t dfd_get_sff_diagnostic(unsigned int sff_index, unsigned int type, char *buf, size_t count);
/**
 * dfd_get_sff_present_status - Obtain the optical module present status
 * @sff_index: Optical module number, starting from 1
 * return: 0:ABSENT
 *         1:PRESENT
 *       : Negative value - Read failed
 */
int dfd_get_sff_present_status(unsigned int sff_index);

#endif /* _WB_SFF_DRIVER_H_ */
