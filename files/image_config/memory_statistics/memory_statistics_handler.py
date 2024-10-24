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
    """
    A utility class that converts dictionaries into objects, allowing access to dictionary keys as object attributes.
    It also supports nested dictionaries and lists of dictionaries, converting them recursively into objects.
    This class includes a method to revert the object back to its original dictionary form.
    """

    def __init__(self, d: dict):
        """
        Initializes the Dict2Obj object by converting the input dictionary into an object where each key can be accessed 
        as an attribute. Recursively handles nested dictionaries and lists.
        
        :param d: A dictionary to be converted into an object.
        """
        for key, value in d.items():
            if isinstance(value, (list, tuple)):
                setattr(self, key, [Dict2Obj(x) if isinstance(x, dict) else x for x in value])
            else:
                setattr(self, key, Dict2Obj(value) if isinstance(value, dict) else value)

    def to_dict(self) -> dict:
        """
        Converts the Dict2Obj object back into its original dictionary format. Handles recursive conversion of nested 
        Dict2Obj instances and lists of objects.
        
        :return: A dictionary representation of the Dict2Obj object.
        """
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
    
class TimeProcessor:
    """
    The TimeProcessor class manages time validation and calculations for data collection within 
    defined sampling intervals and retention periods. It ensures valid time ranges, corrects date 
    formats, and calculates differences in days and hours, total duration in minutes/hours, and 
    remaining days since specified times. Additionally, it processes request data to ensure 
    compliance with retention limits and sampling requirements.
    """
    def __init__(self, sampling_interval: int, retention_period: int):
        """
        Initializes TimeProcessor with the provided collection interval and retention period.

        :param sampling_interval: Interval between data collections, in minutes (3 to 15).
        :param retention_period: Period for data retention, in days (1 to 30).
        """
        self.date_time_format = "%Y-%m-%d %H:%M:%S"
        self.short_date_format = "%Y-%m-%d"
        
        if not (3 <= sampling_interval <= 15):
            raise ValueError("Data collection interval must be between 3 and 15 minutes.")
        if not (1 <= retention_period <= 30):
            raise ValueError("Retention period must be between 1 and 30 days.")

        self.sampling_interval = sampling_interval
        self.retention_period = retention_period

    def ensure_valid_time_range(self, request_data: Dict[str, Any]) -> None:
        """
        Ensures that the 'from' and 'to' time values in the request data are valid.
        If 'to' time is earlier than 'from', they are swapped.

        :param request_data: Dictionary containing request parameters including 'from' and 'to' dates.
        """
        try:
            from_time_str = Utility.fetch_current_date(request_data, 'from', self.date_time_format)
            to_time_str = Utility.fetch_current_date(request_data, 'to', self.date_time_format)

            from_time = datetime.fromisoformat(from_time_str)
            to_time = datetime.fromisoformat(to_time_str)

            if to_time < from_time:
                request_data['from'], request_data['to'] = request_data['to'], request_data['from']
                logging.info(f"Swapped 'from' and 'to': {request_data['from']} <-> {request_data['to']}")
            else:
                logging.info("No need to swap times.")

        except (ValueError, TypeError) as error:
            logging.error(f"Error in ensuring valid time range: {error}")
            raise ValueError("Invalid 'from' or 'to' date format.") from error

    def _parse_and_validate_dates(self, request_data: Dict[str, Any]) -> None:
        """
        Parses and validates 'from' and 'to' dates in the request data.
        If absent, defaults 'to' to the current time and 'from' to the retention period ago.

        :param request_data: Dictionary containing request parameters with potential date values.
        """
        if not request_data.get('to'):
            request_data['to'] = "now"
        if not request_data.get('from'):
            request_data['from'] = f"-{self.retention_period} days"  

        self.ensure_valid_time_range(request_data)

    def _fetch_time_values(self, request_data: Dict[str, Any]) -> Dict[str, datetime]:
        """
        Fetches and converts 'from', 'to', and current time values from request data.

        :param request_data: Dictionary containing request parameters including time values.
        :return: Dictionary with start_time, end_time, and current_time as datetime objects.
        """
        return {
            'start_time': datetime.fromisoformat(Utility.fetch_current_date(request_data, 'from', self.date_time_format)),
            'end_time': datetime.fromisoformat(Utility.fetch_current_date(request_data, 'to', self.date_time_format)),
            'current_time': datetime.fromisoformat(Utility.fetch_current_date({}, 'current_time', self.date_time_format))
        }

    def _validate_time_ranges(self, start_time: datetime, end_time: datetime, current_time: datetime, time_difference: timedelta) -> None:
        """
        Validates the provided time ranges to ensure they adhere to defined constraints.

        :param start_time: The start time of the data collection period.
        :param end_time: The end time of the data collection period.
        :param current_time: The current time for comparison.
        :param time_difference: The difference between end_time and start_time.
        """
        time_difference_obj = Utility.convert_timedelta_to_obj(time_difference)
        
        if end_time > current_time:
            raise ValueError("Datetime format error: 'to' time should not be greater than current time.")
        elif time_difference_obj.days > self.retention_period:
            raise ValueError(f"Datetime format error: time range should not exceed {self.retention_period} days.")
        elif time_difference_obj.days == 0 and time_difference_obj.hours == 0 and time_difference_obj.minutes < self.sampling_interval:
            raise ValueError(f"Datetime format error: time difference should be at least {self.sampling_interval} minutes.")

    def _calculate_day_and_hour_differences(self, start_time: datetime, end_time: datetime, request_data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Calculates the number of days and hour differences between start and end times.

        :param start_time: The starting point of the data collection period.
        :param end_time: The ending point of the data collection period.
        :param request_data: Dictionary containing request parameters for fetching dates.
        :return: Dictionary containing the number of days and hour differences.
        """
        start_date_str = Utility.fetch_current_date(request_data, 'from', self.short_date_format)
        end_date_str = Utility.fetch_current_date(request_data, 'to', self.short_date_format)
        
        start_date = datetime.fromisoformat(start_date_str)
        end_date = datetime.fromisoformat(end_date_str)
        
        day_diff = end_date - start_date
        day_diff_obj = Utility.convert_timedelta_to_obj(day_diff)
        
        start_hour_diff = Utility.convert_timedelta_to_obj(start_time - start_date)
        end_hour_diff = Utility.convert_timedelta_to_obj(end_time - end_date)
        
        return {
            'num_days': day_diff_obj.days,
            'start_hour': start_hour_diff.hours,
            'end_hour': end_hour_diff.hours
        }

    def _calculate_total_hours_and_minutes(self, time_diff: timedelta) -> Dict[str, int]:
        """
        Calculates the total hours and minutes from a given time difference.

        :param time_diff: The time difference as a timedelta object.
        :return: Dictionary containing total hours and total minutes.
        """
        time_diff_obj = Utility.convert_timedelta_to_obj(time_diff)
        total_hours = (time_diff_obj.days * 24) + time_diff_obj.hours
        total_minutes = (time_diff_obj.days * 24 * 60) + (time_diff_obj.hours * 60) + time_diff_obj.minutes
        return {
            'total_hours': total_hours,
            'total_minutes': total_minutes
        }

    def _calculate_remaining_days_since_end(self, end_time: datetime, current_time: datetime) -> Dict[str, int]:
        """
        Calculates the remaining days since the end time and prepares related data.

        :param end_time: The ending time of the data collection period.
        :param current_time: The current time for calculation.
        :return: Dictionary containing remaining days since end and the next day.
        """
        time_since_end = current_time - end_time
        time_since_end_obj = Utility.convert_timedelta_to_obj(time_since_end)
        
        return {
            'start_day': time_since_end_obj.days,
            'end_day': time_since_end_obj.days + 1
        }

    def process_time_information(self, request_data: Dict[str, Any]) -> None:
        """
        Main method to process time information from request data.
        It validates dates, fetches time values, validates time ranges, and calculates various time metrics.

        :param request_data: Dictionary containing request parameters to be processed.
        """
        self._parse_and_validate_dates(request_data)

        time_values = self._fetch_time_values(request_data)
        start_time = time_values['start_time']
        end_time = time_values['end_time']
        current_time = time_values['current_time']

        time_difference = end_time - start_time
        self._validate_time_ranges(start_time, end_time, current_time, time_difference)

        day_and_hour_diffs = self._calculate_day_and_hour_differences(start_time, end_time, request_data)
        total_time = self._calculate_total_hours_and_minutes(time_difference)

        retention_period_days = self.retention_period

        remaining_days_start = (current_time - start_time).days
        remaining_days_end = (current_time - end_time).days

        request_data["time_data"] = {
            "start_time_obj": start_time,
            "end_time_obj": end_time,
            "retention_period_days": retention_period_days,  # Added retention period in days
            "num_days": day_and_hour_diffs['num_days'],
            "total_hours": total_time['total_hours'],
            "total_minutes": total_time['total_minutes'],
            "start_hour": day_and_hour_diffs['start_hour'],
            "end_hour": day_and_hour_diffs['end_hour'],
            "days": time_difference.days,
            "hours": time_difference.seconds // 3600,
            "minutes": (time_difference.seconds % 3600) // 60,
            "start_day": remaining_days_start,
            "end_day": remaining_days_end
        }    

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