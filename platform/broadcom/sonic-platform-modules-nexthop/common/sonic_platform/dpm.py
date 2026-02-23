# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Digital Power Manager (DPM) persistence and serialization.

This module provides device-independent functionality for storing and retrieving
DPM fault records in JSON format on the filesystem.
"""

import base64
import datetime
import json
import os
from typing import Any, Dict, List, Optional, Tuple


class Serializer:
    """Convert DPM fault records to and from JSON-safe format.

    Handles encoding of binary data (bytes) to base64 strings for JSON storage,
    and decoding back to original types when loading from disk.
    """

    @staticmethod
    def _encode_value(value: Any) -> Any:
        """Encode a single value to JSON-safe format.

        Args:
            value: Value to encode (bytes, int, float, str, None, or other)

        Returns:
            JSON-safe representation (base64 string for bytes, original for primitives)
        """
        if isinstance(value, (bytes, bytearray)):
            return 'base64:' + base64.b64encode(bytes(value)).decode('ascii')
        if isinstance(value, (int, float, str)) or value is None:
            return value
        return str(value)

    @staticmethod
    def _decode_value(value: Any) -> Any:
        """Decode a JSON-safe value back to original format.

        Args:
            value: JSON-safe value (potentially base64-encoded string)

        Returns:
            Original value (bytes for base64 strings, original for others)
        """
        if isinstance(value, str) and value.startswith('base64:'):
            return base64.b64decode(value[len('base64:'):])
        return value

    @staticmethod
    def encode_records(records: List[Dict]) -> List[Dict]:
        """Encode a list of fault records to JSON-safe format.

        Args:
            records: List of fault record dictionaries

        Returns:
            List of JSON-safe record dictionaries
        """
        out = []
        for rec in (records or []):
            obj = {}
            for key, value in (rec or {}).items():
                obj[key] = Serializer._encode_value(value)
            out.append(obj)
        return out

    @staticmethod
    def decode_records(json_records: List[Dict]) -> List[Dict]:
        """Decode JSON-safe records back to original format.

        Args:
            json_records: List of JSON-safe record dictionaries

        Returns:
            List of decoded record dictionaries with original types
        """
        out = []
        for rec in (json_records or []):
            obj = {}
            for key, value in (rec or {}).items():
                obj[key] = Serializer._decode_value(value)
            out.append(obj)
        return out


class SystemDPMLogHistory:
    """Manage persistent storage of system-wide DPM fault records.

    Stores fault records in JSON format with automatic retention management.
    Each file contains an envelope with metadata and encoded fault records.

    Envelope format:
        {
            "dpm_type": str,         # DPM device type (e.g., "adm1266")
            "gen_time": str,         # Timestamp in YYYY_MM_DD_HH_MM_SS format
            "schema_version": int,   # Format version (currently 1)
            "records_json": [...]    # List of JSON-safe fault records
        }

    Attributes:
        HISTORY_DIR: Directory path for storing history files
        PREVIOUS_FILE: Symlink name pointing to most recent file
        RETENTION: Maximum number of history files to retain
    """
    HISTORY_DIR = "/host/reboot-cause/nexthop"
    PREVIOUS_FILE = "previous-reboot-cause.json"
    RETENTION = 10

    def __init__(self):
        """Initialize history manager and load existing files.

        Creates history directory if needed, discovers existing history files,
        enforces retention policy, and updates symlink to latest file.
        """
        self.prev_link = os.path.join(self.HISTORY_DIR, self.PREVIOUS_FILE)
        os.makedirs(self.HISTORY_DIR, exist_ok=True)

        self._history_files = []
        for filename in os.listdir(self.HISTORY_DIR):
            if filename.startswith("reboot-cause-") and filename.endswith(".json"):
                self._history_files.append(os.path.join(self.HISTORY_DIR, filename))
        self._history_files.sort()

        while len(self._history_files) > self.RETENTION:
            self.remove_oldest_history()

        if self._history_files:
            self.update_latest_symlink()

    def get_timestamp(self, path: str) -> str:
        """Extract timestamp from a history file's envelope.

        Args:
            path: Path to history file

        Returns:
            Timestamp string from gen_time field, or empty string if not found
        """
        envelope = self._load_json_file(path)
        return envelope.get('gen_time', '')

    def add_history_file(self, path: str) -> None:
        """Add a new history file and enforce retention policy.

        Args:
            path: Path to history file to add
        """
        self._history_files.append(path)
        while len(self._history_files) > self.RETENTION:
            self.remove_oldest_history()

    @staticmethod
    def _delete_path(path: str) -> None:
        """Delete a file, suppressing all errors.

        Args:
            path: Path to file to delete
        """
        try:
            os.remove(path)
        except Exception:  # pylint: disable=broad-except
            pass

    def remove_oldest_history(self) -> Optional[str]:
        """Remove the oldest history file from cache and filesystem.

        Returns:
            Path of removed file, or None if no files exist
        """
        if not self._history_files:
            return None
        oldest = self._history_files.pop(0)
        self._delete_path(oldest)
        return oldest

    @staticmethod
    def _load_json_file(path: str) -> Dict:
        """Load and parse a JSON file.

        Args:
            path: Path to JSON file

        Returns:
            Parsed JSON dictionary, or empty dict on any error
        """
        try:
            with open(path, 'r', encoding='utf-8') as file_handle:
                return json.load(file_handle)
        except Exception:  # pylint: disable=broad-except
            return {}

    def save(self, dpm_type: str, all_faults: List[Dict]) -> None:
        """Save fault records to a new history file.

        Creates a timestamped file with an envelope containing the DPM type
        and encoded fault records. Updates the symlink and enforces retention.

        Args:
            dpm: DPM device type identifier (e.g., "adm1266")
            all_faults: List of fault record dictionaries to save
        """
        timestamp = datetime.datetime.utcnow().strftime('%Y_%m_%d_%H_%M_%S')
        filename = os.path.join(self.HISTORY_DIR, f"reboot-cause-{timestamp}.json")

        envelope = {
            'dpm_type': dpm_type,
            'gen_time': timestamp,
            'schema_version': 1,
            'records_json': Serializer.encode_records(all_faults),
        }

        with open(filename, 'w', encoding='utf-8') as file_handle:
            json.dump(envelope, file_handle, ensure_ascii=False)

        self.add_history_file(filename)
        self.update_latest_symlink()

    def update_latest_symlink(self) -> None:
        """Update symlink to point to the most recent history file."""
        if not self._history_files:
            return

        if os.path.exists(self.prev_link) or os.path.islink(self.prev_link):
            os.remove(self.prev_link)

        os.symlink(self._history_files[-1], self.prev_link)

    def load(self) -> Tuple[Optional[str], List[Dict]]:
        """Load fault records from the most recent history file.

        Returns:
            Tuple of (dpm_type, records) where:
                - dpm_type: DPM device type string, or None if file invalid
                - records: List of decoded fault record dictionaries
        """
        return self.load_file(self.prev_link)

    def load_file(self, path: str) -> Tuple[Optional[str], List[Dict]]:
        """Load fault records from a specific history file.

        Args:
            path: Path to history file to load

        Returns:
            Tuple of (dpm_type, records) where:
                - dpm_type: DPM device type string, or None if file invalid
                - records: List of decoded fault record dictionaries
        """
        envelope = self._load_json_file(path)
        if not isinstance(envelope, dict):
            return (None, [])

        dpm_type = envelope.get('dpm_type')
        records_json = envelope.get('records_json')

        if not isinstance(records_json, list):
            return (dpm_type, [])

        return (dpm_type, Serializer.decode_records(records_json))
