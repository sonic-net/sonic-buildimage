/*
 * wb_wdt.c
 * ko for watchdog function
 */

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/hrtimer.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>

#include "wb_wdt.h"
#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>

#define WDT_READY_NO_CFG     (0)
#define WDT_IS_READY         (1)
#define WDT_NOT_READY        (2)

#define ONE_BYTE             (1)

#define WDT_OFF              (0)
#define WDT_ON               (1)

#define MS_TO_S              (1000)
#define MS_TO_NS             (1000 * 1000)

#define MAX_REG_VAL          (255)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

enum {
    HW_ALGO_TOGGLE,
    HW_ALGO_LEVEL,
    HW_ALGO_EIGENVALUES,
};

enum {
    WATCHDOG_DEVICE_TYPE    = 0,
    HRTIMER_TYPE            = 1,
    THREAD_TYPE             = 2,
};

typedef struct wb_wdt_priv_s {
    /* GPIO feeding dog and logic device feeding dog common variables */
    struct task_struct *thread;
    struct hrtimer hrtimer;
    ktime_t   m_kt;
    const char  *config_dev_name;   /* Configure the logical device name, including the enable and set time  */
    uint8_t     config_mode;        /* 1:gpio feed the dog, 2: logic device feed the dog */
    uint8_t     hw_algo;            /* 1:Flip feed dog, 2: level feed dog, 3: feature value feed dog */
    uint8_t     enable_val;         /* The valid value is enabled */
    uint8_t     disable_val;        /* The valid value is disabled */
    uint8_t     enable_mask;        /* Enable the mask */
    uint8_t     priv_func_mode;     /* 1:i2c, 2:pcie, 3:io, 4:device Files, 5:platform i2c */
    uint8_t     feed_wdt_type;      /* 0: does not feed the dog, 1:hrtimer feeds the dog, 2: threads feed the dog */
    uint32_t    enable_reg;         /* Enable register address */
    uint32_t    timeout_cfg_reg;    /* Timeout Sets the register address */
    uint32_t    timeleft_cfg_reg;
    uint32_t    hw_margin;          /* Timeout period */
    uint32_t    feed_time;          /* Dog feeding time */
    uint8_t     timer_accuracy_reg_flag;
    uint32_t    timer_accuracy_reg;
    uint8_t     timer_accuracy_reg_val;
    uint32_t    timer_accuracy;     /* Timer accuracy (time when cpld is configured with 1) */
    uint8_t     timer_update_reg_flag;
    uint32_t    timer_update_reg;
    uint8_t     timer_update_reg_val;
    gpio_wdt_info_t     gpio_wdt;   /* gpio feeds the dog */
    logic_wdt_info_t    logic_wdt;  /* Logic device to the dog */
    struct device *dev;
    const struct attribute_group *sysfs_group;
    struct mutex update_lock;
    struct watchdog_device  wdd;
    uint32_t wdt_ready;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
    bool no_feed_dog_flag;           /* block dog feeding flag, for testing purposes */
    bool set_new_timeout_flag;       /* set a new watchdog timeout flag */
    int set_timeout_value;          /* the timeout set via sysfs */
}wb_wdt_priv_t;

static int wb_wdt_read(wb_wdt_priv_t *priv, const char *path,
                uint32_t offset, uint8_t wdt_type, uint8_t *buf, size_t count)
{
    device_func_read pfunc;

    switch (wdt_type) {
        case PARENT_WDT:
            pfunc = (device_func_read)priv->read_intf_addr;
            break;
        case SECOND_LOGIC_WDT:
            pfunc = (device_func_read)priv->logic_wdt.read_intf_addr;
            break;
        default:
            DEBUG_ERROR("wdt_type %u unsupport.\n", wdt_type);
            return -EINVAL;
    }
    return pfunc(path, offset, buf, count);
}

static int wb_wdt_write(wb_wdt_priv_t *priv, const char *path,
                uint32_t offset, uint8_t wdt_type, uint8_t *buf, size_t count)
{
    device_func_write pfunc;

    switch (wdt_type) {
        case PARENT_WDT:
            pfunc = (device_func_write)priv->write_intf_addr;
            break;
        case SECOND_LOGIC_WDT:
            pfunc = (device_func_write)priv->logic_wdt.write_intf_addr;
            break;
        default:
            DEBUG_ERROR("wdt_type %u unsupport.\n", wdt_type);
            return -EINVAL;
    }
    return pfunc(path, offset, buf, count);
}

static int wb_wdt_enable_ctrl(wb_wdt_priv_t *priv, uint8_t flag)
{
    int ret;
    uint8_t val;
    uint8_t ctrl_val;

    /* case: Can't not enable/disable WDT */
    if(priv->enable_reg == INVALID_REG_ADDR) {
        dev_dbg(priv->dev, "Can't not enable/disable WDT.\n");
        return 0;
    }

    switch (flag) {
    case WDT_ON:
        ctrl_val = priv->enable_val;
        break;
    case WDT_OFF:
        ctrl_val = priv->disable_val;
        break;
    default:
        DEBUG_ERROR("unsupport wdt enable ctrl:%u.\n", flag);
        return -EINVAL;
    }

    /* If wdt is enabled, read before write does not change the value of other bits */
    val = 0;
    ret = wb_wdt_read(priv, priv->config_dev_name,
                priv->enable_reg, PARENT_WDT, &val, ONE_BYTE);
    if (ret < 0) {
        dev_err(priv->dev, "read wdt control reg error.\n");
        return ret;
    }

    /* If wdt is enabled, read before write does not change the value of other bits */
    val &= ~priv->enable_mask;

    val |= ctrl_val & priv->enable_mask;

    ret = wb_wdt_write(priv, priv->config_dev_name,
                priv->enable_reg, PARENT_WDT, &val, ONE_BYTE);
    if (ret < 0) {
        dev_err(priv->dev, "write wdt control reg error.\n");
        return ret;
    }

    return 0;
}

static void wdt_hwping(wb_wdt_priv_t *priv)
{
    gpio_wdt_info_t *gpio_wdt;
    logic_wdt_info_t *logic_wdt;
    uint8_t tmp_val;
    int ret;

    if (priv->wdt_ready == WDT_NOT_READY) {
        DEBUG_VERBOSE("wdt not ready, do nothing\n");
        return;
    }

    if (priv->config_mode == GPIO_FEED_WDT_MODE) {
        gpio_wdt = &priv->gpio_wdt;
        switch (priv->hw_algo) {
        case HW_ALGO_TOGGLE:
            gpio_wdt = &priv->gpio_wdt;
            gpio_wdt->state = !gpio_wdt->state;
            gpio_set_value_cansleep(gpio_wdt->gpio, gpio_wdt->state);
            DEBUG_VERBOSE("gpio toggle wdt work. val:%u\n", gpio_wdt->state);
            break;
        case HW_ALGO_LEVEL:
            gpio_wdt = &priv->gpio_wdt;
            /* Pulse */
            gpio_set_value_cansleep(gpio_wdt->gpio, !gpio_wdt->active_low);
            udelay(1);
            gpio_set_value_cansleep(gpio_wdt->gpio, gpio_wdt->active_low);
            DEBUG_VERBOSE("gpio level wdt work.\n");
            break;
        }
    } else {
        logic_wdt = &priv->logic_wdt;
        switch (priv->hw_algo) {
        case HW_ALGO_TOGGLE:
            logic_wdt->active_val = !logic_wdt->active_val;
            ret = wb_wdt_write(priv, logic_wdt->feed_dev_name,
                        logic_wdt->feed_reg, SECOND_LOGIC_WDT, &logic_wdt->active_val, ONE_BYTE);
            if (ret < 0) {
                DEBUG_ERROR("logic toggle wdt write failed.ret = %d\n", ret);
            }
            DEBUG_VERBOSE("logic toggle wdt work.\n");
            break;
        case HW_ALGO_LEVEL:
            tmp_val = !logic_wdt->active_val;
            ret = wb_wdt_write(priv, logic_wdt->feed_dev_name,
                        logic_wdt->feed_reg, SECOND_LOGIC_WDT, &tmp_val, ONE_BYTE);
            if (ret < 0) {
                DEBUG_ERROR("logic level wdt write first failed.ret = %d\n", ret);
            }
            udelay(1);
            ret = wb_wdt_write(priv, logic_wdt->feed_dev_name,
                        logic_wdt->feed_reg, SECOND_LOGIC_WDT, &logic_wdt->active_val, ONE_BYTE);
            if (ret < 0) {
                DEBUG_ERROR("logic level wdt write second failed.ret = %d\n", ret);
            }
            DEBUG_VERBOSE("logic level wdt work.\n");
            break;
        case HW_ALGO_EIGENVALUES:
            ret = wb_wdt_write(priv, logic_wdt->feed_dev_name,
                        logic_wdt->feed_reg, SECOND_LOGIC_WDT, &logic_wdt->active_val, ONE_BYTE);
            if (ret < 0) {
                DEBUG_ERROR("logic eigenvalues wdt write failed, path: %s, mode: %d, reg: 0x%x, val: 0x%x, ret: %d\n",
                    logic_wdt->feed_dev_name, logic_wdt->logic_func_mode, logic_wdt->feed_reg,
                    logic_wdt->active_val, ret);
            }
            DEBUG_VERBOSE("logic eigenvalues wdt work, path: %s, mode: %d, reg: 0x%x, val: 0x%x\n",
                logic_wdt->feed_dev_name, logic_wdt->logic_func_mode, logic_wdt->feed_reg, logic_wdt->active_val);
            break;
        }
    }
    return;
}

/* set new wdt timeout */
static int feed_wdt_set_sysfs_timeout(wb_wdt_priv_t *priv)
{
    int ret;
    struct watchdog_device *wdd;

    wdd = &priv->wdd;

    if (priv->set_new_timeout_flag == false) {
        return -EINVAL;
    }

    /* clear flag first */
    priv->set_new_timeout_flag = false;
    /* stop wdt before set timeout */
    ret = wdd->ops->stop(wdd);
    if (ret < 0) {
        /* return when stop wdt fail */
        DEBUG_ERROR("stop hrtimer watchdog failed, ret=%d.\n", ret);
        return ret;
    }

    /* set new timeout */
    ret = wdd->ops->set_timeout(wdd, priv->set_timeout_value / MS_TO_S);
    if (ret < 0) {
        DEBUG_ERROR("set_timeput watchdog failed, ret=%d.\n", ret);
    } else {
        DEBUG_VERBOSE("set watchdog timeout success, ret=%d.\n", ret);
    }

    /* restart wdt */
    ret += wdd->ops->start(wdd);
    if (ret < 0) {
        DEBUG_ERROR("start watchdog failed, ret=%d.\n", ret);
    } else {
        DEBUG_VERBOSE("set watchdog restart success, ret=%d.\n", ret);
    }

    return ret;
}

static enum hrtimer_restart hrtimer_hwping(struct hrtimer *timer)
{
    int ret;

    wb_wdt_priv_t *priv = container_of(timer, wb_wdt_priv_t, hrtimer);

    if (priv->set_new_timeout_flag) {
        ret = feed_wdt_set_sysfs_timeout(priv);
        if (ret < 0) {
            DEBUG_INFO("hrtimer:thread timer:set new timeout value fail.\n");
        }
    }

    if (priv->no_feed_dog_flag) {
        DEBUG_INFO("hrtimer:no feed dog.\n");
    } else {
        wdt_hwping(priv);
        DEBUG_INFO("HRTIMER PING \n");
    }

    hrtimer_forward(timer, timer->base->get_time(), priv->m_kt);
    return HRTIMER_RESTART;
}

static int thread_timer_cfg(wb_wdt_priv_t *priv, wb_wdt_device_t *wb_wdt_device)
{
    struct device *dev;
    uint32_t hw_margin;
    uint32_t feed_time;
    uint32_t accuracy;
    uint8_t set_time_val;
    int ret;

    dev = priv->dev;

    ret = 0;
    if (dev->of_node) {
        ret += of_property_read_u32(dev->of_node, "feed_time", &priv->feed_time);
        if (ret != 0) {
            dev_err(dev, "thread Failed to priv dts.\n");
            return -ENXIO;
        }
    } else {
        priv->feed_time = wb_wdt_device->feed_time;
    }
    DEBUG_VERBOSE("thread priv->feed_time: %u.\n", priv->feed_time);

    hw_margin = priv->hw_margin;
    feed_time = priv->feed_time;
    accuracy = priv->timer_accuracy;

    if ((feed_time > (hw_margin / 2)) || (feed_time == 0)) {
        dev_err(dev, "thread timer feed_time[%d] should be less than half hw_margin or zero.\n", feed_time);
        return -EINVAL;
    }

    if (priv->timer_accuracy_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_accuracy_reg, PARENT_WDT, &priv->timer_accuracy_reg_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(priv->dev, "set timer_accuracy_reg error, reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_accuracy_reg, priv->timer_accuracy_reg_val, ret);
            return ret;
        }
    }

    set_time_val = hw_margin / accuracy;
    ret = wb_wdt_write(priv, priv->config_dev_name,
                priv->timeout_cfg_reg, PARENT_WDT, &set_time_val, ONE_BYTE);
    if (ret < 0) {
        dev_err(dev, "set wdt thread timer reg error.\n");
        return ret;
    }

    if (priv->timer_update_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_update_reg, PARENT_WDT, &priv->timer_update_reg_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(priv->dev, "set timer_update_reg error,  reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_update_reg, priv->timer_update_reg_val, ret);
            return ret;
        }
    }

    return 0;
}

static int wdt_thread_timer(void *data)
{
    int ret;
    wb_wdt_priv_t *priv = data;

    while (!kthread_should_stop()) {
        if (priv->set_new_timeout_flag) {
            ret = feed_wdt_set_sysfs_timeout(priv);
            if (ret < 0) {
                DEBUG_INFO("thread timer:set new timeout value fail.\n");
            }
        }

        if (priv->no_feed_dog_flag) {
            DEBUG_INFO("thread timer:no feed dog.\n");
            continue;
        }

        DEBUG_INFO("THREAD PING \n");
        wdt_hwping(priv);
        schedule_timeout_uninterruptible(msecs_to_jiffies(priv->feed_time));
    }
    return 0;
}

static int thread_timer_create(wb_wdt_priv_t *priv, wb_wdt_device_t *wb_wdt_device)
{
    struct task_struct *p;
    int ret;

    /* Timeout configuration */
    ret = thread_timer_cfg(priv, wb_wdt_device);
    if (ret < 0) {
        dev_err(priv->dev, "set wdt thread timer failed.\n");
        return ret;
    }

    /* Create a dog feed thread */
    p = kthread_create(wdt_thread_timer, (void *)priv, "%s", "wb_wdt");
    if (!IS_ERR(p)) {
        DEBUG_VERBOSE("timer thread create success.\n");
        priv->thread = p;
        wake_up_process(p);
    } else {
        dev_err(priv->dev, "timer thread create failed.\n");
        return -ENXIO;
    }

    /* Enable the watchdog */
    ret = wb_wdt_enable_ctrl(priv, WDT_ON);
    if (ret < 0) {
        dev_err(priv->dev, "thread enable wdt failed.\n");
        kthread_stop(p);
        return -ENXIO;
    }

    return 0;
}

static int hrtimer_cfg(wb_wdt_priv_t *priv, wb_wdt_device_t *wb_wdt_device)
{
    struct device *dev;
    struct hrtimer *hrtimer;
    uint8_t set_time_val;
    uint8_t hrtimer_s;
    uint32_t hrtimer_ns;
    int ret;
    uint32_t hw_margin;
    uint32_t feed_time;
    uint32_t accuracy;
    uint32_t max_timeout;

    dev = priv->dev;

    ret = 0;
    if (dev->of_node) {
        ret += of_property_read_u32(dev->of_node, "feed_time", &priv->feed_time);
        if (ret != 0) {
            dev_err(dev, "hrtimer Failed to priv dts.\n");
            return -ENXIO;
        }
    } else {
        priv->feed_time = wb_wdt_device->feed_time;
    }
    DEBUG_VERBOSE("hrtimer priv->feed_time: %u.\n", priv->feed_time);

    hrtimer = &priv->hrtimer;
    hw_margin = priv->hw_margin;
    feed_time = priv->feed_time;
    accuracy = priv->timer_accuracy;
    max_timeout = accuracy * 255;

    if (hw_margin < accuracy || hw_margin > max_timeout) {
        dev_err(dev, "hrtimer_hw_margin should be between %u and %u.\n",
            accuracy, max_timeout);
        return -EINVAL;
    }
    if ((feed_time > (hw_margin / 2)) || (feed_time == 0)) {
        dev_err(dev, "feed_time[%d] should be less than half hw_margin or zeor.\n", feed_time);
        return -EINVAL;
    }

    hrtimer_s = feed_time / MS_TO_S;                /* The second portion of the time it takes to safely feed your dog */
    hrtimer_ns = (feed_time % MS_TO_S) * MS_TO_NS;  /* The nanosecond portion of safe dog feeding time */
    set_time_val = hw_margin / accuracy;            /* Values written to the logic device */

    if (priv->timer_accuracy_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_accuracy_reg, PARENT_WDT, &priv->timer_accuracy_reg_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(priv->dev, "set timer_accuracy_reg error, reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_accuracy_reg, priv->timer_accuracy_reg_val, ret);
            return ret;
        }
    }

    ret = wb_wdt_write(priv, priv->config_dev_name,
                priv->timeout_cfg_reg, PARENT_WDT, &set_time_val, ONE_BYTE);
    if (ret < 0) {
        dev_err(dev, "set wdt time reg error.\n");
        return ret;
    }

    if (priv->timer_update_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_update_reg, PARENT_WDT, &priv->timer_update_reg_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(priv->dev, "set timer_update_reg error,  reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_update_reg, priv->timer_update_reg_val, ret);
            return ret;
        }
    }

    /* Example Initialize the hrtimer and enable the hrtimer to feed the dog */
    priv->m_kt = ktime_set(hrtimer_s, hrtimer_ns);
    hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hrtimer->function = hrtimer_hwping;
    hrtimer_start(hrtimer, priv->m_kt, HRTIMER_MODE_REL);

    ret = wb_wdt_enable_ctrl(priv, WDT_ON);
    if (ret < 0) {
        dev_err(dev, "hrtimer enable wdt failed.\n");
        return -ENXIO;
    }

    return 0;
}

static int wb_wdt_ping(struct watchdog_device *wdd)
{
    wb_wdt_priv_t *priv = watchdog_get_drvdata(wdd);

    wdt_hwping(priv);
    return 0;
}

static int wb_wdt_start(struct watchdog_device *wdd)
{
    wb_wdt_priv_t *priv = watchdog_get_drvdata(wdd);
    int ret;

    ret = wb_wdt_enable_ctrl(priv, WDT_ON);
    if (ret < 0) {
        DEBUG_ERROR("start wdt enable failed.\n");
        return -ENXIO;
    }
    set_bit(WDOG_HW_RUNNING, &wdd->status);
    return 0;
}

static int wb_wdt_stop(struct watchdog_device *wdd)
{
    wb_wdt_priv_t *priv = watchdog_get_drvdata(wdd);
    int ret;

    ret = wb_wdt_enable_ctrl(priv, WDT_OFF);
    if (ret < 0) {
        DEBUG_ERROR("stop wdt enable failed.\n");
        return -ENXIO;
    }
    clear_bit(WDOG_HW_RUNNING, &wdd->status);
    return 0;
}

static int wb_wdt_set_timeout(struct watchdog_device *wdd, unsigned int t)
{
    wb_wdt_priv_t *priv = watchdog_get_drvdata(wdd);
    uint32_t timeout_ms;
    uint32_t accuracy;
    uint8_t set_time_val;
    int ret;

    accuracy = priv->timer_accuracy;
    timeout_ms = t * 1000;
    if (timeout_ms > accuracy * 255) {
        DEBUG_ERROR("set wdt timeout too larger error.timeout_ms:%u\n", timeout_ms);
        return -EINVAL;
    }

    if (priv->timer_accuracy_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_accuracy_reg, PARENT_WDT, &priv->timer_accuracy_reg_val, ONE_BYTE);
        if (ret < 0) {
            DEBUG_ERROR("set timer_accuracy_reg error, reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_accuracy_reg, priv->timer_accuracy_reg_val, ret);
            return ret;
        }
    }

    set_time_val = timeout_ms / accuracy;
    ret = wb_wdt_write(priv, priv->config_dev_name,
                priv->timeout_cfg_reg, PARENT_WDT, &set_time_val, ONE_BYTE);
    if (ret < 0) {
        DEBUG_ERROR("set wdt timeout reg error, set_time_val:%u ret:%d\n", set_time_val, ret);
        return ret;
    }

    if (priv->timer_update_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_update_reg, PARENT_WDT, &priv->timer_update_reg_val, ONE_BYTE);
        if (ret < 0) {
            DEBUG_ERROR("set timer_update_reg error,  reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_update_reg, priv->timer_update_reg_val, ret);
            return ret;
        }
    }

    wdd->timeout = t;

    return 0;
}

static unsigned int wb_wdt_get_timeleft(struct watchdog_device *wdd)
{
    wb_wdt_priv_t *priv = watchdog_get_drvdata(wdd);
    unsigned int time_left;
    uint32_t accuracy;
    uint8_t get_time_val;
    int ret;

    accuracy = priv->timer_accuracy;

    get_time_val = 0;
    ret = wb_wdt_read(priv, priv->config_dev_name,
                priv->timeleft_cfg_reg, PARENT_WDT, &get_time_val, ONE_BYTE);
    if (ret < 0) {
        DEBUG_ERROR("get wdt timeout reg error.ret:%d\n", ret);
        return ret;
    }
    time_left = get_time_val * accuracy / MS_TO_S;

    DEBUG_VERBOSE("get wdt timeleft %d get_time_val %d accuracy=%d\n",
            time_left, get_time_val, accuracy);
    return time_left;
}

static const struct watchdog_info wb_wdt_ident = {
    .options    = WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT,
    .firmware_version = 0,
    .identity   = "CPLD Watchdog",
};

static const struct watchdog_ops wb_wdt_ops = {
    .owner        = THIS_MODULE,
    .start        = wb_wdt_start,
    .stop         = wb_wdt_stop,
    .ping         = wb_wdt_ping,
    .set_timeout  = wb_wdt_set_timeout,
    .get_timeleft  = wb_wdt_get_timeleft,
};

static int wb_wdt_register_device(wb_wdt_priv_t *priv)
{
    int ret;

    watchdog_set_drvdata(&priv->wdd, priv);

    priv->wdd.info         = &wb_wdt_ident;
    priv->wdd.ops          = &wb_wdt_ops;
    priv->wdd.bootstatus   = 0;
    priv->wdd.timeout      = priv->hw_margin / MS_TO_S;
    priv->wdd.min_timeout  = priv->timer_accuracy / MS_TO_S;
    priv->wdd.max_timeout  = priv->timer_accuracy * MAX_REG_VAL / MS_TO_S;
    priv->wdd.parent       = priv->dev;

    watchdog_stop_on_reboot(&priv->wdd);

    ret = devm_watchdog_register_device(priv->dev, &priv->wdd);
    if (ret != 0) {
        dev_err(priv->dev, "cannot register watchdog device (err=%d)\n", ret);
        return -ENXIO;
    }

    return 0;
}

static int watchdog_device_cfg(wb_wdt_priv_t *priv)
{
    int ret;
    uint8_t set_time_val;

    ret = wb_wdt_enable_ctrl(priv, WDT_OFF);
    if (ret < 0) {
        dev_err(priv->dev, "probe disable wdt failed.\n");
        return -ENXIO;
    }

    if (priv->timer_accuracy_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_accuracy_reg, PARENT_WDT, &priv->timer_accuracy_reg_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(priv->dev, "set timer_accuracy_reg error, reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_accuracy_reg, priv->timer_accuracy_reg_val, ret);
            return ret;
        }
    }

    set_time_val = priv->hw_margin / priv->timer_accuracy;
    ret = wb_wdt_write(priv, priv->config_dev_name,
                priv->timeout_cfg_reg, PARENT_WDT, &set_time_val, ONE_BYTE);
    if (ret < 0) {
        dev_err(priv->dev, "set wdt time reg error.\n");
        return ret;
    }

    if (priv->timer_update_reg_flag != 0) {
        ret = wb_wdt_write(priv, priv->config_dev_name,
                    priv->timer_update_reg, PARENT_WDT, &priv->timer_update_reg_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(priv->dev, "set timer_update_reg error,  reg_addr: 0x%x, value: 0x%x, ret: %d.\n",
                priv->timer_update_reg, priv->timer_update_reg_val, ret);
            return ret;
        }
    }

    return 0;
}

static int logic_wdt_init(wb_wdt_priv_t *priv, wb_wdt_device_t *wb_wdt_device)
{
    struct device *dev;
    logic_wdt_info_t *logic_wdt;
    int ret;

    dev = priv->dev;
    logic_wdt = &priv->logic_wdt;

    ret = 0;
    if (dev->of_node) {
        ret += of_property_read_string(dev->of_node, "feed_dev_name", &logic_wdt->feed_dev_name);
        ret += of_property_read_u32(dev->of_node, "feed_reg", &logic_wdt->feed_reg);
        ret += of_property_read_u8(dev->of_node, "active_val", &logic_wdt->active_val);
        ret += of_property_read_u8(dev->of_node, "logic_func_mode", &logic_wdt->logic_func_mode);
        if (ret != 0) {
            dev_err(dev, "Failed to logic_wdt dts.\n");
            return -ENXIO;
        }
    } else {
        logic_wdt->feed_dev_name = wb_wdt_device->wdt_config_mode.logic_wdt.feed_dev_name;
        logic_wdt->feed_reg = wb_wdt_device->wdt_config_mode.logic_wdt.feed_reg;
        logic_wdt->active_val = wb_wdt_device->wdt_config_mode.logic_wdt.active_val;
        logic_wdt->logic_func_mode = wb_wdt_device->wdt_config_mode.logic_wdt.logic_func_mode;
    }

    logic_wdt->state_val = logic_wdt->active_val;

    ret = find_intf_addr(&logic_wdt->write_intf_addr, &logic_wdt->read_intf_addr, logic_wdt->logic_func_mode);
    if (ret) {
        dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", logic_wdt->logic_func_mode, ret);
        return ret;
    }

    if (!logic_wdt->write_intf_addr || !logic_wdt->read_intf_addr) {
        dev_err(dev, "Fail: logic_wdt func mode %u rw symbol undefined.\n", logic_wdt->logic_func_mode);
        return -ENOSYS;
    }

    DEBUG_VERBOSE("feed_dev_name:%s, feed_reg:0x%x, active_val:%u, logic_func_mode:%u\n",
        logic_wdt->feed_dev_name, logic_wdt->feed_reg,
        logic_wdt->active_val, logic_wdt->logic_func_mode);

    return 0;
}

static int gpio_wdt_init(wb_wdt_priv_t *priv, wb_wdt_device_t *wb_wdt_device)
{
    struct device *dev;
    gpio_wdt_info_t *gpio_wdt;
    enum of_gpio_flags flags;
    uint32_t f = 0;
    int ret;

    dev = priv->dev;
    gpio_wdt = &priv->gpio_wdt;

    if (dev->of_node) {
        flags = OF_GPIO_ACTIVE_LOW;
        gpio_wdt->gpio = of_get_gpio_flags(dev->of_node, 0, &flags);
    } else {
        gpio_wdt->gpio = wb_wdt_device->wdt_config_mode.gpio_wdt.gpio;
        flags = wb_wdt_device->wdt_config_mode.gpio_wdt.flags;
    }
    if (!gpio_is_valid(gpio_wdt->gpio)) {
        dev_err(dev, "gpio is invalid.\n");
        return gpio_wdt->gpio;
    }

    gpio_wdt->active_low = flags & OF_GPIO_ACTIVE_LOW;

    if(priv->hw_algo == HW_ALGO_TOGGLE) {
        f = GPIOF_IN;
    } else if (priv->hw_algo == HW_ALGO_LEVEL) {
        f = gpio_wdt->active_low ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW;
    } else {
        dev_err(dev, "gpio not support hw_algo type: %d.\n", priv->hw_algo);
        return -EINVAL;
    }

    ret = devm_gpio_request_one(dev, gpio_wdt->gpio, f,
                dev_name(dev));
    if (ret) {
        dev_err(dev, "devm_gpio_request_one failed.\n");
        return ret;
    }

    gpio_wdt->state = gpio_wdt->active_low;
    gpio_direction_output(gpio_wdt->gpio, gpio_wdt->state);

    DEBUG_VERBOSE("active_low:%d\n", gpio_wdt->active_low);
    return 0;
}

static ssize_t set_wdt_sysfs_value(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count)
{
    wb_wdt_priv_t *priv = dev_get_drvdata(dev);
    int ret, val;

    val = 0;
    sscanf(buf, "%d", &val);
    DEBUG_VERBOSE("set wdt, val:%d.\n", val);

    if (val < 0 || val > 255) {
        DEBUG_ERROR("set wdt val %d failed.\n", val);
        return -EINVAL;
    }

    mutex_lock(&priv->update_lock);

    ret = wb_wdt_enable_ctrl(priv, val);
    if (ret < 0) {
        DEBUG_ERROR("set wdt sysfs value:%u failed.\n", val);
        goto fail;
    }

    DEBUG_VERBOSE("set wdt sysfs value:%u successed.\n", val);
    mutex_unlock(&priv->update_lock);
    return count;

fail:
    mutex_unlock(&priv->update_lock);
    return ret;
}

static ssize_t show_wdt_sysfs_value(struct device *dev,
                        struct device_attribute *da, char *buf)
{
    wb_wdt_priv_t *priv = dev_get_drvdata(dev);
    uint8_t val, status;
    int ret;

    /* case: Can't not enable/disable WDT */
    if(priv->enable_reg == INVALID_REG_ADDR) {
        /* default is ON */
        status = WDT_ON;
        dev_dbg(priv->dev, "Can't not enable/disable WDT, default is ON. status:%d.\n", status);
        return sprintf(buf, "%u\n", status);
    }

    mutex_lock(&priv->update_lock);

    val = 0;
    /* First read the value val of the enable register */
    ret = wb_wdt_read(priv, priv->config_dev_name,
                priv->enable_reg, PARENT_WDT, &val, ONE_BYTE);
    if (ret < 0) {
        dev_err(priv->dev, "read wdt enable reg val error.\n");
        goto fail;
    }

    val &= priv->enable_mask;
    if (val == priv->enable_val) {
        status = WDT_ON;
    } else if(val == priv->disable_val) {
        status = WDT_OFF;
    } else {
        DEBUG_ERROR("enable reg read val not match set val, read val:%u, mask:%u, enable_val:%u, disable_val:%u",
            val, priv->enable_mask, priv->enable_val, priv->disable_val);
        ret = -EIO;
        goto fail;
    }

    DEBUG_VERBOSE("read_val:%u, mask:%u, enable_val:%u, disable_val:%u, status:%u",
        val, priv->enable_mask, priv->enable_val, priv->disable_val, status);

    mutex_unlock(&priv->update_lock);
    return sprintf(buf, "%u\n", status);

fail:
    mutex_unlock(&priv->update_lock);
    return ret;
}

static ssize_t set_wdt_ready_sysfs_value(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    wb_wdt_priv_t *priv;
    int err;
    uint32_t val;

    priv = dev_get_drvdata(dev);
    val = 0;
    err = kstrtouint(buf, 0, &val);
    if (err) {
        dev_err(priv->dev, "invalid value: %s, can't set wdt ready status\n", buf);
        return err;
    }

    DEBUG_VERBOSE("set wdt ready status value: %u\n", val);

    priv->wdt_ready = val;

    return count;
}

static ssize_t show_wdt_ready_sysfs_value(struct device *dev, struct device_attribute *da,
                   char *buf)
{
    wb_wdt_priv_t *priv;

    priv = dev_get_drvdata(dev);
    return sprintf(buf, "%u\n", priv->wdt_ready);
}

static ssize_t wb_show_no_feed_dog_flag(struct device *dev, struct device_attribute *attr, char *buf)
{
    wb_wdt_priv_t *priv = dev_get_drvdata(dev);
    return snprintf(buf, PAGE_SIZE, "%d\n", priv->no_feed_dog_flag);
}

static ssize_t wb_set_no_feed_dog_flag(struct device *dev, struct device_attribute *attr,
                                             const char *buf, size_t count)
{
    long input_data;
    wb_wdt_priv_t *priv = dev_get_drvdata(dev);
    int ret;

    if (count == 0) {
        DEBUG_ERROR("input is null\n");
        return -EINVAL;
    }

    DEBUG_VERBOSE("current no_feed_dog_flag: 0x%x\n", priv->no_feed_dog_flag);
    input_data = 0;
    ret = kstrtol(buf, 0, &input_data);
    if (ret) {
        dev_warn(dev, "input value fail.\n");
        return -EINVAL;
    }

    if (input_data == 0) {
        priv->no_feed_dog_flag = false;
        dev_warn(dev, "set to normal wdt mode.\n");
    } else {
        priv->no_feed_dog_flag = true;
        dev_warn(dev, "set to not feed wdt mode.\n");
    }

    DEBUG_VERBOSE("setted no_feed_dog_flag: 0x%x\n", priv->no_feed_dog_flag);

    return count;
}

static ssize_t wb_show_wdt_timeout_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    wb_wdt_priv_t *priv = dev_get_drvdata(dev);
    return snprintf(buf, PAGE_SIZE, "%ds\n", priv->set_timeout_value / MS_TO_S);

}

static ssize_t wb_set_wdt_timeout_value(struct device *dev, struct device_attribute *attr,
                                             const char *buf, size_t count)
{
    long input_data;
    int ret;
    wb_wdt_priv_t *priv = dev_get_drvdata(dev);

    if (count == 0) {
        DEBUG_ERROR("input is null\n");
        return -EINVAL;
    }

    DEBUG_VERBOSE("old set timeout vlaue: %ds\n", priv->set_timeout_value / MS_TO_S);

    input_data = 0;
    ret = kstrtol(buf, 0, &input_data);
    if (ret) {
        dev_warn(dev, "input value fail.\n");
        return -EINVAL;
    }

    if (input_data > 0) {
        if ((input_data * MS_TO_S) != priv->set_timeout_value) {
            priv->set_timeout_value = (u32)input_data * MS_TO_S;
            priv->set_new_timeout_flag = true;
            dev_warn(dev, "set new wdt timeout vlaue:%ds.\n", priv->set_timeout_value / MS_TO_S);
        } else {
            dev_warn(dev, "Set the new WDT timeout value to be the same as the old one:%ds.\n", priv->set_timeout_value / MS_TO_S);
        }
    } else {
        dev_warn(dev, "The input value is less than or equal to 0.(%ds)\n", priv->set_timeout_value / MS_TO_S);
    }

    DEBUG_VERBOSE("setted set timeout vlaue: %ds\n", priv->set_timeout_value / MS_TO_S);

    return count;
}

static DEVICE_ATTR(no_feed_wdt, S_IWUSR | S_IRUSR | S_IRGRP, \
                      wb_show_no_feed_dog_flag, wb_set_no_feed_dog_flag);
static DEVICE_ATTR(wdt_timeout_value, S_IWUSR | S_IRUSR | S_IRGRP, \
                      wb_show_wdt_timeout_value, wb_set_wdt_timeout_value);
static SENSOR_DEVICE_ATTR(wdt_status, S_IRUGO | S_IWUSR, show_wdt_sysfs_value, set_wdt_sysfs_value, 0);
static SENSOR_DEVICE_ATTR(wdt_ready, S_IRUGO | S_IWUSR, show_wdt_ready_sysfs_value, set_wdt_ready_sysfs_value, 0);

static struct attribute *wdt_sysfs_attrs[] = {
    &sensor_dev_attr_wdt_status.dev_attr.attr,
    &sensor_dev_attr_wdt_ready.dev_attr.attr,
    &dev_attr_no_feed_wdt.attr,
    &dev_attr_wdt_timeout_value.attr,
    NULL
};

static const struct attribute_group wdt_sysfs_group = {
    .attrs = wdt_sysfs_attrs,
};

static int wb_wdt_probe(struct platform_device *pdev)
{
    wb_wdt_priv_t *priv;
    int ret;
    const char *algo;
    wb_wdt_device_t *wb_wdt_device;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, priv);
    wb_wdt_device = NULL;
    algo = "";
    if (pdev->dev.of_node) {
        ret = 0;
        ret += of_property_read_string(pdev->dev.of_node, "config_dev_name", &priv->config_dev_name);
        ret += of_property_read_string(pdev->dev.of_node, "hw_algo", &algo);
        ret += of_property_read_u8(pdev->dev.of_node, "config_mode", &priv->config_mode);
        ret += of_property_read_u8(pdev->dev.of_node, "priv_func_mode", &priv->priv_func_mode);
        ret += of_property_read_u32(pdev->dev.of_node,"hw_margin_ms", &priv->hw_margin);
        ret += of_property_read_u8(pdev->dev.of_node,"feed_wdt_type", &priv->feed_wdt_type);
        ret += of_property_read_u32(pdev->dev.of_node,"timer_accuracy", &priv->timer_accuracy);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to priv dts.\n");
            return -ENXIO;
        }

        /* timer accuracy register is optional */
        ret = of_property_read_u8(pdev->dev.of_node,"timer_accuracy_reg_flag", &priv->timer_accuracy_reg_flag);
        if (ret < 0) {
            /* case: don't has timer_accuracy_reg */
            dev_dbg(&pdev->dev, "Failed to get property in dts: timer_accuracy_reg_flag.\n");
            priv->timer_accuracy_reg_flag = 0;
        } else {
            ret = of_property_read_u32(pdev->dev.of_node, "timer_accuracy_reg", &priv->timer_accuracy_reg);
            if (ret < 0) {
                dev_err(&pdev->dev, "Failed to get timer accuracy register address.\n");
                return -ENXIO;
            }
            ret = of_property_read_u8(pdev->dev.of_node, "timer_accuracy_reg_val", &priv->timer_accuracy_reg_val);
            if (ret < 0) {
                dev_err(&pdev->dev, "Failed to get timer accuracy register value.\n");
                return -ENXIO;
            }
        }

        /* timer update register is optional */
        ret = of_property_read_u8(pdev->dev.of_node,"timer_update_reg_flag", &priv->timer_update_reg_flag);
        if (ret < 0) {
            /* case: don't has timer_update_reg */
            dev_dbg(&pdev->dev, "Failed to get property in dts: timer_update_reg_flag.\n");
            priv->timer_update_reg_flag = 0; /* no timer update register */
        } else {
            ret = of_property_read_u32(pdev->dev.of_node, "timer_update_reg", &priv->timer_update_reg);
            if (ret < 0) {
                dev_err(&pdev->dev, "Failed to get timer update register address.\n");
                return -ENXIO;
            }
            ret = of_property_read_u8(pdev->dev.of_node, "timer_update_reg_val", &priv->timer_update_reg_val);
            if (ret < 0) {
                dev_err(&pdev->dev, "Failed to get timer update register value.\n");
                return -ENXIO;
            }
        }
        ret = of_property_read_u32(pdev->dev.of_node, "enable_reg", &priv->enable_reg);
        if (ret < 0) {
            /* case: Can't not enable/disable WDT */
            dev_dbg(&pdev->dev, "Failed to get property in dts: enable_reg.\n");
            priv->enable_reg = INVALID_REG_ADDR; /* no enable register */
        } else {
            ret = 0;
            ret += of_property_read_u8(pdev->dev.of_node, "enable_val", &priv->enable_val);
            ret += of_property_read_u8(pdev->dev.of_node, "disable_val", &priv->disable_val);
            ret += of_property_read_u8(pdev->dev.of_node, "enable_mask", &priv->enable_mask);
            if (ret != 0) {
                dev_err(&pdev->dev, "Failed to priv dts.\n");
                return -ENXIO;
            }
        }

        ret = of_property_read_u32(pdev->dev.of_node, "timeout_cfg_reg", &priv->timeout_cfg_reg);
        if (ret < 0) {
            /* case: Can't not set timeout cfg reg */
            dev_dbg(&pdev->dev, "Failed to get property in dts: timeout_cfg_reg.\n");
            priv->timeout_cfg_reg = INVALID_REG_ADDR; /* no timeout cfg register */
        }
        
        priv->timeleft_cfg_reg = priv->timeout_cfg_reg;
        of_property_read_u32(pdev->dev.of_node,"timeleft_cfg_reg", &priv->timeleft_cfg_reg);

        /* wdt_ready is configurable. If the value is not configured, the value is 1 by default */
        ret = of_property_read_u32(pdev->dev.of_node,"wdt_ready", &priv->wdt_ready);
        if (ret != 0) {
            priv->wdt_ready = WDT_IS_READY;
        }

        if (!strcmp(algo, "toggle")) {
            priv->hw_algo = HW_ALGO_TOGGLE;
        } else if (!strcmp(algo, "level")) {
            priv->hw_algo = HW_ALGO_LEVEL;
        } else if (!strcmp(algo, "eigenvalues")) {
            priv->hw_algo = HW_ALGO_EIGENVALUES;
        } else {
            dev_err(&pdev->dev, "hw_algo config error.must be toggle or level.\n");
            return -EINVAL;
        }

        dev_info(&pdev->dev, "config_dev_name:%s, config_mode:%u, priv_func_mode:%u, enable_reg:0x%x, timeout_cfg_reg:0x%x\n",
            priv->config_dev_name, priv->config_mode, priv->priv_func_mode, priv->enable_reg, priv->timeout_cfg_reg);
        dev_info(&pdev->dev, "enable_val:%u, disable_val:%u, enable_mask:%u, hw_margin:%u, feed_wdt_type:%u\n",
            priv->enable_val, priv->disable_val, priv->enable_mask,   priv->hw_margin, priv->feed_wdt_type);
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        wb_wdt_device = pdev->dev.platform_data;
        priv->config_dev_name = wb_wdt_device->config_dev_name;
        algo = wb_wdt_device->hw_algo;
        priv->config_mode = wb_wdt_device->config_mode;
        priv->priv_func_mode = wb_wdt_device->priv_func_mode;
        priv->enable_val = wb_wdt_device->enable_val;
        priv->disable_val = wb_wdt_device->disable_val;
        priv->enable_mask = wb_wdt_device->enable_mask;
        priv->enable_reg = wb_wdt_device->enable_reg;
        priv->timeout_cfg_reg = wb_wdt_device->timeout_cfg_reg;
        priv->hw_margin = wb_wdt_device->hw_margin;
        priv->timer_accuracy = wb_wdt_device->timer_accuracy;
        priv->feed_wdt_type = wb_wdt_device->feed_wdt_type;
        priv->timeleft_cfg_reg = wb_wdt_device->timeleft_cfg_reg;
        priv->timer_accuracy_reg_flag = wb_wdt_device->timer_accuracy_reg_flag;
        priv->timer_accuracy_reg = wb_wdt_device->timer_accuracy_reg;
        priv->timer_accuracy_reg_val = wb_wdt_device->timer_accuracy_reg_val;
        priv->timer_update_reg_flag = wb_wdt_device->timer_update_reg_flag;
        priv->timer_update_reg = wb_wdt_device->timer_update_reg;
        priv->timer_update_reg_val = wb_wdt_device->timer_update_reg_val;
        if (wb_wdt_device->wdt_ready == WDT_READY_NO_CFG) {
            wb_wdt_device->wdt_ready = WDT_IS_READY;
        }        
        priv->wdt_ready = wb_wdt_device->wdt_ready;
    }
    /* debug data set default */
    priv->set_timeout_value = priv->hw_margin;
    priv->set_new_timeout_flag = false;
    priv->no_feed_dog_flag = false;

    if (!strcmp(algo, "toggle")) {
        priv->hw_algo = HW_ALGO_TOGGLE;
    } else if (!strcmp(algo, "level")) {
        priv->hw_algo = HW_ALGO_LEVEL;
    } else if (!strcmp(algo, "eigenvalues")) {
        priv->hw_algo = HW_ALGO_EIGENVALUES;
    } else {
        dev_err(&pdev->dev, "hw_algo config error.must be toggle or level.\n");
        return -EINVAL;
    }

    ret = find_intf_addr(&priv->write_intf_addr, &priv->read_intf_addr, priv->priv_func_mode);
    if (ret) {
        dev_err(&pdev->dev, "find_intf_addr func mode %d fail, ret: %d.\n", priv->priv_func_mode, ret);
        return ret;
    }

    if (!priv->write_intf_addr || !priv->read_intf_addr) {
        dev_err(&pdev->dev, "Fail: func mode %u rw symbol undefined.\n", priv->priv_func_mode);
        return -ENOSYS;
    }

    DEBUG_VERBOSE("config_dev_name:%s, config_mode:%u, priv_func_mode:%u, enable_reg:0x%x, timeout_cfg_reg:0x%x\n",
        priv->config_dev_name, priv->config_mode, priv->priv_func_mode, priv->enable_reg, priv->timeout_cfg_reg);
    DEBUG_VERBOSE("timeout_cfg_reg:0x%x, enable_val:0x%x, disable_val:0x%x, enable_mask:0x%x, hw_margin:%u, feed_wdt_type:%u\n",
        priv->timeleft_cfg_reg, priv->enable_val, priv->disable_val, priv->enable_mask, priv->hw_margin, priv->feed_wdt_type);
    DEBUG_VERBOSE("timer_accuracy_reg_flag: %d, timer_accuracy_reg: 0x%x, timer_accuracy_reg_val: 0x%x, timer_accuracy: %d\n",
        priv->timer_accuracy_reg_flag, priv->timer_accuracy_reg, priv->timer_accuracy_reg_val, priv->timer_accuracy);
    DEBUG_VERBOSE("timer_update_reg_flag: %d, timer_update_reg: 0x%x, timer_update_reg_val: 0x%x\n", priv->timer_update_reg_flag,
        priv->timer_update_reg, priv->timer_update_reg_val);
    DEBUG_VERBOSE("wdt_ready: %d\n", priv->wdt_ready);

    priv->dev = &pdev->dev;
    if (priv->config_mode == GPIO_FEED_WDT_MODE) {
        ret = gpio_wdt_init(priv, wb_wdt_device);
        if (ret < 0) {
            dev_err(&pdev->dev, "init gpio mode wdt failed.\n");
            return -ENXIO;
        }
    } else if (priv->config_mode == LOGIC_FEED_WDT_MODE) {
        ret = logic_wdt_init(priv, wb_wdt_device);
        if (ret < 0) {
            dev_err(&pdev->dev, "init func mode wdt failed.\n");
            return -ENXIO;
        }
    } else {
        dev_err(&pdev->dev, "unsupport %u config_mode, dts configure error.\n",
            priv->config_mode);
        return -ENXIO;
    }

    wdt_hwping(priv);
    DEBUG_VERBOSE("feed wdt init\n");

    switch (priv->feed_wdt_type) {
    case WATCHDOG_DEVICE_TYPE:
        ret = watchdog_device_cfg(priv);
        break;
    case HRTIMER_TYPE:
        ret = hrtimer_cfg(priv, wb_wdt_device);
        break;
    case THREAD_TYPE:
        ret = thread_timer_create(priv, wb_wdt_device);
        break;
    default:
        dev_err(&pdev->dev, "timer type %u unsupport.\n", priv->feed_wdt_type);
        return -EINVAL;
    }
    if (ret < 0) {
        dev_err(&pdev->dev, "init timer feed_wdt_type %u failed.\n", priv->feed_wdt_type);
        return -ENXIO;
    }

    dev_info(&pdev->dev, "register %s mode, config_mode %u, func_mode %u, %u ms overtime wdt success\n",
        algo, priv->config_mode, priv->priv_func_mode, priv->hw_margin);

    /* Feed the dog once after enabling the watchdog */
    wdt_hwping(priv);

    ret = wb_wdt_register_device(priv);
    if (ret < 0) {
        dev_err(&pdev->dev, "kernel watchdog sysfs register %u failed.\n", ret);
        return -ENODEV;
    }

    mutex_init(&priv->update_lock);

    priv->sysfs_group = &wdt_sysfs_group;
    ret = sysfs_create_group(&pdev->dev.kobj, priv->sysfs_group);
    if (ret != 0) {
        dev_err(&pdev->dev, "sysfs_create_group failed. ret:%d.\n", ret);
        return -ENOMEM;
    }

    dev_info(&pdev->dev, "register wdt sysfs success.");

    return 0;
}

static void unregister_action(struct platform_device *pdev)
{
    wb_wdt_priv_t *priv = platform_get_drvdata(pdev);
    int ret;

    /* Feed the dog one last time before logging off */
    wdt_hwping(priv);

    ret = wb_wdt_enable_ctrl(priv, WDT_OFF);
    if (ret < 0) {
        dev_err(&pdev->dev, "remove disable wdt failed.\n");
    }

    sysfs_remove_group(&pdev->dev.kobj, priv->sysfs_group);

    if (priv->feed_wdt_type == HRTIMER_TYPE) {
        hrtimer_cancel(&priv->hrtimer);
    } else if (priv->feed_wdt_type == THREAD_TYPE) {
        kthread_stop(priv->thread);
        priv->thread = NULL;
    } else {
        DEBUG_VERBOSE("wdd type, do nothing.\n");
    }
#if 0
    if (priv->config_mode == GPIO_FEED_WDT_MODE) {
        gpio_wdt = &priv->gpio_wdt;
        gpio_set_value_cansleep(gpio_wdt->gpio, !gpio_wdt->active_low);

        if (priv->hw_algo == HW_ALGO_TOGGLE) {
            gpio_direction_input(gpio_wdt->gpio);
        }
    } else {
        logic_wdt = &priv->logic_wdt;
        logic_wdt->state_val = !logic_wdt->state_val;
        ret = wb_wdt_write(priv, logic_wdt->feed_dev_name,
                    logic_wdt->feed_reg, SECOND_LOGIC_WDT, &logic_wdt->state_val, ONE_BYTE);
        if (ret < 0) {
            dev_err(&pdev->dev, "set wdt control reg error.\n");
        }
    }
#endif

    return;
}

static int wb_wdt_remove(struct platform_device *pdev)
{
    DEBUG_VERBOSE("enter remove wdt.\n");
    unregister_action(pdev);
    dev_info(&pdev->dev, "remove wdt finish.\n");

    return 0;
}

static void wb_wdt_shutdown(struct platform_device *pdev)
{
    DEBUG_VERBOSE("enter shutdown wdt.\n");
    unregister_action(pdev);
    dev_info(&pdev->dev, "shutdown wdt finish.\n");

    return;
}

static const struct of_device_id wb_wdt_dt_ids[] = {
    { .compatible = "wb_wdt", },
    { }
};
MODULE_DEVICE_TABLE(of, wb_wdt_dt_ids);

static struct platform_driver wb_wdt_driver = {
    .driver = {
        .name       = "wb_wdt",
        .of_match_table = wb_wdt_dt_ids,
    },
    .probe  = wb_wdt_probe,
    .remove = wb_wdt_remove,
    .shutdown = wb_wdt_shutdown,
};

#ifdef CONFIG_GPIO_WATCHDOG_ARCH_INITCALL
static int __init wb_wdt_init(void)
{
    return platform_driver_register(&wb_wdt_driver);
}
arch_initcall(wb_wdt_init);
#else
module_platform_driver(wb_wdt_driver);
#endif

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("watchdog driver");
MODULE_LICENSE("GPL");
