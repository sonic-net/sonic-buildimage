#ifndef __WB_EEPROM_93XX46_H
#define __WB_EEPROM_93XX46_H

#include <linux/gpio/consumer.h>

#define EEPROM_93XX46_QUIRK_SINGLE_WORD_READ    BIT(0)
#define EEPROM_93XX46_QUIRK_INSTRUCTION_LENGTH  BIT(1)

#define EE_ADDR8    0x01
#define EE_ADDR16   0x02
#define EE_READONLY 0x08

struct eeprom_93xx46_dev;

struct eeprom_93xx46_platform_data {
    unsigned int        flags;
    unsigned int        quirks;
    struct gpio_desc   *select;
    void (*prepare)(struct eeprom_93xx46_dev *);
    void (*finish) (struct eeprom_93xx46_dev *);
};

#endif
