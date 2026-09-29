#!/usr/bin/env python3
"""
Module: ledswitch_monitor.py
Description: Monitor Ethernet interface breakout modes and netdev_oper_status, execute corresponding sap commands
             Ethernet1024 is handled specially: directly monitor its netdev_oper_status
Usage: python ledswitch_monitor.py {start|stop}
"""

import redis
import threading
import time
import subprocess
import signal
import sys
import logging
import os
from typing import Dict, List, Tuple, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/var/log/ledswitch_monitor.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('ledswitch_monitor')

# Global control variables
monitor_thread = None
stop_event = threading.Event()

LED_SWITCH_CONTROL = 0x0
LED_RED_BLINK     = 0x1
LED_RED_ON        = 0x2
LED_GREEN_BLINK   = 0x3
LED_GREEN_ON      = 0x4
LED_YELLOW_BLINK  = 0x5
LED_YELLOW_ON     = 0x6
LED_OFF           = 0x7

# Special port that needs individual handling
SPECIAL_PORT = "Ethernet1024"

PORT_TO_REG = {
    0: "0x90",    8: "0x91",   16: "0x92",   24: "0x93",   32: "0x94",   40: "0x95",   48: "0x96",   56: "0x97",
    64: "0x98",   72: "0x99",   80: "0x9A",   88: "0x9B",   96: "0x9C",  104: "0x9D",  112: "0x9E",  120: "0x9F",
    128: "0xA0",  136: "0xA1",  144: "0xA2",  152: "0xA3",  160: "0xA4",  168: "0xA5",  176: "0xA6",  184: "0xA7",
    192: "0xA8",  200: "0xA9",  208: "0xAA",  216: "0xAB",  224: "0xAC",  232: "0xAD",  240: "0xAE",  248: "0xAF",
    256: "0xB0",  264: "0xB1",  272: "0xB2",  280: "0xB3",  288: "0xB4",  296: "0xB5",  304: "0xB6",  312: "0xB7",
    320: "0xB8",  328: "0xB9",  336: "0xBA",  344: "0xBB",  352: "0xBC",  360: "0xBD",  368: "0xBE",  376: "0xBF",
    384: "0xC0",  392: "0xC1",  400: "0xC2",  408: "0xC3",  416: "0xC4",  424: "0xC5",  432: "0xC6",  440: "0xC7",
    448: "0xC8",  456: "0xC9",  464: "0xCA",  472: "0xCB",  480: "0xCC",  488: "0xCD",  496: "0xCE",  504: "0xCF",
    512: "0xB0",  520: "0xB1",  528: "0xB2",  536: "0xB3",  544: "0xB4",  552: "0xB5",  560: "0xB6",  568: "0xB7",
    576: "0xB8",  584: "0xB9",  592: "0xBA",  600: "0xBB",  608: "0xBC",  616: "0xBD",  624: "0xBE",  632: "0xBF",
    640: "0xC0",  648: "0xC1",  656: "0xC2",  664: "0xC3",  672: "0xC4",  680: "0xC5",  688: "0xC6",  696: "0xC7",
    704: "0xC8",  712: "0xC9",  720: "0xCA",  728: "0xCB",  736: "0xCC",  744: "0xCD",  752: "0xCE",  760: "0xCF",
    768: "0x90",  776: "0x91",  784: "0x92",  792: "0x93",  800: "0x94",  808: "0x95",  816: "0x96",  824: "0x97",
    832: "0x98",  840: "0x99",  848: "0x9A",  856: "0x9B",  864: "0x9C",  872: "0x9D",  880: "0x9E",  888: "0x9F",
    896: "0xA0",  904: "0xA1",  912: "0xA2",  920: "0xA3",  928: "0xA4",  936: "0xA5",  944: "0xA6",  952: "0xA7",
    960: "0xA8",  968: "0xA9",  976: "0xAA",  984: "0xAB",  992: "0xAC", 1000: "0xAD", 1008: "0xAE", 1016: "0xAF"
}

class LedSwitchMonitor:
    def __init__(self):
        """Initialize the monitor"""
        self.redis_db4 = None
        self.redis_db6 = None
        self.last_status = {}  # Track last status of each port to avoid duplicate command execution
        self.last_special_port_status = None  # Track last status of special port
        self.initialized = False
        
    def connect_redis(self):
        """Connect to Redis databases"""
        try:
            self.redis_db4 = redis.Redis(host='127.0.0.1', port=6379, db=4, decode_responses=True)
            self.redis_db6 = redis.Redis(host='127.0.0.1', port=6379, db=6, decode_responses=True)
            # Test connection
            self.redis_db4.ping()
            self.redis_db6.ping()
            logger.info("Redis connection successful")
            return True
        except Exception as e:
            logger.error(f"Redis connection failed: {e}")
            return False
    
    def get_brkout_mode(self, port_name: str) -> Optional[str]:
        """Get breakout mode of the port from database 4"""
        try:
            key = f"BREAKOUT_CFG|{port_name}"
            data = self.redis_db4.hgetall(key)
            return data.get('brkout_mode')
        except Exception as e:
            logger.error(f"Failed to get breakout mode for {port_name}: {e}")
            return None
    
    def get_netdev_oper_status(self, port_name: str) -> Optional[str]:
        """Get netdev_oper_status of the port from database 6"""
        try:
            key = f"PORT_TABLE|{port_name}"
            data = self.redis_db6.hgetall(key)
            return data.get('netdev_oper_status')
        except Exception as e:
            logger.error(f"Failed to get netdev_oper_status for {port_name}: {e}")
            return None
    
    def get_all_ethernet_ports(self) -> List[str]:
        """Get all Ethernet ports from database 4"""
        ports = []
        try:
            keys = self.redis_db4.keys("BREAKOUT_CFG|Ethernet*")
            for key in keys:
                port = key.replace("BREAKOUT_CFG|", "")
                ports.append(port)
        except Exception as e:
            logger.error(f"Failed to get Ethernet port list: {e}")
        return sorted(ports)

    def get_reg(self, port: int):
        if port not in PORT_TO_REG:
            logger.info(f"Invalid port {port}")
            return None
        logger.info(f"get_reg port:{port}, reg:{PORT_TO_REG[port]}")
        return PORT_TO_REG[port]

    def execute_sap_command(self, port_name: str, led_status: int):
        """Execute sap command"""
        port_num = int(port_name.replace('Ethernet', ''))
        port_reg = self.get_reg(port_num)
        if port_reg is None:
            logger.info(f"port_reg is None port:{port_name}")
            return
        if port_num < 512:
            cmd = f"dfd_debug sysfs_data_wr /dev/cpld15 {port_reg} {led_status}"
        else:
            cmd = f"dfd_debug sysfs_data_wr /dev/cpld16 {port_reg} {led_status}"
        try:
            result = subprocess.run(
                cmd.split(),
                shell=False,  # Explicitly set to False for security
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                logger.info(f"Command executed successfully: {' '.join(cmd)}")
            else:
                logger.error(f"Command execution failed: {' '.join(cmd)}, error: {result.stderr}")
        except subprocess.TimeoutExpired:
            logger.error(f"Command execution timeout (5s): {' '.join(cmd)}")
        except Exception as e:
            logger.error(f"Command execution exception: {' '.join(cmd)}, error: {e}")

    def reset_all_port_reg(self):
        logger.info("LED switch monitoring will be stop, set all port reg to default")
        for port_num in PORT_TO_REG.keys():
            port_name = f"Ethernet{port_num}"
            self.execute_sap_command(port_name, LED_SWITCH_CONTROL)
    
    def handle_special_port(self):
        """Handle Ethernet1024 specially - directly monitor its netdev_oper_status"""
        netdev_status = self.get_netdev_oper_status(SPECIAL_PORT)
        if netdev_status is None:
            logger.warning(f"Failed to get netdev_oper_status for special port {SPECIAL_PORT}")
            return
        
        old_status = self.last_special_port_status
        
        if netdev_status == 'up':
            if old_status != 'up':
                logger.info(f"Special port {SPECIAL_PORT} status changed to up, executing sap LED_GREEN_ON")
                self.execute_sap_command(SPECIAL_PORT, LED_GREEN_ON)
                self.last_special_port_status = 'up'
        elif netdev_status == 'down':
            if old_status != 'down':
                logger.info(f"Special port {SPECIAL_PORT} status changed to down, executing sap LED_OFF")
                self.execute_sap_command(SPECIAL_PORT, LED_OFF)
                self.last_special_port_status = 'down'
    
    def handle_1x800g(self, port: str, current_statuses: Dict[str, str], status_key: str):
        """Handle 1x800G breakout mode - monitor single port"""
        # Skip special port as it's handled separately
        if port == SPECIAL_PORT:
            return
            
        netdev_status = self.get_netdev_oper_status(port)
        if netdev_status is None:
            return
        
        old_status = current_statuses.get(status_key)
        
        if netdev_status == 'up':
            if old_status != 'up':
                logger.info(f"1x800G port {port} status changed to up, executing sap LED_GREEN_ON")
                self.execute_sap_command(port, LED_GREEN_ON)
                current_statuses[status_key] = 'up'
        elif netdev_status == 'down':
            if old_status != 'down':
                logger.info(f"1x800G port {port} status changed to down, executing sap LED_OFF")
                self.execute_sap_command(port, LED_OFF)
                current_statuses[status_key] = 'down'
    
    def handle_2x400g(self, port: str, current_statuses: Dict[str, str], status_key: str):
        """Handle 2x400G breakout mode - monitor port and port+4"""
        # Skip if any port in the group is the special port
        port_num = int(port.replace('Ethernet', ''))
        port1 = port
        port2 = f"Ethernet{port_num + 4}"
        
        if port1 == SPECIAL_PORT or port2 == SPECIAL_PORT:
            # Special port is handled separately, skip group processing
            return
        
        status1 = self.get_netdev_oper_status(port1)
        status2 = self.get_netdev_oper_status(port2)
        
        if status1 is None or status2 is None:
            return
        
        all_up = (status1 == 'up' and status2 == 'up')
        all_down = (status1 == 'down' and status2 == 'down')
        
        old_status = current_statuses.get(status_key)
        
        if all_up:
            if old_status != 'up':
                logger.info(f"2x400G port group {port1} and {port2} both up, executing sap LED_GREEN_ON")
                self.execute_sap_command(port, LED_GREEN_ON)
                current_statuses[status_key] = 'up'
        elif all_down:
            if old_status != 'down':
                logger.info(f"2x400G port group {port1} and {port2} both down, executing sap LED_OFF")
                self.execute_sap_command(port, LED_OFF)
                current_statuses[status_key] = 'down'
        else:
            if old_status != 'mixed':
                logger.info(f"2x400G port group {port1}({status1}) and {port2}({status2}) mixed, executing sap LED_YELLOW_ON")
                self.execute_sap_command(port, LED_YELLOW_ON)
                current_statuses[status_key] = 'mixed'
    
    def handle_4x200g(self, port: str, current_statuses: Dict[str, str], status_key: str):
        """Handle 4x200G breakout mode - monitor port, port+2, port+4, port+6"""
        port_num = int(port.replace('Ethernet', ''))
        ports = [
            port,
            f"Ethernet{port_num + 2}",
            f"Ethernet{port_num + 4}",
            f"Ethernet{port_num + 6}"
        ]
        
        # Skip if any port in the group is the special port
        if SPECIAL_PORT in ports:
            return
        
        statuses = []
        for p in ports:
            status = self.get_netdev_oper_status(p)
            if status is None:
                return
            statuses.append(status)
        
        all_up = all(s == 'up' for s in statuses)
        all_down = all(s == 'down' for s in statuses)
        
        old_status = current_statuses.get(status_key)
        
        if all_up:
            if old_status != 'up':
                logger.info(f"4x200G port group {ports} all up, executing sap LED_GREEN_ON")
                self.execute_sap_command(port, LED_GREEN_ON)
                current_statuses[status_key] = 'up'
        elif all_down:
            if old_status != 'down':
                logger.info(f"4x200G port group {ports} all down, executing sap LED_OFF")
                self.execute_sap_command(port, LED_OFF)
                current_statuses[status_key] = 'down'
        else:
            if old_status != 'mixed':
                status_str = ','.join([f"{p}({s})" for p, s in zip(ports, statuses)])
                logger.info(f"4x200G port group {status_str} mixed, executing sap LED_YELLOW_ON")
                self.execute_sap_command(port, LED_YELLOW_ON)
                current_statuses[status_key] = 'mixed'
    
    def handle_8x100g(self, port: str, current_statuses: Dict[str, str], status_key: str):
        """Handle 8x100G breakout mode - monitor port, port+1, ..., port+7"""
        port_num = int(port.replace('Ethernet', ''))
        ports = [f"Ethernet{port_num + i}" for i in range(8)]
        
        # Skip if any port in the group is the special port
        if SPECIAL_PORT in ports:
            return
        
        statuses = []
        for p in ports:
            status = self.get_netdev_oper_status(p)
            if status is None:
                return
            statuses.append(status)
        
        all_up = all(s == 'up' for s in statuses)
        all_down = all(s == 'down' for s in statuses)
        
        old_status = current_statuses.get(status_key)
        
        if all_up:
            if old_status != 'up':
                logger.info(f"8x100G port group {ports[0]}...{ports[-1]} all up, executing sap LED_GREEN_ON")
                self.execute_sap_command(port, LED_GREEN_ON)
                current_statuses[status_key] = 'up'
        elif all_down:
            if old_status != 'down':
                logger.info(f"8x100G port group {ports[0]}...{ports[-1]} all down, executing sap LED_OFF")
                self.execute_sap_command(port, LED_OFF)
                current_statuses[status_key] = 'down'
        else:
            if old_status != 'mixed':
                logger.info(f"8x100G port group {ports[0]}...{ports[-1]} mixed, executing sap LED_YELLOW_ON")
                self.execute_sap_command(port, LED_YELLOW_ON)
                current_statuses[status_key] = 'mixed'
    
    def monitor_loop(self, stop_event: threading.Event):
        """Main monitoring loop"""
        logger.info("Monitor thread started")
        
        while not stop_event.is_set():
            try:
                # Reconnect to Redis if disconnected
                if not self.initialized or not self.redis_db4 or not self.redis_db6:
                    if not self.connect_redis():
                        time.sleep(5)
                        continue
                    self.initialized = True
                
                # Handle special port Ethernet1024 first
                self.handle_special_port()
                
                # Get all Ethernet ports
                ports = self.get_all_ethernet_ports()
                
                # Track processed ports to avoid duplicate processing
                processed_ports = set()
                
                for port in ports:
                    # Skip special port as it's handled separately
                    if port == SPECIAL_PORT:
                        continue
                    
                    if port in processed_ports:
                        continue
                    
                    brkout_mode = self.get_brkout_mode(port)
                    
                    if brkout_mode == '1x800G':
                        status_key = f"{port}_1x800G"
                        self.handle_1x800g(port, self.last_status, status_key)
                        processed_ports.add(port)
                        
                    elif brkout_mode == '2x400G':
                        status_key = f"{port}_2x400G"
                        self.handle_2x400g(port, self.last_status, status_key)
                        # Mark related ports to avoid duplicate processing
                        port_num = int(port.replace('Ethernet', ''))
                        processed_ports.add(port)
                        processed_ports.add(f"Ethernet{port_num + 4}")
                        
                    elif brkout_mode == '4x200G':
                        status_key = f"{port}_4x200G"
                        self.handle_4x200g(port, self.last_status, status_key)
                        port_num = int(port.replace('Ethernet', ''))
                        for i in range(0, 8, 2):
                            processed_ports.add(f"Ethernet{port_num + i}")
                            
                    elif brkout_mode == '8x100G':
                        status_key = f"{port}_8x100G"
                        self.handle_8x100g(port, self.last_status, status_key)
                        port_num = int(port.replace('Ethernet', ''))
                        for i in range(8):
                            processed_ports.add(f"Ethernet{port_num + i}")
                
            except Exception as e:
                logger.error(f"Monitor loop exception: {e}")
            
            # Wait 2 seconds before next iteration
            stop_event.wait(2)
        
        logger.info("Monitor thread stopped")
    
    def start(self):
        """Start monitoring"""
        global monitor_thread, stop_event
        
        if monitor_thread is not None and monitor_thread.is_alive():
            logger.warning("Monitor thread already running")
            return
        
        stop_event.clear()
        monitor_thread = threading.Thread(target=self.monitor_loop, args=(stop_event,), daemon=True)
        monitor_thread.start()
        logger.info("LED switch monitoring started")
    
    def stop(self):
        """Stop monitoring"""
        global monitor_thread
        self.reset_all_port_reg()
        if monitor_thread is None or not monitor_thread.is_alive():
            logger.warning("Monitor thread not running")
            return
        
        stop_event.set()
        monitor_thread.join(timeout=5)
        monitor_thread = None
        logger.info("LED switch monitoring stopped")


# Global monitor instance
monitor = LedSwitchMonitor()


def signal_handler(signum, frame):
    """Signal handler for graceful shutdown"""
    logger.info(f"Received signal {signum}, exiting...")
    monitor.stop()
    sys.exit(0)


def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python ledswitch_monitor.py {start|stop}")
        sys.exit(1)
    
    command = sys.argv[1]
    
    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    if command == 'start':
        monitor.start()
        # Keep main thread running
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            monitor.stop()
    elif command == 'stop':
        monitor.stop()
    else:
        print(f"Unknown command: {command}")
        print("Usage: python ledswitch_monitor.py {start|stop}")
        sys.exit(1)


if __name__ == "__main__":
    main()
