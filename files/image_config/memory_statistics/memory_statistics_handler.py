import psutil
import os
import signal
import sys
import time
import logging
import gzip
import json
import threading
import configparser
import syslog
import shutil
import re
import math
import socket
import stat
import copy
from typing import Dict, Any
from collections import Counter
from datetime import datetime, timedelta
from swsscommon.swsscommon import ConfigDBConnector
import argparse
import dateparser
import traceback

class SyslogLogger:
    """
    A general-purpose logger class for logging messages using syslog.
    Provides the ability to log messages with different severity levels,
    and optionally print the messages to the console.
    """

    def __init__(self, identifier, log_to_console=False):
        """
        Initializes the logger with a syslog identifier and optional console logging.
        :param identifier: A string that identifies the syslog entries.
        :param log_to_console: A boolean indicating whether to also print log messages to the console.
        """
        self.syslog_identifier = identifier
        self.log_to_console = log_to_console

        # Open the syslog connection with the given identifier
        syslog.openlog(ident=self.syslog_identifier, logoption=syslog.LOG_PID, facility=syslog.LOG_DAEMON)

    def log(self, level, message):
        """
        Logs a message to syslog and optionally to the console.
        :param level: The severity level (e.g., syslog.LOG_ERR, syslog.LOG_INFO, etc.).
        :param message: The log message to be recorded.
        """
        # Log to syslog
        syslog.syslog(level, message)

        # If console logging is enabled, print the message
        if self.log_to_console:
            print(message)

    def log_emergency(self, message):
        """Logs a message with the 'EMERGENCY' level."""
        self.log(syslog.LOG_EMERG, message)

    def log_alert(self, message):
        """Logs a message with the 'ALERT' level."""
        self.log(syslog.LOG_ALERT, message)

    def log_critical(self, message):
        """Logs a message with the 'CRITICAL' level."""
        self.log(syslog.LOG_CRIT, message)

    def log_error(self, message):
        """Logs a message with the 'ERROR' level."""
        self.log(syslog.LOG_ERR, message)

    def log_warning(self, message):
        """Logs a message with the 'WARNING' level."""
        self.log(syslog.LOG_WARNING, message)

    def log_notice(self, message):
        """Logs a message with the 'NOTICE' level."""
        self.log(syslog.LOG_NOTICE, message)

    def log_info(self, message):
        """Logs a message with the 'INFO' level."""
        self.log(syslog.LOG_INFO, message)

    def log_debug(self, message):
        """Logs a message with the 'DEBUG' level."""
        self.log(syslog.LOG_DEBUG, message)

    def close_logger(self):
        """
        Closes the syslog connection.
        """
        syslog.closelog()
        
class Dict2Obj(object):
    def __init__(self, d):
        for key, value in d.items():
            if isinstance(value, (list, tuple)):
                setattr(self, key, [Dict2Obj(x) if isinstance(x, dict) else x for x in value])
            else:
                setattr(self, key, Dict2Obj(value) if isinstance(value, dict) else value)

    def to_dict(self):
        result = {}
        for key in self.__dict__:
            value = getattr(self, key)
            if isinstance(value, Dict2Obj):
                result[key] = value.to_dict()
            elif isinstance(value, list):
                result[key] = [v.to_dict() if isinstance(v, Dict2Obj) else v for v in value]
            else:
                result[key] = value
        return result

class Utility:
    """
    A utility class offering methods for date handling, time delta formatting, and memory size formatting.
    This class also includes enhanced logging and error handling.
    """
    @staticmethod
    def fetch_current_date(request=None, time_key='current_time', date_format="%Y-%m-%d %H:%M:%S"):
        """
        Fetches or parses the current date or a user-specified date.
        :param request: contain none.
        :param time_key: The key to extract the date from the request (default: 'current_time').
        :param date_format: The format in which the date should be returned.
        :return: The formatted date string.
        """
        if request is not None and time_key in request and request[time_key] is not None:
            try:
                parsed_date = dateparser.parse(request[time_key])
                if parsed_date is None:
                    raise ValueError(f"Could not parse date: {request[time_key]}")
                formatted_date = parsed_date.strftime(date_format)
            except Exception as e:
                raise Exception(f"Date format error, input: {request[time_key]}, error: {str(e)}")
        else:
            formatted_date = datetime.now().strftime(date_format)
        
        return formatted_date
    
    @staticmethod
    def _format_timedelta_as_dict(time_delta: timedelta) -> Dict[str, int]:
        """
        Converts a timedelta object into a dictionary containing days, hours, minutes, and seconds.
        :param time_delta: A timedelta object.
        :return: A dictionary representation of the timedelta.
        """
        days = time_delta.days
        total_seconds = time_delta.seconds
        hours = total_seconds // 3600
        minutes = (total_seconds % 3600) // 60
        seconds = total_seconds % 60
        
        return {"days": days, "hours": hours, "minutes": minutes, "seconds": seconds}

    @classmethod
    def format_timedelta_as_dict(cls, time_delta: timedelta) -> Dict[str, int]:
        """
        Class method to format a timedelta object as a dictionary.
        :param time_delta: A timedelta object.
        :return: A dictionary representation of the timedelta.
        """
        return cls._format_timedelta_as_dict(time_delta)

    @classmethod
    def convert_timedelta_to_obj(cls, time_delta: timedelta) -> 'Dict2Obj':
        """
        Converts a timedelta object into a Dict2Obj instance.
        :param time_delta: A timedelta object.
        :return: A Dict2Obj instance representing the timedelta.
        """
        return Dict2Obj(cls._format_timedelta_as_dict(time_delta))
    
    @staticmethod
    def format_memory_size(size):
        """
        Formats a memory size in bytes into a human-readable string with appropriate units.
        :param size: Memory size in bytes.
        :return: A formatted string representing the memory size.
        """
        units = ('KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB')
        unit_prefix = {unit: 1 << (i + 1) * 10 for i, unit in enumerate(units)}
        abs_size = abs(size)
        
        for unit in reversed(units):
            if abs_size >= unit_prefix[unit]:
                value = float(abs_size) / unit_prefix[unit]
                return f"-{value:.2f}{unit}" if size < 0 else f"{value:.2f}{unit}"
        
        return f"{size}B" if size >= 0 else f"-{size}B"

class MemoryStatisticsDaemon:
    """
    Memory Statistics Daemon
    Responsible for collecting and storing memory usage statistics.
    1) Handle enabling or disabling the daemon via ConfigDB.
    2) Collect memory statistics at a configurable sampling interval.
    3) Handle retention of memory logs, deleting old logs when necessary.
    4) Respond to signals for reloading config (SIGHUP) and shutdown (SIGTERM).
    """

    def __init__(self):
        """
        Initialize the MemoryStatisticsDaemon.
        Sets up directories, log files, and loads default settings.
        Also sets up signal handling for reloading and shutdown events.
        """
        self.hdir = "var/log/memory_statistics"
        self.filename = os.path.join(self.hdir, "memory-statistics.log.gz")
        self.log_file = "var/log/memory_statistics_daemon.log"
        self.logger = logger(self.log_file, log_console=False)  # Initialize logger
        os.makedirs(self.hdir, exist_ok=True)  # Ensure memory statistics directory exists

        # Set up threading events to control running, reloading, and shutdown behavior
        self.running = threading.Event()
        self.reloading = threading.Event()
        self.shutdown_event = threading.Event()

        # Setup signal handlers for SIGHUP (reload) and SIGTERM (shutdown)
        signal.signal(signal.SIGHUP, self.handle_sighup)
        signal.signal(signal.SIGTERM, self.handle_sigterm)

        # Load default setting
        self.load_default_settings()

    def load_default_settings(self):
        """
        Load default settings from the config file.
        If no config file is found, fallback values are used.
        """
        config = configparser.ConfigParser()
        config.read('etc/memory_statistics.conf')  # Read configuration from the config file
        self.retention_period = config.getint('default', 'retention_period', fallback=15)  # Default retention period
        self.sampling_interval = config.getint('default', 'sampling_interval', fallback=5)  # Default sampling interval

    def load_config_from_db(self):
        """
        Load runtime configuration from the ConfigDB.
        Retrieves enable/disable state, retention period, and sampling interval.
        """
        self.config_db = ConfigDBConnector()
        self.config_db.connect()  # Connect to ConfigDB

        try:
            config = self.config_db.get_table('MEMORY_STATISTICS')  # Get memory statistics config table

            # Update retention period and sampling interval with values from the database
            self.retention_period = int(config.get('retention-period', self.retention_period))
            self.sampling_interval = int(config.get('sampling-interval', self.sampling_interval))

            # Check if the daemon should be enabled or disabled
            enable_state = config.get('enable', 'false').lower() == 'true'
            if not enable_state:
                self.logger.log("Received disable command, shutting down daemon.", logging.INFO)
                self.handle_sigterm(None, None)

            self.logger.log("Configuration reloaded from ConfigDB.", logging.INFO)
        except Exception as e:
            self.logger.log(f"Error loading configuration from ConfigDB: {e}, using defaults", logging.ERROR)

    def handle_sighup(self, signum, frame):
        """
        Handle SIGHUP signal for reloading the configuration.
        """
        self.logger.log("Received SIGHUP, reloading configuration.", logging.INFO)
        self.reloading.set()  # Trigger reload

    def handle_sigterm(self, signum, frame):
        """
        Handle SIGTERM signal for graceful shutdown.
        """
        self.logger.log("Received SIGTERM, shutting down gracefully.", logging.INFO)
        self.shutdown_event.set()  # Trigger shutdown

    def format_memory_size(self, size):
        """
        Convert memory size to human-readable format (e.g., MB, GB).
        """
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if size < 1024:
                return f"{size:.2f} {unit}"
            size /= 1024

    def collect_memory_statistics(self):
        """
        Collect memory statistics using psutil.
        """
        mem = psutil.virtual_memory()
        return {
            'total_memory': self.format_memory_size(mem.total),
            'used_memory': self.format_memory_size(mem.used),
            'free_memory': self.format_memory_size(mem.free),
            'available_memory': self.format_memory_size(mem.available),
            'cached_memory': self.format_memory_size(mem.cached),
            'buffer_memory': self.format_memory_size(mem.buffers),
            'shared_memory': self.format_memory_size(mem.shared)
        }

    def store_memory_statistics(self, memory_statistics):
        """
        Store memory statistics in a gzipped file.
        """
        try:
            with gzip.open(self.filename, 'wt') as gz_file:
                gz_file.write(f"{memory_statistics}\n")
        except Exception as e:
            self.logger.log(f"Error writing memory statistics to gzip file: {e}", logging.ERROR)

    def cleanup_old_files(self):
        """
        Clean up old memory statistics log files.
        Removes only .gz files, not the .log files.
        """
        for file in os.listdir(self.hdir):
            if file.endswith('.gz'):
                file_path = os.path.join(self.hdir, file)
                if os.path.exists(file_path):
                    try:
                        os.remove(file_path)  # Remove the .gz log file
                        self.logger.log(f"Deleted old log file: {file_path}", logging.INFO)
                    except Exception as e:
                        self.logger.log(f"Error deleting old log file: {e}", logging.ERROR)

    def run_memory_collection(self):
        """
        Thread to collect and store memory statistics periodically.
        """
        while not self.shutdown_event.is_set():
            try:
                memory_statistics = self.collect_memory_statistics()
                self.store_memory_statistics(memory_statistics)
            except Exception as e:
                self.logger.log(f"Error collecting or storing memory statistics: {e}", logging.ERROR)
            if self.shutdown_event.wait(self.sampling_interval):
                break

    def run(self):
        """
        Main entry point to start the daemon.
        """
        self.logger.log("Memory statistics daemon started.", logging.INFO)
        self.cleanup_old_files()

        memory_thread = threading.Thread(target=self.run_memory_collection, daemon=True)
        memory_thread.start()

        while not self.shutdown_event.is_set():
            time.sleep(1)

        memory_thread.join()  # Wait for memory collection thread to finish
        self.logger.log("Memory statistics daemon stopped.", logging.INFO)


if __name__ == "__main__":

    memory_statistics_config = {
        'SYSLOG_ID': "memstats#log",
        'SYSLOG_CONSOLE': True,
    }
    logger = SyslogLogger(
        identifier=memory_statistics_config['SYSLOG_ID'],
        log_to_console=memory_statistics_config['SYSLOG_CONSOLE']
    )

    try:
        daemon = MemoryStatisticsDaemon()
        daemon.run()
    except Exception as e:
        logging.error(f"Fatal error in MemoryStatisticsDaemon: {e}")
        sys.exit(1)