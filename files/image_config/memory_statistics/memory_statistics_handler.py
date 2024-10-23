####################################################################################################################################################################
import psutil
import os
import signal
import sys
import time
import pickle
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
from utilities_common.dict2obj import Dict2Obj
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
        self.period = request['duration']  # 'minutes', 'hours', or 'days'
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
        # Codes explanation section
        fmt = "\nCodes:\tM - minutes, H - hours, D - days\n"
        fmt += "-" * 80 + "\n"

        # Main report details
        fmt += "Report Generated:    {}\n".format(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        fmt += "Analysis Period:     From {} to {}\n".format(self.start.strftime("%Y-%m-%d %H:%M:%S"),
                                                             self.end.strftime("%Y-%m-%d %H:%M:%S"))
        fmt += "Interval:            {} {}\n".format(self.step, self.period.capitalize())

        # Initialize for dynamic columns
        slot = self.start
        num_intervals = 0
        interval_labels = []
        time_labels = []

        # Generate interval labels and time labels
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
                       (INTERVAL_WIDTH + 1) * num_intervals - 1)  # last interval no space

        # Create the separator line
        separator = "-" * total_width + "\n"

        # Build the header with the metric titles and dashes
        header = separator
        # Header row
        header += "{:<{}} {:<{}} {:<{}} {:<{}}".format(
            "Metric", METRIC_WIDTH, "Current", VALUE_WIDTH, "High", VALUE_WIDTH, "Low", VALUE_WIDTH
        )
        for label in interval_labels:
            header += " {:<{}}".format(label, INTERVAL_WIDTH)
        header += "\n"

        # Subheader row
        header += "{:<{}} {:<{}} {:<{}} {:<{}}".format(
            " ", METRIC_WIDTH, "Value", VALUE_WIDTH, "Value", VALUE_WIDTH, "Value", VALUE_WIDTH
        )
        for label in time_labels:
            header += " {:<{}}".format(label, INTERVAL_WIDTH)
        header += "\n"

        # Add another separator
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
            'total': {"prss": memory_data.total, "count": 1},
            'used': {"prss": memory_data.used, "count": 1},
            'free': {"prss": memory_data.free, "count": 1},
            'available': {"prss": memory_data.available, "count": 1},
            'cached': {"prss": memory_data.cached, "count": 1},
            'buffers': {"prss": memory_data.buffers, "count": 1},
            'shared': {"prss": memory_data.shared, "count": 1}        
   
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
                loaded_entry = pickle.load(bfile)
        except Exception as e:
            logging.error(f"Failed to load memory entries from {filepath}: {e}")
            return tentry  # Return default if loading fails

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
                print(e)  # Consider replacing with proper logging

        total_dict[item]['count'] = int(total_dict[item]['count']) + 1

    def dump_memory_usage(self, collect_only):
        """
        Dump memory usage statistics into log files.
        
        Args:
            collect_only (bool): If True, return memory data without dumping.
        """
        sysmem_dict = {}
        total_dict = self.fetch_memory_entries(memory_statistics_config['TOTAL_MEMORY_STATISTICS_LOG_FILENAME'])

        sm = self.fetch_memory_statistics()
        sysmem_dict = {"system": {}, "count": 1}

        # Ensure 'system_memory' and 'system' keys exist
        if 'system_memory' not in total_dict:
            total_dict['system_memory'] = {'system': {}, "count": 0}
        if 'system' not in total_dict['system_memory']:
            total_dict['system_memory']['system'] = {}
            total_dict['system_memory']['count'] = 0

        # Update total dictionary with the collected system memory statistics
        self.update_memory_statistics(total_dict, sysmem_dict, sm, 'system_memory', 'system')

        # Total Count
        total_dict['count'] = int(total_dict['count']) + 1

        # Update the current time and record
        current_time = Utility.fetch_current_date()

        total_dict['current_time'] = current_time
        mem_dict = {"current_time": current_time, "system_memory": sysmem_dict, "count": 1}

        if collect_only is True:
            return mem_dict

        # Dump total system memory statistics into the log file
        try:
            with gzip.open(memory_statistics_config['TOTAL_MEMORY_STATISTICS_LOG_FILENAME'], "wb") as bfile:
                pickle.dump(total_dict, bfile)
        except Exception as e:
            logging.error(f"Failed to dump total memory statistics: {e}")
            return

        try:
            with gzip.open(memory_statistics_config['MEMORY_STATISTICS_LOG_FILENAME'], "ab") as bfile:
                pickle.dump(mem_dict, bfile)
        except Exception as e:
            logging.error(f"Failed to dump memory statistics log: {e}")


    def enforce_retention_policy(self, total_dict):
        """This function enforces a retention policy for memory statistics by identifying and removing entries in total_dict
           that are older than the configured retention period. 
           total_dict (dict): A dictionary containing memory statistics, where each entry is expected to include a 'current_time' field.
        """
        retention_delta = timedelta(days=self.retention_period)
        current_time = datetime.now()  # Use current time directly

        # Log the total_dict before processing
        logging.info(f"Total dictionary before retention policy enforcement: {total_dict}")

        # Identify and remove entries older than the retention period
        for key in list(total_dict.keys()):
            if key == 'current_time':
                continue  # Skip the current time key

            # Ensure the entry has a 'current_time' key
            if 'current_time' not in total_dict[key]:
                logging.warning(f"Key '{key}' does not have 'current_time'; skipping.")
                continue
            
            try:
                entry_time = datetime.fromisoformat(total_dict[key]['current_time'])
                if current_time - entry_time > retention_delta:
                    logging.info(f"Removing old memory entry: {key} (age: {current_time - entry_time})")
                    del total_dict[key]
                else:
                    logging.info(f"Keeping entry: {key} (age: {current_time - entry_time})")
            except ValueError as ve:
                logging.error(f"Invalid date format for entry '{key}': {ve}. Skipping entry.")
            except Exception as e:
                logging.error(f"Error processing entry '{key}': {e}. Skipping entry.")

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
  
        # Get max records count from the request, default to 200 if not provided
        max_record_count = int(request.get('rcount', 200))
        count = 0
        formatted_output = ""

        # Set default field for metrics if not provided
        selected_field = request.get('field', 'avg_value')  # Default to 'avg_value'

        # System memory only
        memory_type = "system_memory"
        current_memory = request['current_memory']

        total_high_value, total_low_value = {}, {}

        # Define fixed widths for each column to ensure alignment
        METRIC_WIDTH = 18
        VALUE_WIDTH = 10
        INTERVAL_WIDTH = 11

        # Process system memory entries
        for metric_name in current_memory[memory_type].get(request['type'], {}):
            # Filter by metric_name if provided in the request
            if request.get('metric_name') and not re.match(f".*{request['metric_name']}.*", metric_name):
                continue

            # Count and truncate if necessary
            count += 1
            if count > max_record_count:
                formatted_output += "<< more records truncated, use filter query to optimize the search >>\n"
                break

            # Initialize total high and low values for the metric
            total_high_value[metric_name] = 0
            total_low_value[metric_name] = 0

            # Prepare a list to hold history values
            history_values = []

            # Iterate over time entries in time_group_list
            for time_entry in total_entry_all.get("time_group_list", []):
                memory_data = time_entry.get(memory_type, {}).get(request['type'], {}).get(metric_name, {})
                if memory_data:
                    mem_value = int(memory_data.get(selected_field, 0))  # Use the value of selected_field
                    history_values.append(Memory_formatter.format_memory_size(mem_value))

                    # Update high and low values
                    total_high_value[metric_name] = max(total_high_value[metric_name], mem_value)
                    total_low_value[metric_name] = min(total_low_value[metric_name] or mem_value, mem_value)
                else:
                    history_values.append('-')

            # Display current, high, low, and history values
            current_mem = Memory_formatter.format_memory_size(self.get_current_memory(request, current_memory, memory_type, metric_name))
            high_mem = Memory_formatter.format_memory_size(total_high_value[metric_name])
            low_mem = Memory_formatter.format_memory_size(total_low_value[metric_name])

            # Construct the data row with proper alignment
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
    def __init__(self, memory_statistics_config):
        """
        Initializes the MemoryStatisticsProcessor with configuration settings.

        Parameters:
        - memory_statistics_config (dict): Configuration dictionary containing parameters for 
          memory statistics processing such as sampling interval and retention period.
        """
        self.memory_statistics_config = memory_statistics_config
        self.sampling_interval = self.memory_statistics_config.get('sampling_interval', 3) * 60  # Default in seconds
        self.retention_period = self.memory_statistics_config.get('retention_period', 30)  # Default in days
        self.entry_manager = MemoryEntryManager()
        self.memory_formatter = Utility()

    def update_sampling_interval(self, new_sampling_interval):
        """
        Updates the sampling interval for memory statistics collection.

        Parameters:
        - new_sampling_interval (int): New sampling interval in minutes.
        """
        self.sampling_interval = new_sampling_interval * 60  # Convert minutes to seconds
        print(f"Updated sampling interval to {self.sampling_interval} seconds.")

    def update_retention_period(self, new_retention_period):
        """
        Updates the retention period for storing memory statistics.

        Parameters:
        - new_retention_period (int): New retention period in days.
        """
        self.retention_period = new_retention_period
        print(f"Updated retention period to {self.retention_period} days.")

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
        interval_minutes = int(interval_minutes)  # Ensure interval_minutes is an integer
        if interval_minutes < 3 or interval_minutes > 15:
            raise ValueError("Sampling interval must be greater than or equal to 3 and less than or equal to 15 minutes.")
        
        hourly_rate = 60 / interval_minutes
        # Set maximum hourly rate to 20, which means mnimun 3 minutes interval
        if hourly_rate > 20:  
            raise ValueError(f"Invalid sampling interval, rate per hour: {hourly_rate}")

        # New condition to ensure hourly rate doesn't exceed 4 15 minutes
        if hourly_rate < 4:
            raise ValueError(f"Hourly rate exceeds the limit, rate per hour: {hourly_rate}")    

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
                        memory_entry = pickle.load(bfile)
                        self.process_memory_entry(memory_entry, start_time_obj, end_time_obj, step, num_columns, time_entry_summary, request_data,
                                                 first_interval_unit, first_interval_rate, second_interval_unit)
                except (EOFError, pickle.UnpicklingError):
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
        :return: Updated memory_statistics_request with the calculated 'duration'.
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
    """Handles the creation, listening, and processing of UNIX socket connections."""

    def __init__(self, address, command_handler):
        """
        Initializes the SocketHandler with a socket address and a command handler.

        :param address: The file system path for the UNIX socket.
        :param command_handler: A callable to process commands received through the socket.
        """
        self.address = address
        self.command_handler = command_handler
        self.listener_socket = None
        self.listener_thread = None  

    def safe_remove_file(self, filepath):
        """Remove a file if it exists to avoid socket binding issues."""
        try:
            if os.path.exists(filepath):
                os.remove(filepath)
                logger.log_debug(f"Removed existing socket file: {filepath}")
        except Exception as e:
            logger.log_error(f"Failed to remove file {filepath}: {e}")

    def create_unix_socket(self):
        """Create and bind a UNIX socket, setting the appropriate permissions."""
        self.listener_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.listener_socket.bind(self.address)
        os.chmod(self.address, stat.S_IRWXU | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH | stat.S_IWOTH)
        self.listener_socket.listen(1)
        logger.log_info(f"UNIX socket created and listening at {self.address}")

    def send_response(self, connection, response_data):
        """Send a JSON-encoded response to the connected client."""
        response_json = json.dumps(response_data)
        connection.sendall(response_json.encode('utf-8'))
        logger.log_debug(f"Sent response: {response_json}")

    def handle_connection(self, connection):
        """Process a single incoming connection, handling requests and sending responses."""
        error_response = {"status": False, "msg": None}
        try:
            request_data = connection.recv(4096)
            if not request_data:
                logger.log_warning("Received empty request")
                return

            request_json = json.loads(request_data.decode('utf-8'))
            logger.log_debug(f"Received request: {request_json}")
            command_name = request_json['command']
            command_data = request_json['data']

            response = self.command_handler(command_name, command_data)

            self.send_response(connection, response)

        except Exception as error:
            logger.log_error(f"Error handling request: {traceback.format_exc()}")
            error_response['msg'] = str(error)
            error_response['data'] = traceback.format_exc()
            self.send_response(connection, error_response)

        finally:
            connection.close()
            logger.log_debug("Connection closed")

    def start_listening(self):
        """Begin listening for incoming socket connections and process them."""
        self.safe_remove_file(self.address)
        self.create_unix_socket()

        while True:
            try:
                connection, client_address = self.listener_socket.accept()
                logger.log_info("Accepted new connection")
                self.handle_connection(connection)
            except Exception as error:
                logger.log_error(f"Socket error: {error}")
                self.safe_remove_file(self.address)
                time.sleep(1)  

    def close_socket(self):
        """Close the listener socket gracefully."""
        if self.listener_socket:
            self.listener_socket.close()
            logger.log_info("Listener socket closed.")

    def shutdown(self):
        """Shut down the socket listener and clean up the socket file."""
        if self.listener_socket:
            self.listener_socket.close()
            logger.log_info("Socket listener shut down.")
        self.safe_remove_file(self.address)

class Daemonizer:
    """Facilitates the daemonization of the process, allowing it to run in the background."""

    def __init__(self, pid_file):
        """
        Initializes the Daemonizer with a specified PID file path.

        :param pid_file: The file path where the daemon's process ID (PID) will be stored.
        """
        self.pid_file = pid_file

    def daemonize(self):
        """Fork the current process to run as a background daemon.

        This method performs double-forking to create a daemon process. It:
        1. Forks the process and exits the parent.
        2. Creates a new session and forks again to ensure the daemon is not a session leader.
        3. Writes the daemon's PID to the specified file and redirects standard file descriptors.
        """
        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)  # Exit the parent process
        except OSError as e:
            logger.log_error(f"First fork failed: {e}")
            sys.exit(1)

        os.setsid()  # Start a new session

        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)  # Exit the first child
        except OSError as e:
            logger.log_error(f"Second fork failed: {e}")
            sys.exit(1)

        # Now in the daemon process
        logger.log_info(f"Daemonization successful with PID: {os.getpid()}")
        self.write_pid_to_file()
        self.redirect_standard_file_descriptors()

    def write_pid_to_file(self):
        """Write the daemon's PID to the specified file for management purposes.

        This method creates or overwrites the PID file with the current process ID
        to facilitate later management, such as stopping the daemon.
        """
        with open(self.pid_file, 'w') as f:
            f.write(f"{os.getpid()}\n")
        logger.log_debug(f"Daemon PID written to {self.pid_file}")

    def redirect_standard_file_descriptors(self):
        """Redirect standard input, output, and error streams to /dev/null.

        This method ensures that the daemon does not retain control of the terminal
        by redirecting all standard file descriptors to /dev/null, effectively silencing
        any output or input.
        """
        sys.stdout.flush()  
        sys.stderr.flush()  
        with open(os.devnull, 'r') as devnull:
            os.dup2(devnull.fileno(), sys.stdin.fileno())  
        with open(os.devnull, 'a+') as devnull:
            os.dup2(devnull.fileno(), sys.stdout.fileno())  
            os.dup2(devnull.fileno(), sys.stderr.fileno())  
        logger.log_debug("Standard file descriptors redirected to /dev/null")

class MemoryStatisticsService:
    """Orchestrates the Memory Statistics Service, managing memory data collection, dynamic configuration, and signal handling."""

    def __init__(self, config):
        """
        Initializes the MemoryStatisticsService with the provided configuration.

        :param config: A dictionary containing configuration parameters for the service, such as socket address and log directory.
        """
        self.config = config
        self.memory_statistics_lock = threading.Lock()
        self.socket_handler = SocketHandler(
            address=self.config['DBUS_SOCKET_ADDRESS'],
            command_handler=self.handle_command
        )
        self.daemonizer = Daemonizer('/var/run/memory_statistics_daemon.pid')
        self.processor = MemoryStatisticsProcessor(config)
        self.shutdown_event = threading.Event()
        self.reload_event = threading.Event()

    def handle_command(self, command_name, command_data):
        """Handles incoming commands received through the socket.

        Executes the command if it is recognized, otherwise logs a warning and returns an error response.

        :param command_name: The name of the command to execute.
        :param command_data: The data associated with the command.
        :return: A response indicating success or failure.
        """
        if hasattr(self, command_name):
            command_method = getattr(self, command_name)
            return command_method(command_data)
        else:
            logger.log_warning(f"Unknown command received: {command_name}")
            return {"status": False, "msg": "Unknown command"}

    def memory_statistics_command_request_handler(self, request):
        """Handles requests for memory statistics.

        Collects current memory usage, updates time information, and calculates memory statistics.

        :param request: A dictionary containing request data, including any time-related information.
        :return: A response containing the calculated memory statistics or an error message.
        """
        try:
            logger.log_info(f"Received memory statistics request: {request}")
            with self.memory_statistics_lock:
                logger.log_info("Memory_Statistics lock acquired.")
                memory_collector = MemoryStatisticsCollector(sampling_interval=3, retention_period=30)
                current_memory = memory_collector.dump_memory_usage(collect_only=True)
                request['current_memory'] = current_memory
                logging.info(f"Current memory usage collected: {current_memory}")

                time_processor = TimeProcessor(sampling_interval=5, retention_period=15)
                time_processor.process_time_information(request)
                logging.info(f"Time information updated in request: {request.get('time_data')}")

                report = self.processor.calculate_memory_statistics_period(request)
                logging.info(f"Memory statistics processing completed. Result: {report}")

                return {"status": True, "data": report}

        except Exception as error:
            logger.log_error(f"Error handling memory statistics request: {error}")
            return {"status": False, "error": str(error)}

    def start_socket_listener(self):
        """Starts the socket listener in a separate thread.

        This method initiates the socket handler's listener thread, enabling the service to process incoming commands.
        """
        self.socket_handler.listener_thread = threading.Thread(target=self.socket_handler.start_listening, name='SocketListener', daemon=True)
        self.socket_handler.listener_thread.start()
        logger.log_info("Socket listener thread started.")

    def collect_memory_statistics(self):
        """Continuously collects memory statistics at specified intervals.

        The method runs in a loop, collecting memory statistics and checking for configuration reloads or shutdown signals.
        """
        logger.log_info("Starting Memory Statistics Collection")
        while not self.shutdown_event.is_set():
            if self.reload_event.is_set():
                self.reload_event.clear()
                self.load_config_from_db()

            try:
                memory_collector = MemoryStatisticsCollector(sampling_interval=5, retention_period=15)
                memory_collector.dump_memory_usage(collect_only=False)
                logging.info("Memory statistics collected and stored.")
            except Exception as error:
                logger.log_error(f"Error during memory statistics collection: {error}")

            self.shutdown_event.wait(self.processor.sampling_interval * 60)  # Wait for the configured interval in seconds

    def cleanup_old_files(self):
        """Cleans up old log files from the log directory.

        This method removes all .gz log files to prevent excessive disk usage and maintain log management.
        """
        try:
            log_directory = self.config['LOG_DIRECTORY']
            for file in os.listdir(log_directory):
                if file.endswith('.gz'):
                    file_path = os.path.join(log_directory, file)
                    try:
                        os.remove(file_path)  # Remove the .gz log file
                        logger.log_info(f"Deleted old log file: {file_path}")
                    except Exception as e:
                        logger.log_error(f"Error deleting old log file: {file_path}, {e}")
        except Exception as e:
            logger.log_error(f"Error during log file cleanup process: {e}")

    def load_default_settings(self):
        """Loads default settings for sampling interval and retention period from the configuration file.

        This method reads the configuration file and updates the processor's settings with default values,
        falling back if the settings are not found.
        """
        logger.log_info("Loading default settings from memory_statistics.conf.")
        
        config = configparser.ConfigParser()
        config.read('/etc/memorystats.conf')  # Change the file name to match your config file
        
        try:
            # Load the sampling interval and retention period from the config file
            self.processor.sampling_interval = config.getint('DEFAULT', 'sampling_interval', fallback=3)  # Default to 3 minutes if not set
            self.processor.retention_period = config.getint('DEFAULT', 'retention_period', fallback=30)   # Default to 30 days if not set

            logger.log_info(f"Default settings loaded: sampling_interval={self.processor.sampling_interval}, retention_period={self.processor.retention_period}")
        except Exception as e:
            logger.log_error(f"Failed to load default settings: {e}")
            self.processor.sampling_interval = 5  # Fallback to defaults
            self.processor.retention_period = 15

    def load_config_from_db(self):
        """Loads configuration settings from a configuration database.

        This method retrieves the memory statistics configuration from the database and updates
        the sampling interval and retention period with the retrieved values.
        """
        self.config_db = ConfigDBConnector()
        self.config_db.connect()  # Connect to ConfigDB

        try:
            config_db = self.config_db.get_table('MEMORY_STATISTICS')  # Get memory statistics config table

            # Update retention period and sampling interval with values from the database
            new_retention_period = int(config_db.get('retention-period', self.processor.retention_period))
            new_sampling_interval = int(config_db.get('sampling-interval', self.processor.sampling_interval))
            
            self.processor.sampling_interval = new_sampling_interval
            self.processor.retention_period = new_retention_period

            logger.log_info(f"Config reloaded from DB: sampling_interval={new_sampling_interval}, retention_period={new_retention_period}")
        except Exception as e:
            logger.log_error(f"Failed to reload config from DB: {e}")

    def reload_signal_handler(self, signum, frame):
        """Handles SIGHUP signal for configuration reload.

        Sets the reload event to trigger a configuration reload in the main collection loop.
        """
        logger.log_info("Received SIGHUP signal, reloading configuration.")
        self.cleanup_old_files()
        self.reload_event.set()

    def shutdown_signal_handler(self, signum, frame):
        """Handles SIGTERM signal for graceful shutdown.

        Sets the shutdown event to signal the collection loop to terminate and perform cleanup.
        """
        logger.log_info("Received SIGTERM signal, shutting down.")
        self.shutdown_event.set()

    def run(self):
        """Runs the Memory Statistics Daemon.

        Initiates the daemon process, starts the socket listener, sets up signal handlers,
        and begins collecting memory statistics until a shutdown signal is received.
        """
        logger.log_info("Starting Memory Statistics Daemon.")
        self.daemonizer.daemonize()
        self.start_socket_listener()

        signal.signal(signal.SIGHUP, self.reload_signal_handler)
        signal.signal(signal.SIGTERM, self.shutdown_signal_handler)

        self.collect_memory_statistics()  # Start collecting memory statistics
        logger.log_info("Memory Statistics Daemon stopped.")

        self.cleanup_old_files()  # Clean up files during shutdown
        self.socket_handler.shutdown()  # Shut down the socket handler

if __name__ == '__main__':
    """
    Main execution block for the Memory Statistics Daemon.
    This block initializes the configuration settings for logging and memory statistics,
    sets up the logger, and starts the memory statistics service daemon.
    """
    memory_statistics_config = {
        'LOG_DIRECTORY': "/var/log/memory_statistics",
        'MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/memory-stats.log.gz",
        'TOTAL_MEMORY_STATISTICS_LOG_FILENAME': "/var/log/memory_statistics/total-memory-stats.log.gz",
        'DBUS_SOCKET_ADDRESS': '/var/run/dbus/memstats.socket',
        'SYSLOG_ID': "memstats#log",
        'SYSLOG_CONSOLE': True,
    }
    logger = SyslogLogger(
        identifier=memory_statistics_config['SYSLOG_ID'],
        log_to_console=memory_statistics_config['SYSLOG_CONSOLE']
    )

    daemon = MemoryStatisticsService(memory_statistics_config)
    daemon.run()