/*
* $Copyright: (c) 2024 Broadcom.
* Broadcom Proprietary and Confidential. All rights reserved.$
*
*/
#ifndef PMBUS_REGULATOR_H_
#define PMBUS_REGULATOR_H_

double Vout_in_V(uint8_t vout_mode, uint16_t vout);
void SMBus_read_regulator_status(char *chip_name, bcm_plp_access_t plp_info, uint8_t regulator_i2c_addr);
void set_rail_address(char *chip_name, bcm_plp_access_t plp_info, uint8_t rail_addr, const uint16_t* r, uint8_t L);
uint16_t set_DVDD_voltage(char *chip_name, bcm_plp_access_t plp_info, uint8_t addr, uint8_t ch, uint16_t mV);
uint16_t get_DVDD_voltage(char *chip_name, bcm_plp_access_t plp_info, uint8_t addr, uint8_t ch);

#endif /* #ifndef PMBUS_REGULATOR_H_ */
