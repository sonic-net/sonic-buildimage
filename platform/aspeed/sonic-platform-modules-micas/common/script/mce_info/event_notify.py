#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

import os
import time
import select
import threading
import logging
import sys
sys.path.append("/usr/local/bin/")


from platform_config import get_config_param
from event_common import EVENT_TRIG_BY_GPIO, EVENT_TRIG_BY_POLL
#from msg_queue_client import put_event
from platform_util import check_value, set_value
from dbus_event import event_notify_by_dbus


# Default log shutdown
log_level_str = os.getenv('EVENT_NOTIFY_LOG_LEVEL', 'NOEXIST').upper()
log_level = getattr(logging, log_level_str, logging.CRITICAL + 1)

logging.basicConfig(level=log_level, format='%(asctime)s [%(levelname)s] %(filename)s:%(lineno)d %(message)s')
logger = logging.getLogger(__name__)
#logger.setLevel(logging.INFO)

GPIO_CFG_PARAS = get_config_param("GPIO_CFG_PARAS", {})
POLL_CFG_PARAS = get_config_param("POLL_CFG_PARAS", {})
INIT_CFG_PARAS = get_config_param("INIT_CFG_PARAS", [])
DEV_EVENT_TRIG_WAY = get_config_param("DEV_EVENT_TRIG_WAY", 0)


def event_notify_by_dbus_tool(event):
    if isinstance(event, int):
        temp_str = "int32:" + str(event)
    elif isinstance(event, str):
        temp_str = 'string:"' + event + '"'
    else:
        logger.error("unsupported event type")
        return -1
    logger.info(f"temp_str: {temp_str}")
    
    service_name = "org.bsp.dbus"
    object_path = "/event/notify"
    interface_name = "event.notify.mce_signal"
    event_notify_cmd = f"dbus-send --system --type=signal --dest={service_name} {object_path} {interface_name} {temp_str}"
    logger.info(f"event_notify_cmd: {event_notify_cmd}")

    # signal test cmd please refer to: dbus-monitor --system "type='signal',interface=event.notify,member='mce_signal'" &
    event_notify_dic = {"cmd": event_notify_cmd, "gettype": "cmd"}
    ret,log = set_value(event_notify_dic)
    if ret != True:
        logger.error(f"dbus-send cmd exec fail: {log}")
        return -2
    return 0


class EventNotify():
    def __init__(self):
        self.gpio_cfg_paras = GPIO_CFG_PARAS
        self.poll_cfg_paras = POLL_CFG_PARAS
        self.init_cfg_paras = INIT_CFG_PARAS
        rv = self.do_operate_according_to_cfg(self.init_cfg_paras)
        if rv != 0:
            logger.error("do operate fail according to init_cfg_paras")
        logger.info("event notify instance create ok")

    def do_operate_according_to_cfg(self, cfg):
        if not cfg:
            logger.info("cfg none, do nothing")
            return 0

        rv = 0
        for item in cfg:
            ret, msg = set_value(item)
            if ret == False:
                logger.error(f"set value fail, msg:{msg}")
                rv = -1
        return rv

    def gpio_init(self):
        if not self.gpio_cfg_paras:
            logger.warning("No GPIO configurations found.")
            return -1

        try:
            for gpio_cfg in self.gpio_cfg_paras.values():
                # 1. get gpio_num
                gpio_num = str(gpio_cfg["gpio_num"])
                edge = gpio_cfg["edge"]
                logger.info(f"gpio_num: {gpio_num}, edge: {edge}")

                # 2. export gpio
                export_path = "/sys/class/gpio/export"
                if not os.path.exists(f"/sys/class/gpio/gpio{gpio_num}"):
                    with open(export_path, 'w') as f:
                        f.write(gpio_num)

                # 3. set gpio direction
                direction_path = f"/sys/class/gpio/gpio{gpio_num}/direction"
                with open(direction_path, 'w') as f:
                    f.write('in')

                # 4. set gpio int edge
                edge_path = f"/sys/class/gpio/gpio{gpio_num}/edge"
                with open(edge_path, 'w') as f:
                    f.write(edge)

        except Exception as e:
            logger.error(f"GPIO init exception: {e}")
            return -2

        logger.info("GPIOs init successfully.")
        return 0


    def do_event_notify_according_to_gpio_cfg(self):
        poller = select.poll()
        fd_to_gpio_cfg = {}
        try:
            # 1. Open all the gpio value files and register a poll listener to monitor the POLLPRI event.
            for gpio_cfg in self.gpio_cfg_paras.values():
                path = f"/sys/class/gpio/gpio{gpio_cfg['gpio_num']}/value"
                fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
                poller.register(fd, select.POLLPRI)
                fd_to_gpio_cfg[fd] = gpio_cfg

                # Read once and clear any possible interrupt status
                os.lseek(fd, 0, os.SEEK_SET)
                os.read(fd, 1)
            logger.info(f"Registered GPIO fds: {list(fd_to_gpio_cfg.keys())}")

            while True:
                # 2. Blocking wait for interrupt event
                #The default value of timeout is None, indicating a blocking access and an infinite wait. The unit of timeout is milliseconds. It returns a list of tuples [(fd, event), ...]
                events = poller.poll()
                for fd, event in events:
                    if event & select.POLLPRI == 0:
                        logger.info(f"do not care event: {event}, skip")
                        continue

                    # get gpio_cfg
                    gpio_cfg = fd_to_gpio_cfg.get(fd)
                    if gpio_cfg is None:
                        logger.warning(f"Unknown fd {fd} event")
                        continue

                    # 3. Read the GPIO value and clear the interrupt trigger status
                    os.lseek(fd, 0, os.SEEK_SET)
                    gpio_value = os.read(fd, 1).decode().strip()

                    check_event_status = gpio_cfg.get("check_status", None)
                    rv = 0
                    if check_event_status is None:
                        rv = event_notify_by_dbus(gpio_cfg.get("events_info"))
                    else:
                        ret, log = check_value(check_event_status)
                        logger.info(f"int check value ret:{ret}, log: {log}")
                        if ret == True:
                            rv = event_notify_by_dbus(gpio_cfg.get("events_info"))

                    if rv:
                        logger.error(f"int notify event by dbus fail, rv: {rv}")

                    clear_event_status = gpio_cfg.get("clear_status", None)
                    if clear_event_status:
                        rv = self.do_operate_according_to_cfg(clear_event_status)
                        if rv != 0:
                            logger.error(f"int do operate fail according to clear_event_status {clear_event_status}")
                            continue

                    logger.info(f"int notify event ok")

        except Exception as e:
            logger.error(f"Exception in GPIO event notify: {e}")
            return -3
        finally:
            for fd in fd_to_gpio_cfg:
                try:
                    poller.unregister(fd)
                    os.close(fd)
                except Exception:
                    pass

    def do_event_notify_according_to_poll_cfg(self):
        if not self.poll_cfg_paras:
            logger.warning("No poll cfg paras found.")
            return -1

        poll_time = self.poll_cfg_paras.get("poll_time", 1)
        poll_cfgs = self.poll_cfg_paras.get("poll_cfgs")
        if poll_cfgs is None:
            logger.warning("No poll_cfgs found in poll configuration.")
            return -2
        logger.info(f"poll_time: {poll_time}")
        logger.info(f"poll_cfgs: {poll_cfgs}")

        try:
            while True:
                for poll_cfg in poll_cfgs.values():
                    check_event_status = poll_cfg.get("check_status", None)
                    if check_event_status is None:
                        logger.error(f"poll check_event_status is none")
                        continue

                    ret, log = check_value(check_event_status)
                    if ret == False:
                        logger.info(f"poll check_event_status not ok, log: {log}")
                        continue
                    logger.info("poll check status event ok")

                    rv = event_notify_by_dbus(poll_cfg.get("events_info"))
                    if rv:
                        logger.error(f"poll notify event by dbus fail, rv: {rv}")

                    clear_event_status = poll_cfg.get("clear_status", None)
                    if clear_event_status:
                        rv = self.do_operate_according_to_cfg(clear_event_status)
                        if rv != 0:
                            logger.error(f"poll do operate fail according to clear_event_status {clear_event_status}")
                            continue

                    logger.info("poll notify event ok")

                time.sleep(poll_time)

        except Exception as e:
            logger.error(f"Exception in poll event notify: {e}")
            return -3


def start():
    try:
        event_notify = EventNotify()
        threads = []
        if DEV_EVENT_TRIG_WAY & EVENT_TRIG_BY_GPIO != 0:
            logger.info("event notify by gpio")
            ret = event_notify.gpio_init()
            if ret:
                logger.error(f"GPIO init failed, ret={ret}")
                sys.exit(1)

            thread1 = threading.Thread(target=event_notify.do_event_notify_according_to_gpio_cfg, daemon=True)
            threads.append(thread1)

        if DEV_EVENT_TRIG_WAY & EVENT_TRIG_BY_POLL != 0:
            logger.info("event notify by poll")
            thread2 = threading.Thread(target=event_notify.do_event_notify_according_to_poll_cfg, daemon=True)
            threads.append(thread2)

        for thread in threads:
            thread.start()

        # The main thread continues to run to prevent termination.
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            logger.info("Received KeyboardInterrupt, exiting...")

    except Exception as e:
        logger.error(f"Exception in start(): {e}")


if __name__ == '__main__':
    start()

