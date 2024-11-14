#!/usr/bin/env python3

import psutil
import os
import signal
import subprocess
import sys
import time
# import pickle
import traceback
import logging
import gzip
import dateparser
import json
import getopt
import argparse
import threading
import configparser
import signal
import socket
import time
import syslog
import re
import math
import copy
from collections import Counter
from datetime import datetime
from datetime import timedelta
from datetime import datetime, timedelta
import fcntl
import sys, traceback
import stat
import os
import sys
import time
import threading
import socket
import os
import stat
import json
import time
import traceback
import argparse
from datetime import timedelta, datetime
from typing import Dict, Any
import dateparser
import re
from datetime import datetime
from swsscommon.swsscommon import ConfigDBConnector
import os
import sys
import signal
import time
import threading
import socket
import json
import configparser
import stat
import traceback
from datetime import datetime
import threading
import os
import signal
import time
import configparser
from datetime import datetime


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
        Initializes the logger with a syslog identifier and optional console logging.
        :param identifier: A string that identifies the syslog entries.
        :param log_to_console: A boolean indicating whether to also print log messages to the console.
        """
        self.syslog_identifier = identifier
        self.log_to_console = log_to_console

        syslog.openlog(ident=self.syslog_identifier, logoption=syslog.LOG_PID, facility=syslog.LOG_DAEMON)

    def log(self, level, message):
        """
        Logs a message to syslog and optionally to the console.
        :param level: The severity level (e.g., syslog.LOG_ERR, syslog.LOG_INFO, etc.).
        :param message: The log message to be recorded.
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

    def close_logger(self):
        """
        Closes the syslog connection.
        """
        syslog.closelog()    


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

    def _validate_time_ranges(self, start_time: datetime, end_time: datetime, current_time: datetime, time_diff: timedelta) -> None:
        """
        Validates the provided time ranges to ensure they adhere to defined constraints.

        :param start_time: The start time of the data collection period.
        :param end_time: The end time of the data collection period.
        :param current_time: The current time for comparison.
        :param time_difference: The difference between end_time and start_time.
        """
        time_diff_obj = Utility.convert_timedelta_to_obj(time_diff)
        
        if end_time > current_time:
            raise Exception("Datetime format error, 'to' time should not be greater than current time.")
        elif time_diff_obj.days > self.retention_period:
            raise Exception(f"Datetime format error, time range should not exceed {self.retention_period} days.")
        elif time_diff_obj.days == 0 and time_diff_obj.hours == 0 and time_diff_obj.minutes < self.sampling_interval:
            raise Exception(f"Datetime format error, time difference should be at least {self.sampling_interval} minutes.")

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
    def __init__(self, request, step):
        """Initialize the report generator with request data and step size.
        
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
        """Create a formatted label for time intervals based on the specified duration unit.
        
        Args:
            slot (datetime): The current time slot for which the label is being generated.
        
        Returns:
            tuple: A tuple containing the interval label and a time label.
        """
        if self.period == "days":
            return "D{:02d}-D{:02d}".format(slot.day, (slot + self.step_timedelta).day), slot.strftime('%d%b%y')
        elif self.period == "hours":
            return "H{:02d}-H{:02d}".format(slot.hour, (slot + self.step_timedelta).hour), slot.strftime('%H:%M')
        else:  # For minutes
            return "M{:02d}-M{:02d}".format(slot.minute, (slot + self.step_timedelta).minute), slot.strftime('%H:%M')

    def get_memmory_statistics_report_header(self):
        """Generate a well-aligned, formatted header for the memory report.
        
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

    def __init__(self, sampling_interval: int, retention_period: int):
        """
        Initializes MemoryStatisticsCollector with data collection interval and retention period.

        :param sampling_interval: Interval between data collections in minutes (3 to 15).
        :param retention_period: Data retention period in days (1 to 30).
        """
        self.sampling_interval = sampling_interval
        self.retention_period = retention_period 
    
    def fetch_memory_statistics(self):
        """
        Collect memory statistics using psutil.
        
        Returns:
            dict: A dictionary containing memory statistics.
        """
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

   
    def fetch_memory_entries(self, filepath):
        """
        Fetch memory entries from a compressed file.
        
        Args:
            filepath (str): The path to the compressed file containing memory entries.

        Returns:
            dict: A dictionary containing loaded memory entries or default structure.
        """
        tentry = {'system_memory': {'system': {}, "count": 0}, "count": 0}
        if not os.path.exists(filepath):
            return tentry

        if os.path.getsize(filepath) <= 0:
            return tentry

        try:
            with gzip.open(filepath, 'rb') as bfile:
                loaded_entry = json.load(bfile)
        except Exception as e:
            logging.error(f"Failed to load memory entries from {filepath}: {e}")
            return tentry 

        return loaded_entry
 
    def update_memory_statistics(self, total_dict, mem_dict, time_list, item, category):
        """
        Update memory statistics for the specified item and category.
        
        Args:
            total_dict (dict): The total statistics dictionary.
            mem_dict (dict): The memory statistics dictionary to update.
            time_list (dict): The current memory statistics collected.
            item (str): The item to update in total_dict.
            category (str): The category under which to update the statistics.
        """
        for memory_metric in time_list.keys():
            try:
                high_value = 0
                low_value = 0
                prss = int(time_list[memory_metric]['prss'])
                mem_dict[category][memory_metric] = {
                    "prss": prss, "count": 1,
                    "high_value": prss, "low_value": prss
                }
                
                mem = total_dict[item][category]
                if memory_metric in mem:
                    tprss = int(mem[memory_metric]["prss"]) + prss
                    tcount = int(mem[memory_metric]["count"]) + 1
                    if prss > int(mem[memory_metric]["high_value"]):
                        high_value = prss
                        low_value = int(mem[memory_metric]["low_value"])
                    elif prss <= int(mem[memory_metric]["low_value"]):
                        low_value = prss
                        high_value = int(mem[memory_metric]["high_value"])
                    mem[memory_metric] = {
                        "prss": tprss, "count": tcount,
                        "high_value": high_value,
                        "low_value": low_value
                    }
                else:
                    mem[memory_metric] = {
                        "prss": prss, "count": 1,
                        "high_value": prss, "low_value": prss
                    }
            except Exception as e:
                logger.log_error(f"Error updating memory statistics for item '{item}', category '{category}', "
                          f"metric '{memory_metric}': {e}")

        total_dict[item]['count'] = int(total_dict[item]['count']) + 1


    def enforce_retention_policy(self, total_dict):
        """This function enforces a retention policy for memory statistics by identifying and removing entries in total_dict
           that are older than the configured retention period. 
           total_dict (dict): A dictionary containing memory statistics, where each entry is expected to include a 'current_time' field.
        """
        current_time = datetime.now()
        retention_threshold = timedelta(days=self.retention_period)

        if 'system_memory' in total_dict:
            for memory_type in total_dict['system_memory']:
                entries_to_remove = []
                
                if isinstance(total_dict['system_memory'][memory_type], dict):
                    for entry_timestamp, memory_data in total_dict['system_memory'][memory_type].items():
                        try:
                            entry_time = datetime.fromisoformat(entry_timestamp)
                        except ValueError:
                            continue 

                        if current_time - entry_time > retention_threshold:
                            entries_to_remove.append(entry_timestamp)

                    for old_entry in entries_to_remove:
                        logging.info(f"Removing outdated entry: {old_entry}")
                        del total_dict['system_memory'][memory_type][old_entry]

        total_dict['system_memory']['count'] = len(total_dict['system_memory'].get('system', {}))

    def collect_and_store_memory_usage(self, collect_only):
        """
        Dump memory usage statistics into log files.
        
        Args:
            collect_only (bool): If True, return memory data without dumping.
        """
        sysmem_dict = {}
        total_dict = self.fetch_memory_entries(memory_statistics_config['TOTAL_MEMORY_STATISTICS_LOG_FILENAME'])

        sm = self.fetch_memory_statistics()
        sysmem_dict = {"system": {}, "count": 1}

        if 'system_memory' not in total_dict:
            total_dict['system_memory'] = {'system': {}, "count": 0}
        if 'system' not in total_dict['system_memory']:
            total_dict['system_memory']['system'] = {}
            total_dict['system_memory']['count'] = 0

        self.update_memory_statistics(total_dict, sysmem_dict, sm, 'system_memory', 'system')

        total_dict['count'] = int(total_dict['count']) + 1

        self.enforce_retention_policy(total_dict)

        current_time = Utility.fetch_current_date()

        total_dict['current_time'] = current_time
        mem_dict = {"current_time": current_time, "system_memory": sysmem_dict, "count": 1}

        if collect_only is True:
            return mem_dict

        try:
            with gzip.open(memory_statistics_config['TOTAL_MEMORY_STATISTICS_LOG_FILENAME'], "wb") as bfile:
                json.dump(total_dict, bfile)
        except Exception as e:
            logging.error(f"Failed to dump total memory statistics: {e}")
            return

        try:
            with gzip.open(memory_statistics_config['MEMORY_STATISTICS_LOG_FILENAME'], "ab") as bfile:
                json.dump(mem_dict, bfile)
        except Exception as e:
            logging.error(f"Failed to dump memory statistics log: {e}")


class MemoryEntryManager:

    def add_memory_entry(self, request, total_entries_all, global_entries, local_entries, new_entry, item, category, entry_list):
        """Add memory entry to global and local entries.
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
        """Calculate and add average values for memory entries.
        Args:
            total_entry (dict): The dictionary containing total memory entries.
            item (str): The item to calculate averages for.
            category (str): The category of the memory entry.
        """
        for metric_name in total_entry[item][category].keys():
            total_entry[item][category][metric_name]['avg_value'] = int(total_entry[item][category][metric_name]['prss']) / int(total_entry[item][category][metric_name]['count'])

    def get_global_entry_data(self, request, total_entry_all, local_entry, entries):
        """Aggregate global entry data from local entries.
        Args:
            request (dict): The request object containing parameters for filtering.
            total_entry_all (dict): Dictionary to hold total entries across all items.
            local_entry (dict): Dictionary holding local memory entries.
            entries (dict): Dictionary containing memory entry time list.
        Returns:
            dict: Aggregated global memory entry data.
        """
        gentry = {"system_memory": {"system": {}, "count": 0}, "count": 0}
        
        for day in range(0, int(entries['count']), 1):
            if entries["time_list"][day]['count'] == 0:
                continue
            
            self.add_memory_entry(request, total_entry_all, gentry, local_entry, entries["time_list"][day], 'system_memory', 'system', "system_list")
        
        self.add_entry_total(gentry, 'system_memory', 'system')
    
        return gentry

    def get_current_memory(self, request, current_memory, memory_type, metric_name):
        """Retrieve current memory value for a specified type and metric name.
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
        """Retrieve formatted memory entry data based on request parameters.
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

        memory_type = "system_memory"
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
            memory_entry = {"system_memory": {}, "count": 0}
            time_entry = {"time_list": [memory_entry], "count": 0}
            time_entry_summary["time_group_list"].append(time_entry)

    def process_files(self, request_data, memory_data_filename, start_time_obj, end_time_obj, step, num_columns, time_entry_summary,
                     first_interval_unit, first_interval_rate, second_interval_unit):
        """
        Processes memory data files within the specified time range.

        Parameters:
        - request_data (dict): Request data containing time range information.
        - memory_data_filename (str): The filename from which to read memory statistics data.
        - start_time_obj (datetime): The starting time object for filtering.
        - end_time_obj (datetime): The ending time object for filtering.
        - step (int): Step size for processing time slices.
        - num_columns (int): Number of columns for time slices.
        - time_entry_summary (dict): Summary structure to hold time group information.
        - first_interval_unit (str): The first time interval unit.
        """
        start_day, end_day = int(request_data['time_data']['start_day']), int(request_data['time_data']['end_day'])
        add = 1 if int(request_data['time_data']['num_days']) == 0 else 0
        
        for i in range(start_day, end_day + add):
            file_name = f"{memory_data_filename}.{i}" if i != 0 else memory_data_filename
            self.process_file(file_name, start_time_obj, end_time_obj, step, num_columns, time_entry_summary, request_data,
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
        if os.path.exists(file_name) and os.path.getsize(file_name) > 0:
            with gzip.open(file_name, "rb") as bfile:
                try:
                    while True:
                        memory_entry = json.load(bfile)
                        self.process_memory_entry(memory_entry, start_time_obj, end_time_obj, step, num_columns, time_entry_summary, request_data,
                                                 first_interval_unit, first_interval_rate, second_interval_unit)
                except (EOFError, json.UnpicklingError):
                    pass

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
        Aggregates processed memory data for reporting.

        Parameters:
        - request_data (dict): Request data containing time range information.
        - time_entry_summary (dict): Summary structure containing aggregated memory statistics.
        - num_columns (int): Number of columns for time slices.
        """
        last_entry = {"system_memory": {"system": {}}}
        for i in range(num_columns):
            time_entry_summary["time_group_list"][i] = self.entry_manager.get_global_entry_data(
                request_data, time_entry_summary, last_entry, time_entry_summary["time_group_list"][i]
            )

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

    def __init__(self, address, command_handler, stop_event):
        """
        Initializes the SocketHandler with the specified parameters.
        
        :param address: The file system path where the UNIX socket will be created.
        :param command_handler: A callable that processes commands received from clients.
        :param stop_event: An event flag used to signal when to stop the socket listener.
        """
        self.address = address
        self.command_handler = command_handler
        self.listener_socket = None
        self.stop_event = stop_event 

    def safe_remove_file(self, filepath):
        """Removes a file if it exists to prevent socket binding errors.
        
        This method checks for the existence of the specified file and removes
        it if found. It logs the action taken or any errors encountered.
        
        :param filepath: The path of the socket file to be removed.
        """
        try:
            if os.path.exists(filepath):
                os.remove(filepath)
                logging.info(f"Removed existing socket file: {filepath}")
        except Exception as e:
            logging.error(f"Failed to remove file {filepath}: {e}")

    def create_unix_socket(self):
        """Creates and configures a UNIX socket for listening for incoming connections.
        
        This method sets up the socket with appropriate permissions and starts
        listening for client connections. It raises an exception if socket creation fails.
        """
        try:
            self.listener_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.listener_socket.settimeout(1.0) 
            self.listener_socket.bind(self.address)
            os.chmod(self.address, 0o644)
            self.listener_socket.listen(5)  
            logging.info(f"UNIX socket created and listening at {self.address}")
        except Exception as e:
            logging.error(f"Failed to create UNIX socket at {self.address}: {e}")
            raise

    def send_response(self, connection, response_data):
        """Sends a JSON response back to the client.
        
        This method encodes the response data as JSON and sends it through
        the established socket connection. It logs any errors encountered during the process.
        
        :param connection: The socket connection to the client.
        :param response_data: The data to be sent as a JSON response.
        """
        try:
            response_json = json.dumps(response_data)
            connection.sendall(response_json.encode('utf-8'))
            logging.debug(f"Sent response: {response_json}")
        except Exception as e:
            logging.error(f"Failed to send response: {e}")

    def handle_connection(self, connection):
        """Processes a single incoming socket connection.
        
        This method reads the request data from the client, decodes it from JSON,
        and processes it using the command handler. It handles any exceptions,
        sending an error response if needed, and closes the connection afterward.
        
        :param connection: The socket connection established with the client.
        """
        error_response = {"status": False, "msg": None}
        try:
            request_data = connection.recv(4096)
            if not request_data:
                logging.warning("Received empty request")
                return

            request_json = json.loads(request_data.decode('utf-8'))
            logging.debug(f"Received request: {request_json}")
            command_name = request_json['command']
            command_data = request_json.get('data', {})

            response = self.command_handler(command_name, command_data)

            self.send_response(connection, response)
        except Exception as error:
            logging.error(f"Error handling request: {traceback.format_exc()}")
            error_response['msg'] = str(error)
            self.send_response(connection, error_response)
        finally:
            try:
                connection.close()
                logging.debug("Connection closed")
            except Exception as e:
                logging.error(f"Error closing connection: {e}")

    def stop_listening(self):
        """Stops the socket listener loop by setting stop_event."""
        logging.info("Stopping listener loop.")
        self.stop_event.set() 
        if self.listener_socket:
            try:
                self.listener_socket.close() 
            except Exception as e:
                logging.error(f"Error while closing listener socket: {e}")
                           

    def start_listening(self):
        """Starts listening for incoming socket connections.
        
        This method initializes the socket, removes any existing socket files,
        and enters a loop to accept and handle incoming connections. The loop
        continues until the stop_event is set. Upon shutdown, it cleans up the
        socket file and closes the listener socket.
        """
        self.safe_remove_file(self.address)
        self.create_unix_socket()

        while not self.stop_event.is_set():
            try:
                connection, client_address = self.listener_socket.accept()
                logging.info("Accepted new connection")
                self.handle_connection(connection)
            except socket.timeout:
                continue  
            except OSError as e:
                if self.stop_event.is_set():
                    logging.info("Socket listener stopped as requested.")
                    break
                else:
                    logging.error(f"Socket error: {e}")
                    time.sleep(1) 
            except Exception as error:
                logging.error(f"Unexpected error: {error}")
                time.sleep(1)

        self.safe_remove_file(self.address)
        logging.info("Socket listener stopped.")


class Daemonizer:
    """Facilitates the daemonization of the current process.
    
    This class provides methods to fork the process into the background,
    manage the process ID (PID), and redirect standard file descriptors
    to /dev/null, ensuring that the daemon operates independently from
    the terminal.
    """
    def __init__(self, pid_file):
        """
        Initializes the Daemonizer with the specified PID file location.
        
        :param pid_file: The file path where the daemon's PID will be stored.
        """
        self.pid_file = pid_file

    def daemonize(self):
        """Forks the process to run as a background daemon.
        
        This method performs the necessary steps to create a daemon process,
        including forking twice and creating a new session. It logs the
        success of the daemonization and writes the PID to a file.
        """
        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)
        except OSError as e:
            logging.error(f"First fork failed: {e}")
            sys.exit(1)

        os.setsid() 

        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)
        except OSError as e:
            logging.error(f"Second fork failed: {e}")
            sys.exit(1)

        logging.info(f"Daemonization successful with PID: {os.getpid()}")
        self.write_pid_to_file()
        self.redirect_standard_file_descriptors()

    def write_pid_to_file(self):
        """Writes the daemon's PID to the specified file for management purposes.
        
        This method ensures that the PID of the running daemon is stored in a
        file, which can be used later to manage the daemon process (e.g., for
        stopping it). It logs the action taken and handles any errors.
        """
        try:
            with open(self.pid_file, 'w') as f:
                f.write(f"{os.getpid()}\n")
            logging.debug(f"Daemon PID written to {self.pid_file}")
        except Exception as e:
            logging.error(f"Failed to write PID file {self.pid_file}: {e}")
            sys.exit(1)

    def redirect_standard_file_descriptors(self):
        """Redirects standard file descriptors to /dev/null.
        
        This method ensures that the daemon does not receive any terminal input/output
        by redirecting stdin, stdout, and stderr to /dev/null. It logs the action
        taken and any errors encountered during the process.
        """
        try:
            sys.stdout.flush()
            sys.stderr.flush()
            with open(os.devnull, 'r') as devnull:
                os.dup2(devnull.fileno(), sys.stdin.fileno())
            with open(os.devnull, 'a+') as devnull:
                os.dup2(devnull.fileno(), sys.stdout.fileno())
                os.dup2(devnull.fileno(), sys.stderr.fileno())
            logging.debug("Standard file descriptors redirected to /dev/null")
        except Exception as e:
            logging.error(f"Failed to redirect standard file descriptors: {e}")
            sys.exit(1)


class MemoryStatisticsService:
    """
    Manages the Memory Statistics Service, responsible for collecting,
    processing, and serving memory usage statistics in a daemonized manner.
    This service utilizes a socket for communication and handles
    commands for memory statistics retrieval, while also managing
    configuration reloading and graceful shutdown procedures.
    """ 

    def __init__(self, memory_statistics_config, config_file_path='memorystats.conf', name="MemoryStatisticsService"):
        """
        Initializes the MemoryStatisticsService instance.
        Parameters:
        - memory_statistics_config (dict): Initial configuration settings for the service.
        - config_file_path (str): Path to the configuration file to load overrides.
        """

        self.name = name  # Set the service name
        logging.info(f"Service initialized with name: {self.name}")  # Log the name

        self.config_file_path = config_file_path
        self.memory_statistics_lock = threading.Lock()
        self.stop_event = threading.Event()  
    
        self.socket_listener_thread = None
        self.memory_collection_thread = None

        self.config = memory_statistics_config.copy()
        self.config.update(self.load_config_from_file())

        self.sampling_interval = int(self.config.get('sampling_interval', 3)) * 60  
        self.retention_period = int(self.config.get('retention_period', 15)) 

        self.socket_handler = SocketHandler(
            address=self.config['DBUS_SOCKET_ADDRESS'],
            command_handler=self.handle_command,
            stop_event=self.stop_event
        )
        self.daemonizer = Daemonizer('/var/run/memory_statistics_daemon.pid')

        signal.signal(signal.SIGHUP, self.handle_sighup)
        signal.signal(signal.SIGTERM, self.handle_sigterm)
    
    def load_config_from_file(self):
        """
        Loads specific configuration values from the configuration file.
        Returns:
        - dict: A dictionary containing configuration overrides from the file.
        """
        config = {}
        parser = configparser.ConfigParser()
        try:
            parser.read(self.config_file_path)
            config['sampling_interval'] = parser.get('default', 'sampling_interval', fallback='3')
            config['retention_period'] = parser.get('default', 'retention_period', fallback='15')
            logging.info(f"Configuration loaded from file (overrides only): {config}")
        except Exception as e:
            logging.error(f"Failed to load configuration from file: {e}")
        return config

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
            logging.warning(f"Unknown command received: {command_name}")
            return {"status": False, "msg": "Unknown command"}

    def handle_sighup(self, signum, frame):
        """
        Responds to the SIGHUP signal to reload the service configuration.
        This method performs cleanup of old log files and updates the service's
        runtime configuration from the ConfigDB.
        """
        logging.info("Received SIGHUP, reloading configuration.")
        try:
            self.cleanup_old_files()
            self.load_config_from_db()
        except Exception as e:
            logging.error(f"Error handling SIGHUP: {e}")

    def handle_sigterm(self, signum, frame):
        """
        Handles the SIGTERM signal for graceful shutdown of the service.
        This method attempts to terminate child processes, stop running threads,
        and perform necessary cleanup operations before exiting.
        """
        logging.info("Received SIGTERM, initiating graceful shutdown...")

        def terminate_child_processes():
            """
            Gracefully terminates all child processes associated with the service.
            """
            current_pid = os.getpid()
            try:
                children = [
                    proc for proc in psutil.process_iter(['pid', 'ppid', 'name'])
                    if proc.info['ppid'] == current_pid
                ]

                for child in children:
                    try:
                        logging.info(f"Sending SIGTERM to child process {child.info['pid']}")
                        os.kill(child.info['pid'], signal.SIGTERM)
                    except ProcessLookupError:
                        continue

                timeout = 5
                start_time = time.time()
                while any(os.kill(child.info['pid'], 0) == 0 for child in children if child.is_running()) \
                        and time.time() - start_time < timeout:
                    time.sleep(0.1)

                for child in children:
                    if child.is_running():
                        logging.warning(f"Force killing child process {child.info['pid']}")
                        os.kill(child.info['pid'], signal.SIGKILL)

            except Exception as e:
                logging.error(f"Error while terminating child processes: {e}")
                raise

        try:
            if hasattr(self.socket_handler, 'stop_accepting'):
                self.socket_handler.stop_accepting()

            terminate_child_processes()

            self.stop_threads()

            self.cleanup()

            logging.info("Shutdown complete. Exiting...")
            sys.exit(0)

        except Exception as e:
            logging.error(f"Error during graceful shutdown: {e}")

            try:
                os.killpg(os.getpgid(0), signal.SIGKILL)
            except Exception as kill_error:
                logging.error(f"Error during force kill: {kill_error}")
            sys.exit(1)

    def load_config_from_db(self):
        """
        Retrieves runtime configuration values from the ConfigDB.
        This method updates the service's sampling interval and retention period
        based on the values retrieved from the database.
        Raises:
        - Exception: If an error occurs while accessing the ConfigDB.
        """
        # Placeholder for ConfigDBConnector
        # Replace with actual implementation
        # class ConfigDBConnector:
        #     def connect(self):
        #         pass

        #     def get_table(self, table_name):
        #         # Dummy implementation
        #         return {
        #             'retention-period': '20',
        #             'sampling-interval': '5' 
        #         }

        #     def disconnect(self):
        #         pass

        config_db = ConfigDBConnector()
        config_db.connect()

        try:
            config = config_db.get_table('MEMORY_STATISTICS')

            self.retention_period = int(config.get('retention-period', self.retention_period))
            self.sampling_interval = int(config.get('sampling-interval', self.sampling_interval)) * 60
            
            logging.info(f"Configuration reloaded from ConfigDB: sampling_interval={self.sampling_interval} seconds, retention_period={self.retention_period} days")
        except Exception as e:
            logging.error(f"Error loading configuration from ConfigDB: {e}, using current settings")
        finally:
            config_db.disconnect()

    def cleanup_old_files(self):
        """
        Deletes old log files from the log directory.
        This method removes any log files with a .gz extension from the specified
        log directory to manage disk space and maintain organization.
        """
        try:
            log_directory = self.config.get('LOG_DIRECTORY', '/var/log/histogram')
            for file in os.listdir(log_directory):
                if file.endswith('.gz'):
                    file_path = os.path.join(log_directory, file)
                    os.remove(file_path)
                    logging.info(f"Deleted old log file: {file_path}")
        except Exception as e:
            logging.error(f"Error during log file cleanup: {e}")

    def memory_statistics_command_request_handler(self, request):
        """
        Processes requests for memory statistics.
        Parameters:
        - request (dict): Contains information about the statistics request.
        Returns:
        - dict: A response indicating the success or failure of the request,
                along with the collected memory statistics data if successful.
        """
        try:
            logging.info(f"Received memory statistics request: {request}")
            with self.memory_statistics_lock:
                memory_collector = MemoryStatisticsCollector(
                    sampling_interval=self.sampling_interval // 60,
                    retention_period=self.retention_period
                )
                current_memory = memory_collector.collect_and_store_memory_usage(collect_only=True)
                request['current_memory'] = current_memory
                logging.info(f"Current memory usage collected: {current_memory}")

                time_processor = TimeProcessor(
                    sampling_interval=self.sampling_interval // 60,
                    retention_period=self.retention_period
                )
                time_processor.process_time_information(request)

                processor = MemoryStatisticsProcessor(self.config,                            
                                                    sampling_interval=self.sampling_interval,
                                                    retention_period=self.retention_period)
                report = processor.calculate_memory_statistics_period(request)
                logging.info(f"Memory statistics processed: {report}")

                return {"status": True, "data": report}

        except Exception as error:
            logging.error(f"Error handling memory statistics request: {error}")
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
        logging.info("Socket listener thread started.")

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
        def memory_collection():
            logging.info("Memory statistics collection thread started.")
            while not self.stop_event.is_set():
                start_time = datetime.now()
                try:
                    with self.memory_statistics_lock:
                        memory_collector = MemoryStatisticsCollector(
                            sampling_interval=self.sampling_interval // 60,
                            retention_period=self.retention_period
                        )
                        memory_collector.collect_and_store_memory_usage(collect_only=False)
                except Exception as error:
                    logging.error(f"Error during memory statistics collection: {error}")

                elapsed_time = (datetime.now() - start_time).total_seconds()
                sleep_time = max(0, self.sampling_interval - elapsed_time)
                if self.stop_event.wait(timeout=sleep_time):
                    break 

            logging.info("Memory statistics collection thread stopped.")

        self.memory_collection_thread = threading.Thread(
            target=memory_collection,
            name='MemoryCollection',
            daemon=True
        )
        self.memory_collection_thread.start()
        logging.info("Memory collection thread started.")

    def stop_threads(self):
        """
        Signals threads to stop and waits for them to exit gracefully.
        This method sets the stop event to signal all running threads to 
        terminate. It also closes the listener socket to unblock the accept 
        method and waits for both the socket listener and memory collection 
        threads to finish their execution within a specified timeout.
        Logs any issues encountered during the stopping process of the threads.
        """
        logging.info("Signaling threads to stop.")
        self.stop_event.set() 

        self.socket_handler.stop_listening()

        if self.socket_listener_thread:
            logging.info("Stopping socket listener thread...")
            self.socket_listener_thread.join(timeout=5)
            if self.socket_listener_thread.is_alive():
                logging.warning("Socket listener thread did not stop gracefully")

        if self.memory_collection_thread:
            logging.info("Stopping memory collection thread...")
            self.memory_collection_thread.join(timeout=5)
            if self.memory_collection_thread.is_alive():
                logging.warning("Memory collection thread did not stop gracefully")    

    def cleanup(self):
        """
        Performs additional cleanup tasks before service shutdown.
        This method handles the cleanup of old log files, removes the socket 
        and PID files, and flushes all log handlers to ensure that all logs 
        are properly written out. This is typically called during service 
        termination to free up resources and maintain a clean state.
        Logs the completion of the cleanup tasks and any errors encountered 
        during the process.
        """
        self.cleanup_old_files()

        try:
            if os.path.exists(self.config['DBUS_SOCKET_ADDRESS']):
                os.unlink(self.config['DBUS_SOCKET_ADDRESS'])
                logging.info(f"Removed socket file: {self.config['DBUS_SOCKET_ADDRESS']}")
        except Exception as e:
            logging.error(f"Error removing socket file: {e}")

        pid_file = '/var/run/memory_statistics_daemon.pid'
        try:
            if os.path.exists(pid_file):
                os.unlink(pid_file)
                logging.info(f"Removed PID file: {pid_file}")
        except Exception as e:
            logging.error(f"Error removing PID file: {e}")

        try:
            for handler in logging.getLogger().handlers:
                handler.flush()
        except Exception as e:
            logging.error(f"Error flushing log handlers: {e}")

        logging.info("Cleanup complete.")

    def run(self):
        """
        Runs the Memory Statistics Service.
        This method starts the service by daemonizing the process and 
        initializing the threads necessary for socket listening and memory 
        statistics collection. The service will continue to run until 
        signaled to stop, during which it sleeps to reduce CPU usage.
        Logs the initialization and starting of the service.
        """
        logging.info(f"{self.name} is starting...")  # Log the service name at start
        self.daemonizer.daemonize()

        self.start_socket_listener()
        self.start_memory_collection()
    
        while not self.stop_event.is_set():
            time.sleep(1) 

if __name__ == '__main__':
    logging.basicConfig(filename='mem_stats_debug.log', level=logging.DEBUG, 
                        format='%(asctime)s - %(levelname)s - %(message)s') 
    memory_statistics_config = {
        'LOG_DIRECTORY': "/var/log/memory_statistics",
        'MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/memory-stats.log.gz",
        'TOTAL_MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/total-memory-stats.log.gz",
        'DBUS_SOCKET_ADDRESS': '/var/run/dbus/memstats.socket'
    }
    
    logger = SyslogLogger(
        identifier="memstats#log",
        log_to_console=True
    )
        
    service = MemoryStatisticsService(memory_statistics_config)
    service.run()

if __name__ == '__main__':
    logging.basicConfig(filename='mem_stats_debug.log', level=logging.DEBUG, 
                        format='%(asctime)s - %(levelname)s - %(message)s') 
    memory_statistics_config = {
        'LOG_DIRECTORY': "/var/log/memory_statistics",
        'MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/memory-stats.log.gz",
        'TOTAL_MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/total-memory-stats.log.gz",
        'DBUS_SOCKET_ADDRESS': '/var/run/dbus/memstats.socket'
    }
    
    logger = SyslogLogger(
        identifier="memstats#log",
        log_to_console=True
    )
    
    service_name = "MemoryStatisticsService"  # You can set this dynamically or pass as an argument
    service = MemoryStatisticsService(memory_statistics_config, name=service_name)
    service.run()