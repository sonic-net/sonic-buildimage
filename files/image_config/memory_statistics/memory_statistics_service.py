#!/usr/bin/env python3

# Standard library imports
import configparser
import contextlib
import gzip
import logging
import math
import os
import re
import shutil
import signal
import socket
import sys
import syslog
import tempfile
import threading
import time
import traceback
from datetime import datetime, timedelta
from typing import Any, Dict

# Third-party imports
import dateparser
import json
import psutil

# Local application/library imports
from swsscommon.swsscommon import ConfigDBConnector


SYSTEM_MEMORY_KEY = 'system_memory'

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


class SyslogLogger:
    """
    A general-purpose logger class for logging messages using syslog.
    Provides the ability to log messages with different severity levels,
    and optionally print the messages to the console.
    """

    def __init__(self, identifier, log_to_console=False):
        """
        Initialize the SyslogLogger with a syslog identifier and console logging option.
        
        :param identifier: A string that identifies the syslog entries.
        :param log_to_console: A boolean indicating whether to also print log messages to the console.
        """
        self.syslog_identifier = identifier
        self.log_to_console = log_to_console
        self.is_open = False

    def __enter__(self):
        """
        Context manager entry method to open syslog connection.
        Ensures the connection is opened only if not already open.
        
        :return: The logger instance
        """
        if not self.is_open:
            syslog.openlog(
                ident=self.syslog_identifier, 
                logoption=syslog.LOG_PID, 
                facility=syslog.LOG_DAEMON
            )
            self.is_open = True
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Context manager exit method to close syslog connection.
        
        :param exc_type: Exception type if an exception occurred
        :param exc_val: Exception value if an exception occurred
        :param exc_tb: Traceback if an exception occurred
        """
        self.close()

    def close(self):
        """
        Explicitly close the syslog connection if it's open.
        """
        if self.is_open:
            syslog.closelog()
            self.is_open = False

    def log(self, level, message):
        """
        Log a message to syslog and optionally to the console.
        
        :param level: The severity level (e.g., syslog.LOG_ERR, syslog.LOG_INFO)
        :param message: The log message to be recorded
        """
        syslog.syslog(level, message)
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

    @contextlib.contextmanager
    def managed_logging(self):
        """
        Additional context manager method for more explicit logging management.
        
        :yields: The logger instance
        """
        try:
            with self:
                yield self
        finally:
            self.close()


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
            except (ValueError, TypeError) as e:
                error_message = f"Date format error, input: {request[time_key]}, error: {str(e)}"
                logger.log_error(error_message)
                raise ValueError(error_message)
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
        Validates the input type and range, and raises appropriate exceptions for invalid inputs.
        
        :param size: Memory size in bytes (int or float).
        :return: A formatted string representing the memory size.
        :raises TypeError: If the input is not a number.
        :raises ValueError: If the input size is not finite or exceeds reasonable bounds.
        """
        try:
            if not isinstance(size, (int, float)):
                error_message = f"Invalid type for size: {type(size)}. Expected int or float."
                logger.log_error(error_message)
                raise TypeError(error_message)
            
            if not math.isfinite(size):
                error_message = "Size must be a finite number."
                logger.log_error(error_message)
                raise ValueError(error_message)
            
            max_size = 1 << 90 
            if abs(size) > max_size:
                error_message = f"Size exceeds maximum supported value of {max_size} bytes."
                logger.log_error(error_message)
                raise ValueError(error_message)
            
            units = ('B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB')
            abs_size = abs(size)
            
            for i, unit in enumerate(units):
                threshold = 1 << (i * 10) 
                next_threshold = 1 << ((i + 1) * 10) if i + 1 < len(units) else float('inf')
                
                if abs_size < next_threshold:
                    if unit == 'B':
                        return f"{size}B" if size >= 0 else f"-{abs(size)}B"
                    
                    value = abs_size / threshold
                    return f"-{value:.2f}{unit}" if size < 0 else f"{value:.2f}{unit}"
            
            return f"{size}B"
        
        except (TypeError, ValueError) as e:
            logger.log_error(f"Error occurred: {e}")
            raise
        except RuntimeError as e:
            logger.log_error(f"Unexpected error formatting memory size: {e}")
            raise


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
                logger.log_info(f"Swapped 'from' and 'to': {request_data['from']} <-> {request_data['to']}")
            else:
                logger.log_info("No need to swap times.")

        except (ValueError, TypeError) as error:
            logger.log_error(f"Error in ensuring valid time range: {error}")
            raise ValueError("Invalid 'from' or 'to' date format.") from error

    def _parse_and_validate_dates(self, request_data: Dict[str, Any]) -> None:
        """
        Parses and validates 'from' and 'to' dates in the request data.
        If absent, defaults 'to' to the current time and 'from' to the retention period ago.

        :param request_data: Dictionary containing request parameters with potential date values.
        """
        if not isinstance(request_data, dict):
            error_message = f"Invalid request_data type: {type(request_data)}. Expected dict."
            logger.log_error(error_message)
            raise TypeError(error_message)

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

    def _validate_time_ranges(self, start_time: datetime, end_time: datetime, current_time: datetime, time_diff: timedelta) -> None:
        """
        Validates the provided time ranges to ensure they adhere to defined constraints.

        :param start_time: The start time of the data collection period.
        :param end_time: The end time of the data collection period.
        :param current_time: The current time for comparison.
        :param time_difference: The difference between end_time and start_time.
        """
        try:
            time_diff_dict = Utility.format_timedelta_as_dict(time_diff)
        
            if end_time > current_time:
                error_message = "End time cannot be in the future."
                logger.log_error(error_message)
                raise ValueError(error_message)
            elif time_diff_dict['days'] > self.retention_period:
                error_message = f"Time range cannot exceed retention period of {self.retention_period} days."
                logger.log_error(error_message)
                raise ValueError(error_message)
            elif time_diff_dict['days'] == 0 and time_diff_dict['hours'] == 0 and time_diff_dict['minutes'] < self.sampling_interval:
                error_message = f"Time difference must be at least {self.sampling_interval} minutes."
                logger.log_error(error_message)
                raise ValueError(error_message)
        except ValueError as error:
            logger.log_error(f"Error validating time ranges: {error}")
            raise

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
        day_diff_dict = Utility.format_timedelta_as_dict(day_diff)
        
        start_hour_diff = Utility.format_timedelta_as_dict(start_time - start_date)
        end_hour_diff = Utility.format_timedelta_as_dict(end_time - end_date)
        
        return {
            'num_days': day_diff_dict['days'],
            'start_hour': start_hour_diff['hours'],
            'end_hour': end_hour_diff['hours']
        }

    def _calculate_total_hours_and_minutes(self, time_diff: timedelta) -> Dict[str, int]:
        """
        Calculates the total hours and minutes from a given time difference.

        :param time_diff: The time difference as a timedelta object.
        :return: Dictionary containing total hours and total minutes.
        """
        time_diff_dict = Utility.format_timedelta_as_dict(time_diff)
        total_hours = (time_diff_dict['days'] * 24) + time_diff_dict['hours']
        total_minutes = (time_diff_dict['days'] * 24 * 60) + (time_diff_dict['hours'] * 60) + time_diff_dict['minutes']
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
        time_since_end_dict = Utility.format_timedelta_as_dict(time_since_end)
        
        return {
            'start_day': time_since_end_dict['days'],
            'end_day': time_since_end_dict['days'] + 1
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
        remaining_days = self._calculate_remaining_days_since_end(end_time, current_time)

        request_data["time_data"] = {
            "start_time_obj": start_time,
            "end_time_obj": end_time,
            "retention_period_days": self.retention_period, 
            "num_days": day_and_hour_diffs['num_days'],
            "total_hours": total_time['total_hours'],
            "total_minutes": total_time['total_minutes'],
            "start_hour": day_and_hour_diffs['start_hour'],
            "end_hour": day_and_hour_diffs['end_hour'],
            "days": time_difference.days,
            "hours": time_difference.seconds // 3600,
            "minutes": (time_difference.seconds % 3600) // 60,
            "start_day": remaining_days['start_day'],
            "end_day": remaining_days['end_day']
        }


class MemoryReportGenerator:
    """
    This class generates a formatted memory statistics report based on specified time intervals and duration.
    It is initialized with request data for start and end times, and a step size for report granularity.
    The class includes methods to create interval labels and generate a structured report header, 
    detailing metrics and their corresponding values for easy analysis of memory statistics over the defined time frame.
    """

    def __init__(self, request, step):
        """
        Initialize the report generator with request data and step size.
        
        Args:
            request (dict): The request data containing time and duration information.
            step (int): The step size for the report intervals (e.g., 1 for 1 minute).
        """
        self.request = request
        self.step = step
        self.start = request['time_data']['start_time_obj']
        self.end = request['time_data']['end_time_obj']
        self.period = request['duration']  
        self.step_timedelta = timedelta(**{self.period: self.step})

    def get_interval_column_label(self, slot):
        """
        Create a formatted label for time intervals based on the specified duration unit.
        
        Args:
            slot (datetime): The current time slot for which the label is being generated.
        
        Returns:
            tuple: A tuple containing the interval label and a time label.
        """
        if self.period == "days":
            return "D{:02d}-D{:02d}".format(slot.day, (slot + self.step_timedelta).day), slot.strftime('%d%b%y')
        elif self.period == "hours":
            return "H{:02d}-H{:02d}".format(slot.hour, (slot + self.step_timedelta).hour), slot.strftime('%H:%M')
        else:
            return "M{:02d}-M{:02d}".format(slot.minute, (slot + self.step_timedelta).minute), slot.strftime('%H:%M')

    def get_memmory_statistics_report_header(self):
        """
        Generate a well-aligned, formatted header for the memory report.
        
        Returns:
            str: The formatted header for the memory statistics report.
        """
        fmt = "\nCodes:\tM - minutes, H - hours, D - days\n"
        fmt += "-" * 80 + "\n"

        fmt += "Report Generated:    {}\n".format(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        fmt += "Analysis Period:     From {} to {}\n".format(self.start.strftime("%Y-%m-%d %H:%M:%S"),
                                                             self.end.strftime("%Y-%m-%d %H:%M:%S"))
        fmt += "Interval:            {} {}\n".format(self.step, self.period.capitalize())

        slot = self.start
        num_intervals = 0
        interval_labels = []
        time_labels = []

        while slot <= self.end:
            interval_label, time_label = self.get_interval_column_label(slot)
            interval_labels.append(interval_label)
            time_labels.append(time_label)
            num_intervals += 1
            slot += self.step_timedelta

        METRIC_WIDTH = 18
        VALUE_WIDTH = 10
        INTERVAL_WIDTH = 11

        total_width = (METRIC_WIDTH + 3 * (VALUE_WIDTH + 1) +
                       (INTERVAL_WIDTH + 1) * num_intervals - 1) 

        separator = "-" * total_width + "\n"

        header = separator
        header += "{:<{}} {:<{}} {:<{}} {:<{}}".format(
            "Metric", METRIC_WIDTH, "Current", VALUE_WIDTH, "High", VALUE_WIDTH, "Low", VALUE_WIDTH
        )
        for label in interval_labels:
            header += " {:<{}}".format(label, INTERVAL_WIDTH)
        header += "\n"

        header += "{:<{}} {:<{}} {:<{}} {:<{}}".format(
            " ", METRIC_WIDTH, "Value", VALUE_WIDTH, "Value", VALUE_WIDTH, "Value", VALUE_WIDTH
        )
        for label in time_labels:
            header += " {:<{}}".format(label, INTERVAL_WIDTH)
        header += "\n"

        header += separator

        return fmt + header


class MemoryStatisticsCollector:
    """
    Handles system memory statistics collection, management, and retention. Initializes with a specified
    sampling interval (in minutes) and retention period (in days) to determine how frequently data is
    collected and how long it is retained.
    """

    def __init__(self, sampling_interval: int, retention_period: int):
        """
        Initializes MemoryStatisticsCollector with data collection interval and retention period.

        :param sampling_interval: Interval between data collections in minutes (3 to 15).
        :param retention_period: Data retention period in days (1 to 30).
        :raises ValueError: If sampling_interval or retention_period is out of valid range.
        """
        if not (3 <= sampling_interval <= 15):
            raise ValueError("Sampling interval must be between 3 and 15 minutes.")
        if not (1 <= retention_period <= 30):
            raise ValueError("Retention period must be between 1 and 30 days.")

        self.sampling_interval = sampling_interval
        self.retention_period = retention_period
        self.last_prune_time = None

    def fetch_memory_statistics(self):
        """
        Collect memory statistics using psutil.
        
        :return: Dictionary containing memory statistics.
        :raises RuntimeError: If psutil fails to collect memory data.
        """
        try:
            memory_data = psutil.virtual_memory()
            memory_stats = {
                'total_memory': {"prss": memory_data.total, "count": 1},
                'used_memory': {"prss": memory_data.used, "count": 1},
                'free_memory': {"prss": memory_data.free, "count": 1},
                'available_memory': {"prss": memory_data.available, "count": 1},
                'cached_memory': {"prss": memory_data.cached, "count": 1},
                'buffers_memory': {"prss": memory_data.buffers, "count": 1},
                'shared_memory': {"prss": memory_data.shared, "count": 1}
            }
            del memory_data
            return memory_stats
        except RuntimeError as e:
            logger.log_error(f"Failed to fetch memory statistics: {e}")
            raise

    def fetch_memory_entries(self, filepath: str):
        """
        Fetch memory entries from a compressed JSON file.
        
        :param filepath: Path to the compressed file containing memory entries.
        :return: Dictionary or list containing loaded memory entries or default structure.
        :raises TypeError: If filepath is not a string.
        :raises OSError: If file cannot be read.
        :raises json.JSONDecodeError: If JSON is invalid.
        """
        if not isinstance(filepath, str):
            error_message = f"Invalid filepath type: {type(filepath)}. Expected str."
            logger.log_error(error_message)
            raise TypeError(error_message)

        default_entry = (
            {SYSTEM_MEMORY_KEY: {'system': {}, "count": 0}, "count": 0}
            if 'total' in filepath
            else []
        )

        if not os.path.exists(filepath) or os.path.getsize(filepath) <= 0:
            return default_entry

        try:
            with gzip.open(filepath, 'rt', encoding='utf-8') as jfile:
                loaded_entry = json.load(jfile)
            return loaded_entry
        except OSError as e:
            logger.log_error(f"Failed to read memory entries from {filepath}: {e}")
            raise
        except json.JSONDecodeError as e:
            logger.log_error(f"Invalid JSON in memory entries from {filepath}: {e}")
            raise
        except TypeError as e:
            logger.log_error(f"Type error loading memory entries from {filepath}: {e}")
            raise

    def update_memory_statistics(self, total_dict: dict, mem_dict: dict, time_list: dict, item: str, category: str):
        """
        Update memory statistics for the specified item and category.
        
        :param total_dict: The total statistics dictionary.
        :param mem_dict: The memory statistics dictionary to update.
        :param time_list: The current memory statistics collected.
        :param item: The item to update in total_dict.
        :param category: The category under which to update the statistics.
        :raises TypeError: If inputs are not of expected types.
        """
        if not all(isinstance(x, dict) for x in [total_dict, mem_dict, time_list]):
            error_message = "total_dict, mem_dict, and time_list must be dictionaries."
            logger.log_error(error_message)
            raise TypeError(error_message)
        if not all(isinstance(x, str) for x in [item, category]):
            error_message = "item and category must be strings."
            logger.log_error(error_message)
            raise TypeError(error_message)

        if category not in mem_dict:
            mem_dict[category] = {}
        if item not in total_dict:
            total_dict[item] = {'count': 0}
        if category not in total_dict[item]:
            total_dict[item][category] = {}

        for memory_metric in time_list.keys():
            try:
                prss = int(time_list[memory_metric]['prss'])
                entry_data = {
                    "prss": prss, 
                    "count": 1,
                    "high_value": prss, 
                    "low_value": prss
                }
                mem_dict[category][memory_metric] = entry_data
                mem = total_dict[item][category]
                if memory_metric in mem:
                    tprss = int(mem[memory_metric]["prss"]) + prss
                    tcount = int(mem[memory_metric]["count"]) + 1
                    high_value = max(prss, int(mem[memory_metric].get("high_value", prss)))
                    low_value = min(prss, int(mem[memory_metric].get("low_value", prss)))
                    mem[memory_metric] = {
                        "prss": tprss, 
                        "count": tcount,
                        "high_value": high_value,
                        "low_value": low_value
                    }
                else:
                    mem[memory_metric] = entry_data
            except (TypeError, KeyError, ValueError) as e:
                logger.log_error(f"Error updating memory statistics for metric {memory_metric}: {e}")
                continue
        total_dict[item]['count'] = int(total_dict[item].get('count', 0)) + 1

    def enforce_retention_policy(self, total_dict: dict, individual_list: list, total_filepath: str, individual_filepath: str):
        """
        Enforce retention policy by removing entries older than the retention period for individual statistics only.
        
        :param total_dict: Dictionary containing total memory statistics (not pruned).
        :param individual_list: List containing individual memory statistics entries (pruned by retention period).
        :param total_filepath: Path to the total memory statistics file.
        :param individual_filepath: Path to the individual memory statistics file.
        :return: Number of entries removed.
        :raises TypeError: If inputs are not of expected types.
        :raises OSError: If file writing fails.
        """
        if not isinstance(total_dict, dict):
            error_message = f"Invalid total_dict type: {type(total_dict)}. Expected dict."
            logger.log_error(error_message)
            raise TypeError(error_message)
        if not isinstance(individual_list, list):
            error_message = f"Invalid individual_list type: {type(individual_list)}. Expected list."
            logger.log_error(error_message)
            raise TypeError(error_message)
        if not all(isinstance(x, str) for x in [total_filepath, individual_filepath]):
            error_message = "Filepaths must be strings."
            logger.log_error(error_message)
            raise TypeError(error_message)

        for filepath in [total_filepath, individual_filepath]:
            dir_path = os.path.dirname(filepath)
            if not os.access(dir_path, os.W_OK):
                logger.log_error(f"No write permission for directory: {dir_path}")
                raise OSError(f"No write permission for directory: {dir_path}")

        for filepath in [total_filepath, individual_filepath]:
            if os.path.exists(filepath) and os.path.getsize(filepath) > 0:
                try:
                    with gzip.open(filepath, 'rt', encoding='utf-8') as jfile:
                        json.load(jfile)
                    logger.log_info(f"Validated JSON integrity of file: {filepath}")
                except (OSError, json.JSONDecodeError) as e:
                    logger.log_error(f"Invalid or corrupted JSON in {filepath}: {e}")
                    raise json.JSONDecodeError(f"Corrupted JSON in {filepath}: {str(e)}", "", 0)

        current_time = datetime.now()
        retention_threshold = timedelta(days=self.retention_period)

        original_len = len(individual_list)
        indices_to_remove = []
        for i, entry in enumerate(individual_list):
            try:
                entry_time = datetime.fromisoformat(entry['current_time'])
                if current_time - entry_time > retention_threshold:
                    logger.log_info(f"Marking outdated individual entry for deletion with timestamp: {entry['current_time']}")
                    indices_to_remove.append(i)
            except (ValueError, TypeError, KeyError) as e:
                logger.log_error(f"Error processing individual entry timestamp at index {i}: {e}")
                indices_to_remove.append(i)

        for i in sorted(indices_to_remove, reverse=True):
            individual_list.pop(i)

        removed_entries = original_len - len(individual_list)
        logger.log_info(f"Retention policy enforced: Removed {removed_entries} individual entries")

        if removed_entries > 0:
            temp_file_path = None
            try:
                with tempfile.NamedTemporaryFile(mode='wb', suffix='.gz', dir=os.path.dirname(individual_filepath), delete=False) as temp_file:
                    temp_file_path = temp_file.name
                    with gzip.GzipFile(filename='', mode='wb', fileobj=temp_file, compresslevel=6) as gz_file:
                        gz_file.write(json.dumps(individual_list, default=str).encode('utf-8'))
                shutil.move(temp_file_path, individual_filepath)
                logger.log_info(f"Successfully wrote individual memory statistics to {individual_filepath} after removing {removed_entries} entries")
            except (OSError, json.JSONEncodeError) as e:
                logger.log_error(f"Failed to write individual memory statistics: {str(e)}")
                if temp_file_path and os.path.exists(temp_file_path):
                    os.unlink(temp_file_path)
                raise OSError(f"Failed to update individual memory statistics file: {e}")

        return removed_entries

    def collect_and_store_memory_usage(self, collect_only: bool):
        """
        Collect and store memory usage statistics into log files using JSON format.
        
        :param collect_only: If True, return memory data without storing.
        :return: Memory statistics dictionary if collect_only is True, None otherwise.
        :raises KeyError: If required configuration keys are missing.
        :raises RuntimeError: If memory statistics cannot be fetched.
        :raises OSError: If file operations fail.
        :raises json.JSONDecodeError: If JSON is invalid.
        :raises TypeError: If collect_only is not a boolean.
        """
        if not isinstance(collect_only, bool):
            error_message = f"Invalid collect_only type: {type(collect_only)}. Expected bool."
            logger.log_error(error_message)
            raise TypeError(error_message)

        if not isinstance(memory_statistics_config, dict):
            error_message = f"Invalid memory_statistics_config type: {type(memory_statistics_config)}. Expected dict."
            logger.log_error(error_message)
            raise TypeError(error_message)

        required_keys = ['TOTAL_MEMORY_STATISTICS_LOG_FILENAME', 'MEMORY_STATISTICS_LOG_FILENAME']
        if not all(key in memory_statistics_config for key in required_keys):
            error_message = f"Missing required configuration keys: {required_keys}"
            logger.log_error(error_message)
            raise KeyError(error_message)

        total_filepath = memory_statistics_config['TOTAL_MEMORY_STATISTICS_LOG_FILENAME']
        individual_filepath = memory_statistics_config['MEMORY_STATISTICS_LOG_FILENAME']

        try:
            total_dict = self.fetch_memory_entries(total_filepath)
            individual_list = self.fetch_memory_entries(individual_filepath)
        except (OSError, json.JSONDecodeError) as e:
            logger.log_error(f"Failed to fetch memory entries: {e}")
            total_dict = {SYSTEM_MEMORY_KEY: {'system': {}, "count": 0}, "count": 0}
            individual_list = []

        try:
            sm = self.fetch_memory_statistics()
        except RuntimeError as e:
            logger.log_error(f"Failed to fetch memory statistics: {e}")
            raise

        sysmem_dict = {"system": {}, "count": 1}
        if SYSTEM_MEMORY_KEY not in total_dict:
            total_dict[SYSTEM_MEMORY_KEY] = {'system': {}, "count": 0}
        if 'system' not in total_dict[SYSTEM_MEMORY_KEY]:
            total_dict[SYSTEM_MEMORY_KEY]['system'] = {}
            total_dict[SYSTEM_MEMORY_KEY]['count'] = 0

        try:
            self.update_memory_statistics(total_dict, sysmem_dict, sm, SYSTEM_MEMORY_KEY, 'system')
        except (TypeError, KeyError) as e:
            logger.log_error(f"Failed to update memory statistics: {e}")
            return None

        total_dict['count'] = int(total_dict.get('count', 0)) + 1
        current_time = Utility.fetch_current_date()
        mem_dict = {"current_time": current_time, SYSTEM_MEMORY_KEY: sysmem_dict, "count": 1}

        individual_list.append(mem_dict)

        current_time = datetime.now()
        if self.last_prune_time is None or (current_time - self.last_prune_time) >= timedelta(days=1):
            try:
                removed_entries = self.enforce_retention_policy(total_dict, individual_list, total_filepath, individual_filepath)
                self.last_prune_time = current_time
                logger.log_info(f"Enforced retention policy, removed {removed_entries} entries")
            except (TypeError, OSError) as e:
                logger.log_error(f"Failed to enforce retention policy: {e}")
                return None

        if collect_only:
            return mem_dict

        temp_file_path = None
        try:
            with tempfile.NamedTemporaryFile(mode='wb', suffix='.gz', dir=os.path.dirname(individual_filepath), delete=False) as temp_file:
                temp_file_path = temp_file.name
                with gzip.GzipFile(filename='', mode='wb', fileobj=temp_file, compresslevel=6) as gz_file:
                    gz_file.write(json.dumps(individual_list, default=str).encode('utf-8'))
            shutil.move(temp_file_path, individual_filepath)
            logger.log_info(f"Successfully wrote individual memory statistics to {individual_filepath}")
        except (OSError, json.JSONEncodeError) as e:
            logger.log_error(f"Failed to write individual memory statistics: {str(e)}")
            if temp_file_path and os.path.exists(temp_file_path):
                os.unlink(temp_file_path)
            return None

        temp_file_path = None
        try:
            with tempfile.NamedTemporaryFile(mode='wb', suffix='.gz', dir=os.path.dirname(total_filepath), delete=False) as temp_file:
                temp_file_path = temp_file.name
                with gzip.GzipFile(filename='', mode='wb', fileobj=temp_file, compresslevel=6) as gz_file:
                    gz_file.write(json.dumps(total_dict, default=str).encode('utf-8'))
            shutil.move(temp_file_path, total_filepath)
            logger.log_info(f"Successfully wrote total memory statistics to {total_filepath}")
        except (OSError, json.JSONEncodeError) as e:
            logger.log_error(f"Failed to write total memory statistics: {str(e)}")
            if temp_file_path and os.path.exists(temp_file_path):
                os.unlink(temp_file_path)
            return None

        return None



class MemoryEntryManager:
    """
    Manages memory entries by handling additions, aggregations, and retrieval of memory data entries across 
    different categories and types. This class is designed to support memory tracking and reporting in a 
    structured way for various items like 'system_memory'.
    """

    def add_memory_entry(self, request, total_entries_all, global_entries, local_entries, new_entry, item, category, entry_list):
        """
        Add memory entry to global and local entries.
        Args:
            request (dict): The request object containing parameters for filtering.
            total_entries_all (dict): Dictionary to hold total entries across all items.
            global_entries (dict): Dictionary to hold global memory entries.
            local_entries (dict): Dictionary to hold local memory entries.
            new_entry (dict): The new memory entry to be added.
            item (str): The item being processed (e.g., 'system_memory').
            category (str): The category of the memory entry.
            entry_list (str): The entry list name for aggregation.
        """
        if item not in global_entries:
            global_entries[item] = {}

        for metric_name in new_entry[item][category].keys():
            if 'metric_name' in request and request['metric_name'] is not None and not re.match(f".*{request['metric_name']}.*", metric_name):
                continue

            current_rss = int(new_entry[item][category][metric_name]['prss']) / int(new_entry[item][category][metric_name]['count'])

            if metric_name in global_entries[item][category]:
                global_entries[item][category][metric_name]['prss'] += int(new_entry[item][category][metric_name]['prss'])
                global_entries[item][category][metric_name]['count'] += int(new_entry[item][category][metric_name]['count'])
            else:
                global_entries[item][category][metric_name] = new_entry[item][category][metric_name].copy()

            local_entries[item][category][metric_name] = {'prss': current_rss}

            if metric_name in total_entries_all[entry_list]:
                total_entries_all[entry_list][metric_name] += int(global_entries[item][category][metric_name]['prss'])
            else:
                total_entries_all[entry_list][metric_name] = int(global_entries[item][category][metric_name]['prss'])

    def add_entry_total(self, total_entry, item, category):
        """
        Calculate and add average values for memory entries.
        Args:
            total_entry (dict): The dictionary containing total memory entries.
            item (str): The item to calculate averages for.
            category (str): The category of the memory entry.
        """
        for metric_name in total_entry[item][category].keys():
            total_entry[item][category][metric_name]['avg_value'] = int(total_entry[item][category][metric_name]['prss']) / int(total_entry[item][category][metric_name]['count'])

    def get_global_entry_data(self, request, total_entry_all, local_entry, entries):
        """
        Aggregate global entry data from local entries.
        Args:
            request (dict): The request object containing parameters for filtering.
            total_entry_all (dict): Dictionary to hold total entries across all items.
            local_entry (dict): Dictionary holding local memory entries.
            entries (dict): Dictionary containing memory entry time list.
        Returns:
            dict: Aggregated global memory entry data.
        """
        gentry = {SYSTEM_MEMORY_KEY: {"system": {}, "count": 0}, "count": 0}
        
        for day in range(0, int(entries['count']), 1):
            if entries["time_list"][day]['count'] == 0:
                continue
            
            self.add_memory_entry(request, total_entry_all, gentry, local_entry, entries["time_list"][day], SYSTEM_MEMORY_KEY, 'system', "system_list")
        
        self.add_entry_total(gentry, SYSTEM_MEMORY_KEY, 'system')
    
        return gentry

    def get_current_memory(self, request, current_memory, memory_type, metric_name):
        """
        Retrieve current memory value for a specified type and metric name.
        Args:
            request (dict): The request object containing parameters for filtering.
            current_memory (dict): Current memory data structure.
            memory_type (str): Type of memory to retrieve (e.g., 'system_memory').
            metric_name (str): Name of the metric to retrieve.
        Returns:
            int: Current memory value or "0" if not found.
        """
        if memory_type in current_memory and metric_name in current_memory[memory_type][request['type']]:
            return current_memory[memory_type][request['type']][metric_name]['prss']
        else:
            return "0"

    def get_memory_entry_data(self, request, total_entry_all, time_range):
        """
        Retrieve formatted memory entry data based on request parameters.
        Args:
            request (dict): The request object containing parameters for filtering.
            total_entry_all (dict): Dictionary to hold total entries across all items.
            time_range (str): Time range for fetching entries.
        Returns:
            str: Formatted memory entry data.
        """
        Memory_formatter = Utility()
  
        max_record_count = int(request.get('rcount', 200))
        count = 0
        formatted_output = ""

        selected_field = request.get('field', 'avg_value')

        memory_type = SYSTEM_MEMORY_KEY
        current_memory = request['current_memory']

        total_high_value, total_low_value = {}, {}

        METRIC_WIDTH = 18
        VALUE_WIDTH = 10
        INTERVAL_WIDTH = 11

        for metric_name in current_memory[memory_type].get(request['type'], {}):
            if request.get('metric_name') and not re.match(f".*{request['metric_name']}.*", metric_name):
                continue

            count += 1
            if count > max_record_count:
                formatted_output += "<< more records truncated, use filter query to optimize the search >>\n"
                break

            total_high_value[metric_name] = 0
            total_low_value[metric_name] = 0

            history_values = []

            for time_entry in total_entry_all.get("time_group_list", []):
                memory_data = time_entry.get(memory_type, {}).get(request['type'], {}).get(metric_name, {})
                if memory_data:
                    mem_value = int(memory_data.get(selected_field, 0))
                    history_values.append(Memory_formatter.format_memory_size(mem_value))

                    total_high_value[metric_name] = max(total_high_value[metric_name], mem_value)
                    total_low_value[metric_name] = min(total_low_value[metric_name] or mem_value, mem_value)
                else:
                    history_values.append('-')

            current_mem = Memory_formatter.format_memory_size(self.get_current_memory(request, current_memory, memory_type, metric_name))
            high_mem = Memory_formatter.format_memory_size(total_high_value[metric_name])
            low_mem = Memory_formatter.format_memory_size(total_low_value[metric_name])

            formatted_output += (
                f"{metric_name:<{METRIC_WIDTH}} "
                f"{current_mem:<{VALUE_WIDTH}} "
                f"{high_mem:<{VALUE_WIDTH}} "
                f"{low_mem:<{VALUE_WIDTH}} "
                + ' '.join(f"{v:<{INTERVAL_WIDTH}}" for v in history_values) + "\n"
            )

        return formatted_output


class MemoryStatisticsProcessor:
    """
    Class for processing and aggregating memory statistics over specified time slices.
    
    This class handles the collection, validation, and reporting of memory statistics, allowing for 
    configurable sampling intervals and retention periods. It processes data from files and organizes 
    it into time slices for analysis.
    """

    def __init__(self, memory_statistics_config, sampling_interval: int, retention_period: int):
        """
        Initializes the MemoryStatisticsProcessor with configuration settings.

        Parameters:
        - memory_statistics_config (dict): Configuration dictionary containing parameters for 
          memory statistics processing such as sampling interval and retention period.
        """
        self.memory_statistics_config = memory_statistics_config
        self.entry_manager = MemoryEntryManager()
        self.memory_formatter = Utility()
        self.sampling_interval = sampling_interval
        self.retention_period = retention_period  

    def process_memory_statistics_timeslice(self, request_data, memory_data_filename, first_interval_unit, first_interval_rate, 
                                            second_interval_unit, second_interval_rate, third_interval_unit, 
                                            third_interval_rate, primary_interval):
        """
        Processes memory statistics for a specified time slice based on the provided request data.

        Parameters:
        - request_data (dict): Request data containing time range and duration information.
        - memory_data_filename (str): The filename from which to read memory statistics data.
        - first_interval_unit (str): The first time interval unit.
        - first_interval_rate (int): The rate for the first interval.
        - second_interval_unit (str): The second time interval unit.
        - second_interval_rate (int): The rate for the second interval.
        - third_interval_unit (str): The third time interval unit.
        - third_interval_rate (int): The rate for the third interval.
        - primary_interval (str): The primary time interval.

        Returns:
        - str: Generated report of the processed memory statistics.
        """
        time_entry_summary = self.initialize_time_entry_summary()
        total_hours, num_columns, step = self.calculate_time_parameters(request_data, first_interval_unit, first_interval_rate, second_interval_unit)
                
        interval_minutes = self.sampling_interval / 60
        self.validate_sampling_interval(interval_minutes)

        start_time_obj, end_time_obj = request_data['time_data']['start_time_obj'], request_data['time_data']['end_time_obj']
        
        self.initialize_time_slices(time_entry_summary, num_columns)
        self.process_files(request_data, memory_data_filename, start_time_obj, end_time_obj, step, num_columns, time_entry_summary,
                          first_interval_unit, first_interval_rate, second_interval_unit)
        self.aggregate_data(request_data, time_entry_summary, num_columns)
        
        return self.generate_report(request_data, time_entry_summary, num_columns, step)

    def initialize_time_entry_summary(self):
        """
        Initializes the summary structure for time entries.

        Returns:
        - dict: A dictionary to hold system list and time group list.
        """
        return {"system_list": {}, "time_group_list": []}

    def calculate_time_parameters(self, request_data, first_interval_unit, first_interval_rate, second_interval_unit):
        """
        Calculates total hours, number of columns, and step for processing time slices.

        Parameters:
        - request_data (dict): Request data containing time range information.
        - first_interval_unit (str): The first time interval unit.
        - first_interval_rate (int): The rate for the first interval.
        - second_interval_unit (str): The second time interval unit.

        Returns:
        - tuple: Total hours, number of columns, and step size.
        """
        num_days = int(request_data['time_data']['num_days'])
        total_hours = self.calculate_total_hours(request_data, num_days, first_interval_unit, first_interval_rate, second_interval_unit)
        step = max(1, int(math.ceil(total_hours / 10)))
        num_columns = self.calculate_num_columns(total_hours, step)
        return total_hours, num_columns, step

    def calculate_total_hours(self, request_data, num_days, first_interval_unit, first_interval_rate, second_interval_unit):
        """
        Calculates the total hours based on request data and specified intervals.

        Parameters:
        - request_data (dict): Request data containing time range information.
        - num_days (int): Number of days specified in the request.
        - first_interval_unit (str): The first time interval unit.
        - first_interval_rate (int): The rate for the first interval.
        - second_interval_unit (str): The second time interval unit.

        Returns:
        - int: Total hours calculated based on input parameters.
        """
        if request_data['duration'] == "days":
            return num_days
        return (int(request_data['time_data'][first_interval_unit]) * first_interval_rate) + int(request_data['time_data'][second_interval_unit])

    def calculate_num_columns(self, total_hours, step):
        """
        Calculates the number of columns for the report based on total hours and step size.

        Parameters:
        - total_hours (int): Total hours to be processed.
        - step (int): Step size for processing time slices.

        Returns:
        - int: Number of columns to represent the time slices.
        """
        num_columns = int(math.ceil(total_hours / step))
        if total_hours % step == 0:
            num_columns += 1
        return num_columns

    def validate_sampling_interval(self, interval_minutes):
        """
        Validates the specified sampling interval to ensure it meets the defined criteria.

        Parameters:
        - interval_minutes (int): Sampling interval in minutes.

        Raises:
        - ValueError: If the sampling interval is not within the accepted range.
        """
        if interval_minutes <= 0:
            raise ValueError("Sampling interval must be a positive integer.")

        hourly_rate = 60 / int(interval_minutes)

        if hourly_rate > 20:
            raise ValueError(f"Invalid sampling interval, rate per hour: {hourly_rate}")

    def initialize_time_slices(self, time_entry_summary, num_columns):
        """
        Initializes the time slices for aggregating memory entries.

        Parameters:
        - time_entry_summary (dict): Summary structure to hold time group information.
        - num_columns (int): Number of columns for time slices.
        """
        for _ in range(num_columns):
            memory_entry = {SYSTEM_MEMORY_KEY: {}, "count": 0}
            time_entry = {"time_list": [memory_entry], "count": 0}
            time_entry_summary["time_group_list"].append(time_entry)

    def process_files(self, request_data, memory_data_filename, start_time_obj, end_time_obj, step, num_columns, time_entry_summary,
                 first_interval_unit, first_interval_rate, second_interval_unit):
        """
        Processes memory data file within the specified time range.

        Parameters:
        - request_data: Request data containing time range information.
        - memory_data_filename: The filename from which to read memory statistics data.
        - start_time_obj: The starting time object for filtering.
        - end_time_obj: The ending time object for filtering.
        - step: Step size for processing time slices.
        - num_columns: Number of columns for time slices.
        - time_entry_summary: Summary structure to hold time group information.
        - first_interval_unit: The first time interval unit.
        - first_interval_rate: The rate for the first interval.
        - second_interval_unit: The second time interval unit.
        """
        self.process_file(memory_data_filename, start_time_obj, end_time_obj, step, num_columns, time_entry_summary, request_data,
                        first_interval_unit, first_interval_rate, second_interval_unit)
    

    def process_file(self, file_name, start_time_obj, end_time_obj, step, num_columns, time_entry_summary, request_data,
                    first_interval_unit, first_interval_rate, second_interval_unit):
        """
        Processes a single memory statistics file to extract and aggregate data.
        Parameters:
        - file_name (str): The filename containing memory statistics data.
        - start_time_obj (datetime): The starting time object for filtering.
        - end_time_obj (datetime): The ending time object for filtering.
        - step (int): Step size for processing time slices.
        - num_columns (int): Number of columns for time slices.
        - time_entry_summary (dict): Summary structure to hold time group information.
        - request_data (dict): Request data containing time range information.
        - first_interval_unit (str): The first time interval unit.
        - first_interval_rate (int): The rate for the first interval.
        - second_interval_unit (str): The second time interval unit.
        """
        if not os.path.exists(file_name) or os.path.getsize(file_name) <= 0:
            return
        
        with gzip.open(file_name, 'rt', encoding='utf-8') as jfile:
            content = json.load(jfile)
            entries = content if isinstance(content, list) else [content]
            for memory_entry in entries:
                self.process_memory_entry(
                    memory_entry,
                    start_time_obj, 
                    end_time_obj, 
                    step, 
                    num_columns, 
                    time_entry_summary, 
                    request_data,
                    first_interval_unit, 
                    first_interval_rate, 
                    second_interval_unit
                )    

    def process_memory_entry(self, memory_entry, start_time_obj, end_time_obj, step, num_columns, time_entry_summary, request_data,
                             first_interval_unit, first_interval_rate, second_interval_unit):
        """
        Processes a single memory entry to determine its appropriate time slot for aggregation.

        Parameters:
        - memory_entry (dict): A dictionary representing a memory entry with a timestamp and associated data.
        - start_time_obj (datetime): The starting time object for filtering memory entries.
        - end_time_obj (datetime): The ending time object for filtering memory entries.
        - step (int): Step size for processing time slices.
        - num_columns (int): Number of columns for time slices in the summary.
        - time_entry_summary (dict): Summary structure to hold time group information.
        - request_data (dict): Request data containing time range information.
        - first_interval_unit (str): The first time interval unit for calculating slots.
        - first_interval_rate (int): The rate for the first interval.

        This method checks if the memory entry's creation time falls within the specified time range 
        and adds it to the appropriate time group in the summary.
        """
        rtime = datetime.fromisoformat(memory_entry['current_time'])
        if start_time_obj <= rtime <= end_time_obj:
            slot = self.get_time_slot_index(rtime, start_time_obj, step, first_interval_unit, second_interval_unit, first_interval_rate, request_data['duration'])
            if slot < num_columns:
                self.add_entry_to_time_group_list(time_entry_summary, slot, memory_entry)

    def aggregate_data(self, request_data, time_entry_summary, num_columns):
        """
        Aggregates memory data using optimized batch processing for better performance.

        Args:
            request_data (dict): Contains time range information for the request
            time_entry_summary (dict): Summary of aggregated memory statistics.
            num_columns (int): Number of columns (time slices) to process.
        """
        batch_size = max(1, min(num_columns // 4, 10))
        
        last_entry = {SYSTEM_MEMORY_KEY: {"system": {}}}
        
        time_group_list = time_entry_summary["time_group_list"]
        
        for start in range(0, num_columns, batch_size):
            end = min(start + batch_size, num_columns)
            
            try:
                processed_batch = [
                    self.entry_manager.get_global_entry_data(
                        request_data, 
                        time_entry_summary, 
                        last_entry, 
                        time_group_list[i]
                    ) for i in range(start, end)
                ]
                
                time_group_list[start:end] = processed_batch

            except (KeyError, IndexError, TypeError, ValueError) as e:
                logger.log_error(
                    f"Batch processing error: "
                    f"Columns {start}-{end}, "
                    f"Error: {e}",
                    extra={
                        'request_data': request_data,
                        'num_columns': num_columns,
                        'batch_start': start,
                        'batch_end': end
                    }
                )
                continue

        return time_group_list

    def generate_report(self, request_data, time_entry_summary, num_columns, step):
        """
        Generates a formatted report based on processed memory statistics.

        Parameters:
        - request_data (dict): Request data containing report generation parameters.
        - time_entry_summary (dict): Summary structure containing aggregated memory statistics.
        - num_columns (int): Number of columns for time slices.
        - step (int): Step size for processing time slices.

        Returns:
        - str: The formatted report as a string.
        """
        report_generator = MemoryReportGenerator(request_data, step)
        data = report_generator.get_memmory_statistics_report_header()
        data += self.entry_manager.get_memory_entry_data(request_data, time_entry_summary, num_columns)
        return data

    def get_time_slot_index(self, rtime, start_time_obj, step, first_interval_unit, second_interval_unit, first_interval_rate, period):
        """
        Determines the index of the time slot for a given timestamp based on the specified intervals.

        Parameters:
        - rtime (datetime): The timestamp of the memory entry.
        - start_time_obj (datetime): The starting time object for calculating the slot index.
        - step (int): Step size for processing time slices.
        - first_interval_unit (str): The first time interval unit for calculating the slot.
        - second_interval_unit (str): The second time interval unit for calculating the slot.
        - first_interval_rate (int): The rate for the first interval.
        - period (str): The duration type ('days', 'hours', etc.) that dictates how to calculate the slot.

        Returns:
        - int: The calculated slot index for the given timestamp.

        This method calculates the difference in time from the start time to the memory entry's time 
        and determines which slot the entry belongs to based on the defined intervals and step size.
        """
        delta = rtime - start_time_obj
        delta_dict = Utility.format_timedelta_as_dict(delta)

        if period == "days":
            diff = delta_dict[first_interval_unit] * first_interval_rate + int(round(delta_dict[second_interval_unit] / 24))
        else:
            diff = delta_dict[first_interval_unit] * first_interval_rate + delta_dict[second_interval_unit]

        return int(diff / step)

    def add_entry_to_time_group_list(self, time_entry_summary, slot, memory_entry):
        """
        Adds a memory entry to the specified time group in the summary.

        Parameters:
        - time_entry_summary (dict): Summary structure to hold time group information.
        - slot (int): The index of the time group to which the memory entry will be added.
        - memory_entry (dict): A dictionary representing a memory entry.

        This method adds the memory entry to the time group's list and increments the count of entries 
        in that group.
        """
        if time_entry_summary["time_group_list"][slot]['count'] == 0:
            time_entry_summary["time_group_list"][slot]['time_list'][0] = memory_entry
        else:
            time_entry_summary["time_group_list"][slot]['time_list'].append(memory_entry)
        
        time_entry_summary["time_group_list"][slot]['count'] += 1

    def generate_memory_statistics(self, memory_statistics_request):
        """
        Generates memory statistics based on the specified duration from the request.

        Parameters:
        - memory_statistics_request (dict): Request data containing duration and other parameters 
          for generating memory statistics.

        Returns:
        - str: The generated report of memory statistics.

        This method processes the memory statistics based on the specified duration ('days', 'hours', 
        or 'minutes') and calls the appropriate processing function for the required time slice.
        Raises:
        - ValueError: If the specified duration is invalid.
        """
        period = memory_statistics_request['duration']
        
        if period == "days":
            return self.process_memory_statistics_timeslice(
                memory_statistics_request, 
                self.memory_statistics_config['TOTAL_MEMORY_STATISTICS_LOG_FILENAME'], 
                "days", 1, 
                "hours", 60, 
                "minutes", 60, 
                "seconds"
            )
        elif period == "hours":
            return self.process_memory_statistics_timeslice(
                memory_statistics_request, 
                self.memory_statistics_config['MEMORY_STATISTICS_LOG_FILENAME'], 
                "days", 24, 
                "hours", 60, 
                "minutes", 60, 
                "seconds"
            )
        elif period == "minutes":
            return self.process_memory_statistics_timeslice(
                memory_statistics_request, 
                self.memory_statistics_config['MEMORY_STATISTICS_LOG_FILENAME'], 
                "hours", 60, 
                "minutes", 60, 
                "seconds", 60,
                "seconds"
            )
        else:
            raise ValueError(f"Invalid period: {period}")

    def calculate_memory_statistics_period(self, memory_statistics_request):
        """
        Determines the duration of memory statistics collection based on the provided time range. 
        Updates the request with the appropriate duration unit (days, hours, or minutes).
        :param memory_statistics_request: Dictionary containing 'from', 'to', and 'time_data' 
                                        with calculated time metrics.
        :raises ValueError: If the time interval is zero.
        :return: Updated memory_statistics_request with the calculated 'duration
        """
        time_info = memory_statistics_request['time_data']
        from_time = memory_statistics_request['from']
        to_time = memory_statistics_request['to']

        if time_info['days'] == 0 and time_info['hours'] == 0 and time_info['minutes'] == 0:
            raise ValueError(f"Invalid time interval for start: {from_time}, end: {to_time}, "
                             f"Days: {time_info['days']}, Hours: {time_info['hours']}, Minutes: {time_info['minutes']}")

        if time_info['total_hours'] >= 48:
            memory_statistics_request['duration'] = "days"
        elif time_info['total_minutes'] >= 120:
            memory_statistics_request['duration'] = "hours"
        else:
            memory_statistics_request['duration'] = "minutes"

        return self.generate_memory_statistics(memory_statistics_request)


class SocketHandler:
    """Handles the creation and management of a UNIX socket for communication.
    
    This class is responsible for setting up a UNIX socket, accepting incoming
    connections, processing requests, and sending responses. It provides methods
    for managing socket file cleanup and error handling during communication.
    """

    def __init__(self, address, command_handler, stop_event, timeout=1.0, backlog=5, buffer_size=4096, max_retries=10):
        """
        Initializes the SocketHandler with the specified parameters.
        
        :param address: The file system path where the UNIX socket will be created.
        :param command_handler: A callable that processes commands received from clients.
        :param stop_event: An event flag used to signal when to stop the socket listener.
        :param timeout: Socket timeout for accept operations (seconds). Default: 1.0.
        :param backlog: Maximum number of queued connections. Default: 5.
        :param buffer_size: Buffer size for receiving requests. Default: 4096.
        :param max_retries: Maximum retries for persistent errors in accept loop. Default: 10.
        """
        self.address = address
        self.command_handler = command_handler
        self.listener_socket = None
        self.stop_event = stop_event
        self.timeout = timeout
        self.backlog = backlog
        self.buffer_size = buffer_size
        self.max_retries = max_retries

    def safe_remove_file(self, filepath):
        """Removes a file if it exists to prevent socket binding errors.
        
        :param filepath: The path of the socket file to be removed.
        """
        try:
            if os.path.exists(filepath):
                os.remove(filepath)
                logger.log_info(f"Removed existing socket file: {filepath}")
        except OSError as e:
            logger.log_error(f"Failed to remove file {filepath}: {type(e).__name__}: {e}")

    def cleanup(self):
        """Cleans up socket resources, including the socket file and listener socket."""
        if self.listener_socket:
            try:
                self.listener_socket.close()
                logger.log_info("Listener socket closed.")
            except socket.error as e:
                logger.log_error(f"Error closing listener socket: {type(e).__name__}: {e}")
            self.listener_socket = None
        self.safe_remove_file(self.address)

    def create_unix_socket(self):
        """Creates and configures a UNIX socket for listening for incoming connections.
        
        Raises:
            socket.error: If socket creation or binding fails.
            OSError: If permission or ownership changes fail.
        """ 
        try:
            self.listener_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.listener_socket.settimeout(self.timeout)
            self.listener_socket.bind(self.address)
            os.chmod(self.address, 0o600)
            os.chown(self.address, os.getuid(), -1)
            self.listener_socket.listen(self.backlog)
            logger.log_info(f"UNIX socket created and listening at {self.address}")
        except (socket.error, OSError) as e:
            logger.log_error(f"Failed to create UNIX socket at {self.address}: {type(e).__name__}: {e}")
            raise

    def send_response(self, connection, response_data):
        """Sends a JSON response back to the client.
        
        :param connection: The socket connection to the client.
        :param response_data: The data to be sent as a JSON response.
        """
        try:
            response_json = json.dumps(response_data)
            connection.sendall(response_json.encode('utf-8'))
            logger.log_debug(f"Sent response: {response_json}")
        except (json.JSONEncodeError, socket.error) as e:
            logger.log_error(f"Failed to send response: {type(e).__name__}: {e}")

    def handle_connection(self, connection):
        """Processes a single incoming socket connection.
        
        :param connection: The socket connection established with the client.
        """
        error_response = {"status": False, "msg": None}
        try:
            request_data = connection.recv(self.buffer_size)
            if not request_data:
                logger.log_warning("Received empty request")
                return

            request_json = json.loads(request_data.decode('utf-8'))
            logger.log_debug(f"Received request: {request_json}")
            command_name = request_json['command']
            command_data = request_json.get('data', {})

            response = self.command_handler(command_name, command_data)

            self.send_response(connection, response)
        except (json.JSONDecodeError, KeyError, ValueError) as error:
            logger.log_error(f"Error handling request: {type(error).__name__}: {error}")
            logger.log_debug(f"Request error traceback: {traceback.format_exc()}")
            error_response['msg'] = str(error)
            self.send_response(connection, error_response)
        finally:
            try:
                connection.close()
                logger.log_debug("Connection closed")
            except socket.error as e:
                logger.log_error(f"Error closing connection: {type(e).__name__}: {e}")

    def stop(self):
        """Stops the socket listener loop and closes the socket.
        
        This method sets the stop event and closes the listener socket, ensuring
        a graceful shutdown.
        """
        logger.log_info("Stopping socket handler.")
        self.stop_event.set()
        self.cleanup()

    def stop_accepting(self):
        """Alias for stop to maintain compatibility with external calls."""
        self.stop()

    def start_listening(self):
        """Starts listening for incoming socket connections.
        
        This method initializes the socket and enters a loop to accept and handle
        incoming connections until the stop_event is set. It ensures cleanup on exit.
        """
        try:
            self.create_unix_socket()
            retry_count = 0
            backoff = 0.1

            while not self.stop_event.is_set():
                try:
                    connection, client_address = self.listener_socket.accept()
                    logger.log_info("Accepted new connection")
                    self.handle_connection(connection)
                    retry_count = 0
                    backoff = 0.1
                except socket.timeout:
                    logger.log_debug("Socket timeout occurred while waiting for a connection.")
                    continue
                except OSError as e:
                    if self.stop_event.is_set():
                        logger.log_info("Socket listener stopped as requested.")
                        break
                    else:
                        logger.log_error(f"Socket error: {type(e).__name__}: {e}")
                        retry_count += 1
                        if retry_count >= self.max_retries:
                            logger.log_error("Max retries reached for socket errors.")
                            raise RuntimeError("Persistent socket errors exceeded max retries")
                        time.sleep(backoff)
                        backoff = min(backoff * 2, 1.0)
                except (socket.error, RuntimeError) as error:
                    logger.log_error(f"Unexpected error: {type(error).__name__}: {error}")
                    retry_count += 1
                    if retry_count >= self.max_retries:
                        logger.log_error("Max retries reached for unexpected errors.")
                        raise RuntimeError("Persistent errors exceeded max retries")
                    time.sleep(backoff)
                    backoff = min(backoff * 2, 1.0)
        finally:
            self.cleanup()
            logger.log_info("Socket listener stopped.")

class Daemonizer:
    """Facilitates the daemonization of the current process.
    
    This class provides methods to fork the process into the background,
    manage the process ID (PID), and redirect standard file descriptors
    to /dev/null, ensuring that the daemon operates independently from
    the terminal.
    """

    def __init__(self, pid_file: str) -> None:
        """
        Initializes the Daemonizer with the specified PID file location.
        
        Args:
            pid_file (str): The file path where the daemon's PID will be stored.

        Raises:
            TypeError: If pid_file is not a string. Logged with input type.
        """
        logger.log_debug(f"Entering Daemonizer.__init__: pid_file={pid_file}")
        if not isinstance(pid_file, str):
            logger.log_error(f"Daemonizer.__init__: Invalid pid_file type: {type(pid_file)}")
            raise TypeError("pid_file must be a string")

        self.pid_file = pid_file
        logger.log_debug("Daemonizer initialized successfully")

    def daemonize(self) -> None:
        """Forks the process to run as a background daemon.
        
        This method performs the necessary steps to create a daemon process,
        including forking twice and creating a new session. It logs the
        success of the daemonization and writes the PID to a file.
        
        Raises:
            OSError: If forking or session creation fails. Logged with error details.
            RuntimeError: If daemonization validation, PID writing, or descriptor redirection fails. Logged with context.
        """
        logger.log_debug("Entering daemonize")
        try:
            pid = os.fork()
            if pid > 0:
                logger.log_debug(f"daemonize: First fork successful, parent exiting with pid={pid}")
                sys.exit(0)
        except OSError as e:
            logger.log_error(f"daemonize: First fork failed: {e}")
            raise RuntimeError(f"First fork failed: {e}")

        logger.log_debug("daemonize: First fork child, changing directory and session")
        os.chdir("/")  
        os.umask(0)   
        try:
            os.setsid()   
            logger.log_debug(f"daemonize: New session created, sid={os.getsid(0)}")
        except OSError as e:
            logger.log_error(f"daemonize: Session creation failed: {e}")
            raise RuntimeError(f"Session creation failed: {e}")

        try:
            pid = os.fork()
            if pid > 0:
                logger.log_debug(f"daemonize: Second fork successful, parent exiting with pid={pid}")
                sys.exit(0)
        except OSError as e:
            logger.log_error(f"daemonize: Second fork failed: {e}")
            raise RuntimeError(f"Second fork failed: {e}")

        logger.log_debug("daemonize: Second fork child, validating daemonization")
        self._validate_daemonization()

        logger.log_debug("daemonize: Writing PID to file")
        self.write_pid_to_file()

        logger.log_debug("daemonize: Redirecting standard file descriptors")
        self.redirect_standard_file_descriptors()
        logger.log_info(f"daemonize: Daemonization completed successfully, pid={os.getpid()}")

    def _validate_daemonization(self) -> None:
        """
        Performs additional validation to confirm successful daemonization.
        
        Checks:
        - Process is not a process group leader
        - Working directory has been changed
        - Session ID is different from parent
        
        Raises:
            RuntimeError: If validation checks fail. Logged with specific failure details.
            OSError: If system calls (getpid, getsid, getcwd) fail. Logged with error details.
        """
        logger.log_debug(f"Entering _validate_daemonization: pid={os.getpid()}")
        try:
            if os.getpid() == os.getpgrp():
                logger.log_error("_validate_daemonization: Process is still a process group leader")
                raise RuntimeError("Failed to become session leader")

            if os.getcwd() != '/':
                logger.log_error(f"_validate_daemonization: Working directory is {os.getcwd()}, expected /")
                raise RuntimeError("Working directory not changed to root")

            current_sid = os.getsid(0)
            if current_sid == -1:
                logger.log_error("_validate_daemonization: Failed to retrieve session ID")
                raise RuntimeError("Could not get session ID")

            logger.log_info(f"_validate_daemonization: Daemonization validation successful. PID: {os.getpid()}, Session ID: {current_sid}")
        except OSError as e:
            logger.log_error(f"_validate_daemonization: System call error during validation: {e}")
            raise RuntimeError(f"Daemonization validation failed: {e}")

    def write_pid_to_file(self) -> None:
        """Writes the daemon's PID to the specified file for management purposes.
        
        This method ensures that the PID of the running daemon is stored in a
        file, which can be used later to manage the daemon process (e.g., for
        stopping it). It logs the action taken and handles any errors.
        
        Raises:
            OSError: If directory creation or file operations fail. Logged with pid_file and error details.
            ValueError: If PID read from file is invalid. Logged with pid_file and PID value.
            RuntimeError: If PID mismatch occurs. Logged with pid_file and PID details.
        """
        logger.log_debug(f"Entering write_pid_to_file: pid_file={self.pid_file}")
        try:
            pid_dir = os.path.dirname(self.pid_file)
            os.makedirs(pid_dir, exist_ok=True)
            logger.log_debug(f"write_pid_to_file: Ensured directory exists: {pid_dir}")

            with open(self.pid_file, 'w') as f:
                f.write(f"{os.getpid()}\n")
            logger.log_debug(f"write_pid_to_file: Wrote PID {os.getpid()} to {self.pid_file}")

            with open(self.pid_file, 'r') as f:
                pid_from_file = int(f.read().strip())
                if pid_from_file != os.getpid():
                    logger.log_error(f"write_pid_to_file: PID mismatch in {self.pid_file}: expected {os.getpid()}, got {pid_from_file}")
                    raise RuntimeError("PID mismatch in PID file")
        except OSError as e:
            logger.log_error(f"write_pid_to_file: Failed to write PID file {self.pid_file}: {e}")
            raise RuntimeError(f"Failed to write PID file: {e}")
        except ValueError as e:
            logger.log_error(f"write_pid_to_file: Invalid PID read from {self.pid_file}: {e}")
            raise RuntimeError(f"Invalid PID in file: {e}")

    def redirect_standard_file_descriptors(self) -> None:
        """Redirects standard file descriptors to /dev/null.
        
        This method ensures that the daemon does not receive any terminal input/output
        by redirecting stdin, stdout, and stderr to /dev/null. It logs the action
        taken and any errors encountered during the process.
        
        Raises:
            OSError: If file operations or descriptor redirection fail. Logged with error details.
            RuntimeError: If descriptor validation fails. Logged with validation details.
        """
        logger.log_debug("Entering redirect_standard_file_descriptors")
        try:
            sys.stdout.flush()
            sys.stderr.flush()

            with open(os.devnull, 'r') as stdin_null, \
                 open(os.devnull, 'a+') as stdout_stderr_null:
                os.dup2(stdin_null.fileno(), sys.stdin.fileno())
                os.dup2(stdout_stderr_null.fileno(), sys.stdout.fileno())
                os.dup2(stdout_stderr_null.fileno(), sys.stderr.fileno())
                logger.log_debug("redirect_standard_file_descriptors: Descriptors redirected to /dev/null")

            self._validate_file_descriptors()
            logger.log_debug("redirect_standard_file_descriptors: Standard file descriptors redirected and validated")
        except OSError as e:
            logger.log_error(f"redirect_standard_file_descriptors: Failed to redirect standard file descriptors: {e}")
            raise RuntimeError(f"Failed to redirect file descriptors: {e}")

    def _validate_file_descriptors(self) -> None:
        """
        Validates that file descriptors have been correctly redirected.
        
        Raises:
            OSError: If stat or file descriptor operations fail. Logged with error details.
            RuntimeError: If descriptors are not correctly redirected. Logged with stat details.
        """
        logger.log_debug(f"Entering _validate_file_descriptors: stdin_fd={sys.stdin.fileno()}, "
                         f"stdout_fd={sys.stdout.fileno()}, stderr_fd={sys.stderr.fileno()}")
        try:
            stdin_stat = os.fstat(sys.stdin.fileno())
            stdout_stat = os.fstat(sys.stdout.fileno())
            stderr_stat = os.fstat(sys.stderr.fileno())
            
            devnull_stat = os.stat(os.devnull)

            if not (stdin_stat.st_ino == stdout_stat.st_ino == stderr_stat.st_ino == devnull_stat.st_ino and
                    stdin_stat.st_dev == stdout_stat.st_dev == stderr_stat.st_dev == devnull_stat.st_dev):
                logger.log_error(f"_validate_file_descriptors: Descriptors not redirected to /dev/null: "
                                 f"stdin_ino={stdin_stat.st_ino}, stdout_ino={stdout_stat.st_ino}, "
                                 f"stderr_ino={stderr_stat.st_ino}, devnull_ino={devnull_stat.st_ino}")
                raise RuntimeError("File descriptors not correctly redirected")
            logger.log_debug("_validate_file_descriptors: File descriptor validation successful")
        except OSError as e:
            logger.log_error(f"_validate_file_descriptors: System call error during validation: {e}")
            raise RuntimeError(f"File descriptor validation failed: {e}")

class ThreadSafeConfig:
    """
    A thread-safe configuration manager that uses a read-write lock 
    to allow multiple concurrent reads but exclusive writes.
    """
    def __init__(self, initial_config):
        self._config = initial_config
        self._lock = threading.RLock() 
    
    def get(self, key, default=None):
        """
        Safely retrieve a configuration value.
        """
        with self._lock:
            return self._config.get(key, default)
    
    def update(self, updates):
        """
        Safely update configuration with new values.
        """
        with self._lock:
            self._config.update(updates)
    
    def get_copy(self):
        """
        Get a thread-safe copy of the current configuration.
        """
        with self._lock:
            return self._config.copy()

class MemoryStatisticsService:
    """
    Manages the Memory Statistics Service, responsible for collecting,
    processing, and serving memory usage statistics in a daemonized manner.
    This service utilizes a socket for communication and handles
    commands for memory statistics retrieval, while also managing
    configuration reloading and graceful shutdown procedures.
    """
    def __init__(self, memory_statistics_config, config_file_path='./etc/memory_statistics.conf', name="MemoryStatisticsService"):
        """
        Initializes the MemoryStatisticsService instance.
        Parameters:
        - memory_statistics_config (dict): Initial configuration settings for the service.
        - config_file_path (str): Path to the configuration file to load overrides.
        """
        self.config = ThreadSafeConfig(memory_statistics_config)
        self.name = name
        logger.log_info(f"Service initialized with name: {self.name}")

        self.config_file_path = config_file_path
        self.memory_statistics_lock = threading.Lock()
        self.stop_event = threading.Event()

        self.config.update(memory_statistics_config)
        self.load_config_from_file()
        
        self.socket_listener_thread = None
        self.memory_collection_thread = None

        self.sampling_interval = int(self.config.get('sampling_interval', 5)) * 60
        self.retention_period = int(self.config.get('retention_period', 15))

        self.socket_handler = SocketHandler(
            address=self.config.get('DBUS_SOCKET_ADDRESS'),
            command_handler=self.handle_command,
            stop_event=self.stop_event
        )
        self.daemonizer = Daemonizer('/var/run/memory_statistics_daemon.pid')

        signal.signal(signal.SIGHUP, self.handle_sighup)
        signal.signal(signal.SIGTERM, self.handle_sigterm)

    def load_config_from_file(self):
        """
        Loads and applies configuration values from the configuration file.
        
        Raises:
            configparser.Error: If parsing the configuration file fails.
            OSError: If reading the file fails.
        """
        parser = configparser.ConfigParser()
        try:
            if os.path.exists(self.config_file_path):
                parser.read(self.config_file_path)

                updates = {
                    'sampling_interval': parser.getint('default', 'sampling_interval', fallback=self.config.get('sampling_interval')),
                    'retention_period': parser.getint('default', 'retention_period', fallback=self.config.get('retention_period')),
                }
                
                self.config.update(updates)
                
                logger.log_info(f"Configuration loaded from file: "
                                f"sampling_interval={updates['sampling_interval']}, "
                                f"retention_period={updates['retention_period']}")
            else:
                logger.log_warning(f"Configuration file not found at {self.config_file_path}. Proceeding with default settings.")
        except (configparser.Error, OSError) as e:
            logger.log_error(f"Configuration loading error: {e}")
            raise

    def handle_command(self, command_name, command_data):
        """
        Processes incoming commands received via the socket.
        Parameters:
        - command_name (str): The name of the command to handle.
        - command_data (dict): Data associated with the command.
        Returns:
        - dict: Response indicating the success or failure of command execution.
        """
        if hasattr(self, command_name):
            command_method = getattr(self, command_name)
            return command_method(command_data)
        else:
            logger.log_warning(f"Unknown command received: {command_name}")
            return {"status": False, "msg": "Unknown command"}

    def handle_sighup(self, signum, frame):
        """
        Responds to the SIGHUP signal to reload the service configuration.
        This method performs cleanup of old log files and updates the service's
        runtime configuration from the ConfigDB.
        """
        logger.log_info("Received SIGHUP, reloading configuration.")
        try:
            self.cleanup_old_files()
            self.load_config_from_db()
        except (OSError, RuntimeError) as e:
            logger.log_error(f"Error handling SIGHUP: {e}")

    def terminate_child_processes(self, timeout: float = 5.0) -> None:
        """
        Gracefully terminates all child processes associated with the service.
        Sends SIGTERM to child processes and waits for them to exit within the specified
        timeout. If any processes remain running, sends SIGKILL to force termination.
        Logs the progress and any errors encountered during termination.

        Args:
            timeout (float, optional): Timeout in seconds to wait for child processes to
                terminate gracefully. Defaults to 5.0 seconds.

        Raises:
            ValueError: If the timeout is invalid (e.g., negative or non-numeric).
            psutil.Error: If process iteration or access fails.
            OSError: If signal sending fails.
        """
        if not isinstance(timeout, (int, float)):
            logger.log_error(f"Invalid timeout type: {type(timeout)}. Using default of 5.0 seconds.")
            timeout = 5.0
        if timeout < 0:
            logger.log_error(f"Negative timeout: {timeout}. Using default of 5.0 seconds.")
            timeout = 5.0
        if timeout > 60.0:
            logger.log_warning(f"Timeout {timeout} seconds is large; capping at 60.0 seconds.")
            timeout = 60.0

        logger.log_info(f"Terminating child processes with timeout {timeout} seconds.")

        current_pid = os.getpid()
        failed_processes = []

        try:
            children = [
                proc for proc in psutil.process_iter(['pid', 'ppid', 'name'])
                if proc.info['ppid'] == current_pid
            ]

            if not children:
                logger.log_info("No child processes found.")
                return

            for child in children:
                try:
                    pid = child.info['pid']
                    logger.log_info(f"Sending SIGTERM to child process {pid} ({child.info['name']})")
                    os.kill(pid, signal.SIGTERM)
                except (ProcessLookupError, OSError) as e:
                    logger.log_warning(f"Failed to send SIGTERM to child process {pid}: {e}")
                    continue

            start_time = time.time()
            while children and time.time() - start_time < timeout:
                children = [
                    child for child in children
                    if child.is_running() and child.info['ppid'] == current_pid
                ]
                if not children:
                    break
                time.sleep(0.1)

            for child in children:
                if child.is_running():
                    pid = child.info['pid']
                    logger.log_warning(f"Child process {pid} ({child.info['name']}) did not terminate within {timeout} seconds. Sending SIGKILL.")
                    try:
                        os.kill(pid, signal.SIGKILL)
                        failed_processes.append(f"{pid} ({child.info['name']})")
                    except (ProcessLookupError, OSError) as e:
                        logger.log_error(f"Failed to send SIGKILL to child process {pid}: {e}")

            if failed_processes:
                logger.log_error(f"Some child processes failed to terminate gracefully: {', '.join(failed_processes)}")
            else:
                logger.log_info("All child processes terminated successfully.")

        except psutil.Error as e:
            logger.log_error(f"Error iterating or accessing processes: {e}")
            raise
        except OSError as e:
            logger.log_error(f"Error during child process termination: {e}")
            raise

    def handle_sigterm(self, signum, frame):
        """
        Handles the SIGTERM signal for graceful shutdown of the service.
        This method attempts to terminate child processes, stop running threads,
        and perform necessary cleanup operations before exiting.
        """
        logger.log_info("Received SIGTERM, initiating graceful shutdown...")
        try:
            if hasattr(self.socket_handler, 'stop_accepting'):
                self.socket_handler.stop_accepting()

            self.terminate_child_processes(timeout=5.0)
            self.stop_threads(timeout=5.0)

            self.cleanup()

            logger.log_info("Shutdown complete. Exiting...")
            sys.exit(0)
        except (OSError, psutil.Error, RuntimeError) as e:
            logger.log_error(f"Error during graceful shutdown: {e}")
            try:
                os.killpg(os.getpgid(0), signal.SIGKILL)
            except OSError as kill_error:
                logger.log_error(f"Error during force kill: {kill_error}")
            sys.exit(1)

    def load_config_from_db(self):
        """
        Retrieves runtime configuration values from ConfigDB with enhanced safety.
        
        Raises:
            RuntimeError: If ConfigDB connection or query fails.
        """
        logger.log_info("Starting configuration retrieval from ConfigDB")
        config_db = ConfigDBConnector()

        try:
            config_db.connect()
            config = config_db.get_table('MEMORY_STATISTICS')

            updates = {}

            sampling_interval = config.get('sampling_interval', 5)
            try:
                sampling_interval = int(sampling_interval)
                if 3 <= sampling_interval <= 15:
                    updates['sampling_interval'] = sampling_interval
                    logger.log_info(f"Validated sampling interval: {sampling_interval} minutes")
                else:
                    raise ValueError(f"Sampling interval out of range: {sampling_interval}")
            except (ValueError, TypeError) as interval_error:
                logger.log_warning(f"Invalid sampling interval: {interval_error}. Using default.")
                updates['sampling_interval'] = 5

            retention_period = config.get('retention_period', 15)
            try:
                retention_period = int(retention_period)
                if 1 <= retention_period <= 30:
                    updates['retention_period'] = retention_period
                    logger.log_info(f"Validated retention period: {retention_period} days")
                else:
                    raise ValueError(f"Retention period out of range: {retention_period}")
            except (ValueError, TypeError) as retention_error:
                logger.log_warning(f"Invalid retention period: {retention_error}. Using default.")
                updates['retention_period'] = 15

            self.config.update(updates)

            self.sampling_interval = updates.get('sampling_interval', 5) * 60
            self.retention_period = updates.get('retention_period', 15)

            logger.log_info(f"Configuration updated: "
                            f"sampling_interval={self.sampling_interval // 60} minutes, "
                            f"retention_period={self.retention_period} days")
        except RuntimeError as error:
            logger.log_error(f"Configuration retrieval failed: {error}")
            self.sampling_interval = 5 * 60
            self.retention_period = 15
        finally:
            try:
                config_db.disconnect()
            except OSError as disconnect_error:
                logger.log_error(f"Error closing ConfigDB connection: {disconnect_error}")

    def cleanup_old_files(self):
        """
        Deletes old log files from the log directory.
        
        Raises:
            OSError: If listing or deleting files fails.
        """
        try:
            log_directory = self.config.get('LOG_DIRECTORY', '/var/log/memory_statistics')
            for file in os.listdir(log_directory):
                if file.endswith('.gz'):
                    file_path = os.path.join(log_directory, file)
                    os.remove(file_path)
                    logger.log_info(f"Deleted old log file: {file_path}")
        except OSError as e:
            logger.log_error(f"Error during log file cleanup: {e}")

    def memory_statistics_command_request_handler(self, request):
        """
        Thread-safe handler for memory statistics requests.
        
        Raises:
            json.JSONDecodeError: If JSON parsing fails.
            ValueError: If configuration or request data is invalid.
            RuntimeError: If collector or processor fails.
        """
        try:
            logger.log_info(f"Received memory statistics request: {request}")

            current_config = self.config.get_copy()
            sampling_interval = int(current_config.get('sampling_interval', 5)) * 60
            retention_period = int(current_config.get('retention_period', 15))

            with self.memory_statistics_lock:
                memory_collector = MemoryStatisticsCollector(
                    sampling_interval=sampling_interval // 60,
                    retention_period=retention_period
                )
                current_memory = memory_collector.collect_and_store_memory_usage(collect_only=True)
                request['current_memory'] = current_memory
                logger.log_info(f"Current memory usage collected: {current_memory}")

                time_processor = TimeProcessor(
                    sampling_interval=sampling_interval // 60,
                    retention_period=retention_period
                )
                time_processor.process_time_information(request)

                processor = MemoryStatisticsProcessor(
                    current_config,
                    sampling_interval=sampling_interval,
                    retention_period=retention_period
                )
                report = processor.calculate_memory_statistics_period(request)
                logger.log_info(f"Memory statistics processed: {report}")

            return {"status": True, "data": report}
        except json.JSONDecodeError as je:
            logger.log_error(f"JSON decoding error in memory statistics request: {je}")
            return {"status": False, "error": f"Invalid JSON format: {str(je)}"}
        except (ValueError, RuntimeError) as error:
            logger.log_error(f"Error handling memory statistics request: {error}")
            return {"status": False, "error": str(error)}

    def start_socket_listener(self):
        """
        Starts the socket listener in a separate thread.
        This method initializes and starts the socket listener, allowing the 
        service to accept incoming commands. The listener operates in a daemon 
        thread, ensuring that it runs in the background and does not block 
        the main program from exiting.
        Logs the start of the socket listener thread.
        """
        self.socket_listener_thread = threading.Thread(
            target=self.socket_handler.start_listening,
            name='SocketListener',
            daemon=True
        )
        self.socket_listener_thread.start()
        logger.log_info("Socket listener thread started.")

    def memory_collection(self):
        """
        Collects memory statistics at intervals defined by `sampling_interval`. 
        The collection process continues until the `stop_event` is set. Each cycle 
        collects data, handles errors, and ensures the next collection happens 
        after the remaining time in the sampling interval.
        """
        logger.log_info("Memory statistics collection thread started.")
        while not self.stop_event.is_set():
            start_time = datetime.now()
            try:
                with self.memory_statistics_lock:
                    memory_collector = MemoryStatisticsCollector(
                        sampling_interval=self.sampling_interval // 60,
                        retention_period=self.retention_period
                    )
                    memory_collector.collect_and_store_memory_usage(collect_only=False)
            except json.JSONDecodeError as je:
                logger.log_error(f"JSON encoding/decoding error during collection: {je}")
            except RuntimeError as error:
                logger.log_error(f"Error during memory statistics collection: {error}")

            elapsed_time = (datetime.now() - start_time).total_seconds()
            sleep_time = max(0, self.sampling_interval - elapsed_time)
            if self.stop_event.wait(timeout=sleep_time):
                break 
        logger.log_info("Memory statistics collection thread stopped.")

    def start_memory_collection(self):
        """
        Starts memory statistics collection in a separate thread.
        This method initializes and starts a separate thread dedicated to 
        collecting memory statistics at defined intervals. The collection 
        process runs in a loop until signaled to stop, capturing memory usage 
        data and logging any errors that may occur during the process.
        The collection interval is determined by the `sampling_interval` 
        configuration. Logs the start and stop of the memory collection thread.
        """ 
        self.memory_collection_thread = threading.Thread(
            target=self.memory_collection,
            name='MemoryCollection',
            daemon=True
        )
        self.memory_collection_thread.start()
        logger.log_info("Memory collection thread started.")

    def stop_threads(self, timeout: float = 5.0) -> None:
        """
        Signals threads to stop and waits for them to exit gracefully.
        This method sets the stop event to signal all running threads to terminate,
        closes the listener socket to unblock the accept method, and waits for the
        socket listener and memory collection threads to finish within a specified
        timeout. Logs issues encountered during the process.

        Args:
            timeout (float, optional): Timeout in seconds for thread termination.
                Defaults to 5.0 seconds.

        Raises:
            ValueError: If the timeout is invalid (e.g., negative or non-numeric).
        """
        if not isinstance(timeout, (int, float)):
            logger.log_error(f"Invalid timeout type: {type(timeout)}. Using default of 5.0 seconds.")
            timeout = 5.0
        if timeout < 0:
            logger.log_error(f"Negative timeout: {timeout}. Using default of 5.0 seconds.")
            timeout = 5.0
        if timeout > 60.0:
            logger.log_warning(f"Timeout {timeout} seconds is large; capping at 60.0 seconds.")
            timeout = 60.0

        logger.log_info(f"Signaling threads to stop with timeout {timeout} seconds.")
        self.stop_event.set()

        failed_threads = []

        for thread in [self.socket_listener_thread, self.memory_collection_thread]:
            if thread and thread.is_alive():
                logger.log_info(f"Waiting for {thread.name} to stop...")
                thread.join(timeout=timeout)
                if thread.is_alive():
                    logger.log_warning(f"{thread.name} did not terminate within {timeout} seconds.")
                    failed_threads.append(thread.name)
                else:
                    logger.log_info(f"{thread.name} terminated successfully.")

        self.socket_listener_thread = None
        self.memory_collection_thread = None

        if failed_threads:
            logger.log_error(
                f"Threads failed to terminate: {', '.join(failed_threads)}. "
                "Consider using multiprocessing for forceful termination or increasing the timeout."
            )
        else:
            logger.log_info("All threads stopped successfully.")

    def cleanup(self):
        """
        Performs additional cleanup tasks before service shutdown.
        This method handles the cleanup of old log files, removes the socket 
        and PID files, and flushes all log handlers to ensure that all logs 
        are properly written out. This is typically called during service 
        termination to free up resources and maintain a clean state.
        Logs the completion of the cleanup tasks and any errors encountered 
        during the process.
        
        Raises:
            OSError: If file operations fail.
            AttributeError: If log handler access fails.
        """
        self.cleanup_old_files()

        try:
            socket_address = self.config.get('DBUS_SOCKET_ADDRESS')
            if socket_address and os.path.exists(socket_address):
                os.unlink(socket_address)
                logger.log_info(f"Removed socket file: {socket_address}")
        except (FileNotFoundError, OSError) as e:
            logger.log_error(f"Error removing socket file: {e}")

        pid_file = '/var/run/memory_statistics_daemon.pid'
        try:
            if os.path.exists(pid_file):
                os.unlink(pid_file)
                logger.log_info(f"Removed PID file: {pid_file}")
        except (FileNotFoundError, OSError) as e:
            logger.log_error(f"Error removing PID file: {e}")

        try:
            for handler in logging.getLogger().handlers:
                handler.flush()
        except (AttributeError, OSError) as e:
            logger.log_error(f"Error flushing log handlers: {e}")

        logger.log_info("Cleanup complete.")

    def run(self):
        """
        Runs the Memory Statistics Service.
        This method starts the service by daemonizing the process and 
        initializing the threads necessary for socket listening and memory 
        statistics collection. The service will continue to run until 
        signaled to stop, during which it sleeps to reduce CPU usage.
        Logs the initialization and starting of the service.
        
        Raises:
            OSError: If daemonization or socket initialization fails.
            RuntimeError: If daemonization or service components fail to start.
        """
        logger.log_info(f"{self.name} is starting...")
        try:
            self.daemonizer.daemonize()
            self.start_socket_listener()
            self.start_memory_collection()
            while not self.stop_event.is_set():
                time.sleep(1)
        except (OSError, RuntimeError) as e:
            logger.log_error(f"Service failed to start: {e}")
            raise

if __name__ == '__main__':
    memory_statistics_config = {
        'sampling_interval': 5, 
        'retention_period': 15,
        'LOG_DIRECTORY': "/var/log/memory_statistics",
        'MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/memory-stats.json.gz",
        'TOTAL_MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/total-memory-stats.json.gz",
        'DBUS_SOCKET_ADDRESS': '/var/run/dbus/memstats.socket'
    }

    logger = SyslogLogger(identifier="memory_statistics", log_to_console=False)

    service_name = "MemoryStatisticsService" 
    service = MemoryStatisticsService(memory_statistics_config, name=service_name)
    service.run()
