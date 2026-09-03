#ifndef _WB_GPIO_INIT_H_
#define _WB_GPIO_INIT_H_

#define GGPIOD_REQUESTED             (1 << 0)   /* GPIO has been requested */
#define GGPIOD_REQUESTED_BY_OTHER    (1 << 1)   /* Managed GPIO is requested by other modules */
#define GGPIOD_IS_OUT                (1 << 2)   /* GPIO is configured as output mode */
#define GGPIOD_IS_OUT_ACTIVE         (1 << 3)   /* GPIO is set to high level */

#define GPIO_OUTPUT_MODE             (0)        /* Output mode defined as 0 */
#define GPIO_INPUT_MODE              (1)        /* input mode defined as 1 */

#define GPIO_HIGH_LEVEL              (1)        /* High level defined as 1 */

/* GPIO port node data structure */
typedef struct gpio_port_s {
    struct list_head list;
    unsigned int gpio;                         /* GPIO number */
    int init_direction;                        /* Initial GPIO direction: 0 = output, 1 = input */
    int init_level;                            /* Initial level when GPIO is configured as output: 0 = low, 1 = high */
    const char * gpio_name;                    /* GPIO port name */
    int free_flag;                             /* GPIO release flag: determines whether the GPIO needs to be freed after request */
    unsigned long flag;                        /* GPIO node flag: tracks request/release status and input/output mode */
} gpio_port_t;

/* GPIO management structure: used to manage all GPIO port nodes */
typedef struct gpio_init_s {
    struct list_head list;                     /* Linked list for storing GPIO port nodes */
    struct device *dev;
    int gpio_num;                              /* Total number of GPIOs managed */
    spinlock_t gpio_init_lock;                 /* Spinlock for thread-safe GPIO management operations */
} gpio_init_t;

/**
 * @deprecated    Please use driver model instead
 * Request a GPIO. This should be called before any of the other functions
 * are used on this GPIO.
 *
 * Note: With driver model, the label is allocated so there is no need for
 * the caller to preserve it.
 *
 * @param gpio    GPIO number
 * @param label    User label for this GPIO
 * @return 0 if ok
 */
int bsp_gpio_request(unsigned int gpio, const char *label);

/**
 * @deprecated    Please use driver model instead
 * Stop using the GPIO.  This function should not alter pin configuration.
 *
 * @param gpio    GPIO number
 * @return void
 */
void bsp_gpio_free(unsigned int gpio);

/**
 * @deprecated    Please use driver model instead
 * Make a GPIO an input.
 *
 * @param gpio    GPIO number
 * @return 0 if ok
 */
int bsp_gpio_direction_input(unsigned int gpio);

/**
 * @deprecated    Please use driver model instead
 * Make a GPIO an output, and set its value.
 *
 * @param gpio    GPIO number
 * @param value    GPIO value (0 for low or 1 for high)
 * @return 0 if ok
 */
int bsp_gpio_direction_output(unsigned int gpio, int value);

/**
 * @deprecated    Please use driver model instead
 * Get a GPIO's value. This will work whether the GPIO is an input
 * or an output.
 *
 * @param gpio    GPIO number
 * @return 0 if low, 1 if high
 */
int bsp_gpio_get_value(unsigned int gpio);

/**
 * @deprecated    Please use driver model instead
 * Set an output GPIO's value. The GPIO must already be an output or
 * this function may have no effect.
 *
 * @param gpio    GPIO number
 * @param value    GPIO value (0 for low or 1 for high)
 * @return void
 */
void bsp_gpio_set_value(unsigned int gpio, int value);

#endif    /* _WB_GPIO_INIT_H_ */
