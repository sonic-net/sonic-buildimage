#ifndef _GENERIC_CIC_H
#define _GENERIC_CIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "generic_cic_config.h"

#define irq_reg_info_valid(_reg_info_ptr) \
	((_reg_info_ptr)->mask != 0)

#define for_each_irq_reg(_priv, _reg_info_ptr) \
	for ((_reg_info_ptr) = _priv->irq_reg_table; \
	     irq_reg_info_valid((_reg_info_ptr)); \
	     (_reg_info_ptr)++)

#define REG_WIDTH CIC_REG_WIDTH

#define UINT32_C(c)  __UINT32_C(c)
#define UINT16_C(c)  __UINT16_C(c)

/*
 * Functions that must be implemented by the user. There are two
 * templates available.
 *
 *
 * Template #1: static tables and register offsets
 *
 * In the good old days, every cic driver would return static, hard-
 * coded tables and offsets to the generic infrastructure. For
 * instance the siril* cic drivers are built with:
 *
 *   -DCIENA_CIC_GET_MASTER_MASK='$(CIENA_REG)_get_master_mask'
 *
 * The implementation of the function is purely static. For example:
 *
 *   uint32_t rudra40_get_master_mask(void)
 *   {
 *   	return (uint32_t) RUDRA40_REG_OFFSET(RUDRA40_GLUE_ISM_MASTER_EVENT);
 *   }
 *
 *
 * Template #2: device-specific tables and register offsets
 *
 * The same cic driver can be implemented for several devices. In such
 * cases, the user functions must distinguish between devices. Here is
 * an example how this can be implemented. First, the cic platform
 * data is augmented with device-specific values:
 *
 *   struct my_irq_cic_config {
 *   	struct generic_cic_config base_config;
 *   	unsigned                  portnum;
 *   };
 *
 * Platform cic devices are created with the augmented config:
 *
 *   struct my_irq_cic_config micc = { ... };
 *   platform_device_register_resndata(..., &micc, sizeof(micc));
 *
 * The cic driver uses the augmented config to return a variable
 * register offset:
 *
 *   #define CIENA_CIC_MASTER_MASK my_fpga_master_mask
 *   uint32_t my_fpga_master_mask(struct device *dev)
 *   {
 *   	struct my_irq_cic_config *micc;
 *   	unsigned                  reg;
 *
 *   	micc = dev_get_platdata(dev);
 *   	reg  = MY_FPGA_ISM_MASTER_EVENT + micc->portnum;
 *
 *   	return (uint32_t) MY_FPGA_REG_OFFSET(reg);
 *   }
 */

#ifdef CIENA_CIC_MASTER_SUM
uint32_t CIENA_CIC_MASTER_SUM(struct device *dev);
#define CALL_CIC_MASTER_SUM(__d) CIENA_CIC_MASTER_SUM(__d)
#else
#ifndef CIENA_CIC_GET_MASTER_SUM
#define CIENA_CIC_GET_MASTER_SUM get_master_sum
#endif
uint32_t CIENA_CIC_GET_MASTER_SUM(void);
#define CALL_CIC_MASTER_SUM(__d) CIENA_CIC_GET_MASTER_SUM()
#endif
static inline uint32_t cic_master_sum(__maybe_unused struct device *dev)
{
	return CALL_CIC_MASTER_SUM(dev);
}

#ifdef CIENA_CIC_MASTER_MASK
uint32_t CIENA_CIC_MASTER_MASK(struct device *dev);
#define CALL_CIC_MASTER_MASK(__d) CIENA_CIC_MASTER_MASK(__d)
#else
#ifdef CIENA_CIC_GET_MASTER_MASK
uint32_t CIENA_CIC_GET_MASTER_MASK(void);
#else
#define CIENA_CIC_GET_MASTER_MASK get_master_mask
static inline uint32_t get_master_mask(void)
{
	return 0;
}
#endif
#define CALL_CIC_MASTER_MASK(__d) CIENA_CIC_GET_MASTER_MASK()
#endif
static inline uint32_t cic_master_mask(__maybe_unused struct device *dev)
{
	return CALL_CIC_MASTER_MASK(dev);
}

#ifdef CIENA_CIC_MSI_CTRL
uint32_t CIENA_CIC_MSI_CTRL(struct device *dev);
#define CALL_CIC_MSI_CTRL(__d) CIENA_CIC_MSI_CTRL(__d)
#else
#ifdef CIENA_CIC_GET_MSI_CTRL
uint32_t CIENA_CIC_GET_MSI_CTRL(void);
#else
#define CIENA_CIC_GET_MSI_CTRL get_msi_ctrl
static inline uint32_t get_msi_ctrl(void)
{
	return 0;
}
#endif
#define CALL_CIC_MSI_CTRL(__d) CIENA_CIC_GET_MSI_CTRL()
#endif
static inline uint32_t cic_msi_ctrl(__maybe_unused struct device *dev)
{
	return CALL_CIC_MSI_CTRL(dev);
}

#ifdef CIENA_CIC_INTERRUPT_TABLE
irq_level_t *CIENA_CIC_INTERRUPT_TABLE(struct device *dev);
#define CALL_CIC_INTERRUPT_TABLE(__d) CIENA_CIC_INTERRUPT_TABLE(__d)
#else
#ifndef CIENA_CIC_GET_INTERRUPT_TABLE
#define CIENA_CIC_GET_INTERRUPT_TABLE get_interrupt_table
#endif
irq_level_t *CIENA_CIC_GET_INTERRUPT_TABLE(void);
#define CALL_CIC_INTERRUPT_TABLE(__d) CIENA_CIC_GET_INTERRUPT_TABLE()
#endif
static inline irq_level_t *cic_interrupt_table(__maybe_unused struct device *d)
{
	return CALL_CIC_INTERRUPT_TABLE(d);
}

#ifdef CIENA_CIC_TO_STR
const char *CIENA_CIC_TO_STR(struct device *dev, uint32_t pin, uint32_t level);
#define CALL_CIC_TO_STR(__d, __p, __l) CIENA_CIC_TO_STR(__d, __p, __l)
#else
#ifndef CIENA_CIC_PIN_TO_STR
#define CIENA_CIC_PIN_TO_STR cic_pin_to_str
#endif
const char *CIENA_CIC_PIN_TO_STR(uint32_t pin, uint32_t level);
#define CALL_CIC_TO_STR(__d, __p, __l) CIENA_CIC_PIN_TO_STR(__p, __l)
#endif
static inline const char *cic_to_str(__maybe_unused struct device *dev,
				     uint32_t pin, uint32_t level)
{
	return CALL_CIC_TO_STR(dev, pin, level);
}

#ifdef CIENA_GPIO_LKUP
enum cic_gpio_fate CIENA_GPIO_LKUP(struct device *dev, uint32_t pin,
				   uint32_t level);
#define CALL_GPIO_LKUP(__d, __p, __l) CIENA_GPIO_LKUP(__d, __p, __l)
#else
#ifndef CIENA_CIC_GPIO_LKUP
#define CIENA_CIC_GPIO_LKUP cic_add_gpio_lkup
static inline bool cic_add_gpio_lkup(uint32_t pin, uint32_t level)
{
	return false;
}
#endif
bool CIENA_CIC_GPIO_LKUP(uint32_t pin, uint32_t level);
#define CALL_GPIO_LKUP(__d, __p, __l)					\
	(CIENA_CIC_GPIO_LKUP(__p, __l) ? cic_gpio_irq : cic_gpio_export)
#endif
static inline
enum cic_gpio_fate cic_gpio_lkup(__maybe_unused struct device *dev,
				 uint32_t pin, uint32_t level)
{
	return CALL_GPIO_LKUP(dev, pin, level);
}

#ifdef CIENA_GPIO_ACTIVE_LOW
bool CIENA_GPIO_ACTIVE_LOW(struct device *dev, uint32_t pin, uint32_t level);
#define CALL_GPIO_ACTIVE_LOW(__d, __p, __l) CIENA_GPIO_ACTIVE_LOW(__d, __p, __l)
#else
#define CALL_GPIO_ACTIVE_LOW(__d, __p, __l) false
#endif
static inline bool cic_gpio_active_low(__maybe_unused struct device *dev,
				 uint32_t pin, uint32_t level)
{
	return CALL_GPIO_ACTIVE_LOW(dev, pin, level);
}

#include "generic_cic_priv.h"

#ifdef __cplusplus
}
#endif
#endif /* _GENERIC_CIC_H */
