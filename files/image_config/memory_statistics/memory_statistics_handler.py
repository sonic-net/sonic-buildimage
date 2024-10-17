import gzip
import psutil
import os
import sys
import signal
import time
import threading
import configparser
import logging
from swsscommon.swsscommon import ConfigDBConnector

class Logger:
    """
    A simple logging utility class for handling log messages.
    This class provides a flexible logging setup with options to log messages
    to a file and/or to the console. It supports various logging levels and
    formats for log messages.
    """

    def __init__(self, log_file, log_level=logging.INFO, log_console=False):
        """
        Initializes the Logger instance.
        Args:
            log_file (str): Path to the log file.
            log_level (int): Logging level (default is logging.INFO).
            log_console (bool): Whether to log messages to the console (default is False).
        """
        self.log_file = log_file
        self.log_level = log_level
        self.log_console = log_console
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(log_level)

        if not any(isinstance(h, logging.FileHandler) for h in self.logger.handlers):
            file_handler = logging.FileHandler(self.log_file)
            file_handler.setLevel(log_level)
            self.logger.addHandler(file_handler)

        if log_console and not any(isinstance(h, logging.StreamHandler) for h in self.logger.handlers):
            console_handler = logging.StreamHandler()
            console_handler.setLevel(log_level)
            self.logger.addHandler(console_handler)

        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        file_handler.setFormatter(formatter)
        if log_console:
            console_handler.setFormatter(formatter)

    def log(self, message, level=logging.INFO):
        """
        Logs a message with the specified severity level.
        Args:
            message (str): The message to log.
            level (int): The severity level of the log message. Default is logging.INFO.
        """
        {
            logging.DEBUG: self.logger.debug,
            logging.INFO: self.logger.info,
            logging.WARNING: self.logger.warning,
            logging.ERROR: self.logger.error,
            logging.CRITICAL: self.logger.critical,
        }.get(level, self.logger.info)(message)

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
        self.logger = Logger(self.log_file, log_console=False)  # Initialize logger
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
    try:
        daemon = MemoryStatisticsDaemon()
        daemon.run()
    except Exception as e:
        logging.error(f"Fatal error in MemoryStatisticsDaemon: {e}")
        sys.exit(1)