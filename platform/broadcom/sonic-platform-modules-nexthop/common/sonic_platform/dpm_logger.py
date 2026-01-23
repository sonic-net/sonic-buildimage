# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Digital Power Manager (DPM) file logger.

This module provides device-independent functionality for storing and retrieving
reboot causes and DPM records in JSON format on the filesystem.
"""

import datetime
import json
import os

from dataclasses import asdict, dataclass
from pathlib import Path
from sonic_platform.dpm_base import DpmBase, DpmPowerUpEntry, RebootCause, timestamp_as_string
from typing import Iterable


@dataclass
class DataBase:
    gen_time: str
    schema_version: int


@dataclass
class CauseV1:
    source: str
    timestamp: str
    cause: str
    description: str


@dataclass
class DpmV1:
    name: str
    type: str
    records: list[dict[str, str]]


@dataclass
class DataV1(DataBase):
    causes: list[CauseV1]
    dpms: list[DpmV1]

    def __post_init__(self):
        # Convert causes dicts into CauseV1 objects
        self.causes = [c if isinstance(c, CauseV1) else CauseV1(**c) for c in self.causes]
        # Convert dpms dicts into DpmV1 objects
        self.dpms = [d if isinstance(d, DpmV1) else DpmV1(**d) for d in self.dpms]


UNKNOWN_DATA = DataBase(gen_time="", schema_version=-1)


def load_data_from_file(path: Path | str) -> DataBase:
    """Loads and parses a JSON file into a structured Data object.

    Args:
        path: Path to JSON file.

    Returns:
        Parsed Data object, or UNKNOWN_DATA if parsing fails.
    """
    if isinstance(path, str):
        path = Path(path)

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if data.get("schema_version") == 1:
            return DataV1(**data)
    except Exception:
        pass

    return UNKNOWN_DATA


class DpmLogger:
    """Manages persistent storage of system-wide reboot causes and DPM records.

    Stores the data as a series of JSON files on the filesystem. Each file
    contains an envelope with metadata and encoded reboot causes and DPM records.

    Currently, only schema_version 1 (`DataV1`) is supported.

    Attributes:
        HISTORY_DIR: Directory path for storing history files
        PREVIOUS_FILE: Symlink name pointing to most recent file
        RETENTION: Maximum number of history files to retain
    """

    HISTORY_DIR = "/host/reboot-cause/nexthop"
    PREVIOUS_FILE = "previous-reboot-cause.json"
    RETENTION = 50

    def __init__(self) -> None:
        """Initializes the DPM logger."""
        self.prev_link = os.path.join(self.HISTORY_DIR, self.PREVIOUS_FILE)
        os.makedirs(self.HISTORY_DIR, exist_ok=True)

    def _get_sorted_history_files(self) -> list[Path]:
        """Returns list of history files present on the system.

        Returns:
            Sorted list of history file paths (oldest to newest).
        """
        history_dir = Path(self.HISTORY_DIR)
        if not history_dir.is_dir():
            return []

        return sorted(
            file_path
            for file_path in history_dir.iterdir()
            if file_path.name.startswith("reboot-cause-") and file_path.name.endswith(".json")
        )

    def _enforce_retention_policy(self) -> None:
        """Enforces retention policy by removing old history files.

        Removes oldest files until the file count is within RETENTION limit.
        """
        history_files = self._get_sorted_history_files()
        while len(history_files) > self.RETENTION:
            history_files[0].unlink(missing_ok=True)
            history_files.pop(0)

    def _update_symlink_to(self, target: str) -> None:
        """Updates symlink to point to the specified target file."""
        if os.path.exists(self.prev_link) or os.path.islink(self.prev_link):
            os.remove(self.prev_link)
        os.symlink(target, self.prev_link)

    def save(
        self,
        causes: Iterable[RebootCause],
        dpm_to_powerups: dict[DpmBase, list[DpmPowerUpEntry]],
    ) -> None:
        """Saves causes and DPM records to a new history file.

        Creates a DataV1 object and writes it to a timestamped file in JSON format.

        Args:
            causes: List of RebootCause objects, as observed from
                    SW reboot cause and HW DPM records.
            dpm_to_powerups: Dictionary of DPM to list of powerups,
                             where each powerup contains a list of DPM records.
        """
        gen_time = datetime.datetime.now(tz=datetime.timezone.utc)
        filename = os.path.join(
            self.HISTORY_DIR, f"reboot-cause-{gen_time.strftime('%Y_%m_%d_%H_%M_%S')}.json"
        )
        data = DataV1(
            gen_time=gen_time.strftime("%Y-%m-%d %H:%M:%S %Z"),
            schema_version=1,
            causes=[
                CauseV1(
                    c.source,
                    timestamp_as_string(c.timestamp),
                    c.cause,
                    c.description if c.type == RebootCause.Type.HARDWARE else "n/a",
                )
                for c in causes
            ],
            dpms=[
                DpmV1(
                    dpm.get_name(),
                    dpm.get_type().value,
                    [record.as_dict() for powerup in powerups for record in powerup.dpm_records],
                )
                for dpm, powerups in dpm_to_powerups.items()
            ],
        )

        with open(filename, "w", encoding="utf-8") as file_handle:
            json.dump(asdict(data), file_handle)

        self._enforce_retention_policy()
        self._update_symlink_to(filename)

    def load(self) -> DataBase:
        """Loads log data from the most recent history file.

        Caller can try casting to DataV1 for schema_version 1, and etc.

        Returns:
            The newest DataBase decoded from the newest history file,
            or UNKNOWN_DATA if no history files are present or if parsing fails.
        """
        return load_data_from_file(self.prev_link)

    def load_all(self) -> list[DataBase]:
        """Loads log data from all history files.

        For each object, caller can try casting to DataV1 for schema_version 1, and etc.

        Returns:
            List of DataBase objects, one per history file, sorted from oldest to newest.
        """
        return [load_data_from_file(path) for path in self._get_sorted_history_files()]
