#ifndef _LINUX_FPGA_H
#define _LINUX_FPGA_H

enum FPGA_TYPE {
       FPGA_NONE = 0,
       FPGA
};

/*
 * port_info - optical port info
 * @index: front panel port index starting from 1
 * @typr: port type, see *PORT_TYPE*
 */
struct fpga_info {
        const char *name;
        unsigned int index;
        enum FPGA_TYPE type;
};

/*
 * fpga_platform_data - port xcvr private data
 * @fpga_reg_size: register range of each port
 * @num_ports: number of front panel ports
 * @devices: list of front panel port info
 */
struct fpga_platform_data {
        unsigned int fpga_reg_size;
        int num_ports;
        struct fpga_info *devices;
};

#endif /* _LINUX_I2C_CLS_H */
