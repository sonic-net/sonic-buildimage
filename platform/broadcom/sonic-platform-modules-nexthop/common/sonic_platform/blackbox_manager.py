# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Device-independent blackbox manager."""

import datetime

from dataclasses import dataclass

from sonic_platform_base.device_base import DeviceBase
from sonic_platform.blackbox_logger import BlackBoxLogger, SkippedEntries
from sonic_platform.blackbox_record_base import BlackBoxRecordBase

SCHEMA_VERSION = 1
_GEN_TIME_FMT = "%Y-%m-%d %H:%M:%S %Z"

# Sentinel source value selecting every configured source.
ALL_SOURCES = "all"

_INITIAL_MAX = 5
_RECENT_MAX = 5
_HISTORY_MAX = 20


@dataclass
class BlackBoxSnapshot:
    log_type: str
    gen_time: str
    schema_version: int
    payload: dict

    def as_dict(self) -> dict:
        return {
            "log_type": self.log_type,
            "gen_time": self.gen_time,
            "schema_version": self.schema_version,
            "payload": self.payload,
        }


class BlackBoxManager:
    def __init__(
        self,
        pddf_data = None,
        pddf_plugin_data = None,
        devices: dict[str, list[DeviceBase]] = {},
    ):
        blackbox_cfg = (pddf_plugin_data or {}).get("BLACKBOX", {})
        if devices.keys() != blackbox_cfg.keys():
            raise ValueError(
                f"devices and blackbox cfg must have matching keys: "
                f"{sorted(devices.keys())} != {sorted(blackbox_cfg.keys())}"
            )

        self._pddf_data = pddf_data
        self._pddf_plugin_data = pddf_plugin_data
        self._devices = devices
        self._loggers = {}
        self._blackbox_filter = {}
        for source, cfg in blackbox_cfg.items():
            if "log_path" not in cfg or "discard_duplicate_logs" not in cfg:
                continue
            self._blackbox_filter[source] = cfg["discard_duplicate_logs"]
            self._loggers[source] = BlackBoxLogger(
                history_dir=cfg["log_path"],
                initial_max=_INITIAL_MAX,
                recent_max=_RECENT_MAX,
                history_max=_HISTORY_MAX,
            )

    @property
    def sources(self) -> list[str]:
        """Returns the configured blackbox source names."""
        return list(self._loggers.keys())

    def get_logger(self, source: str) -> BlackBoxLogger | None:
        """Returns the logger for a source, or None if unknown."""
        return self._loggers.get(source)

    def _now(self) -> str:
        """Returns the current UTC time formatted for gen_time."""
        return datetime.datetime.now(tz=datetime.timezone.utc).strftime(_GEN_TIME_FMT)

    def fetch_blackbox(self, source: str, clear_blackbox: bool = False) -> dict[str, dict]:
        """Reads and encodes device blackboxes into per-source payloads.

        One source, or every source if 'all'.
        """
        payloads: dict[str, dict] = {}
        for src, device_list in self._devices.items():
            if source != ALL_SOURCES and src != source:
                continue

            dev_records = {}
            for dev in device_list:
                try:
                    raw_records = dev.get_blackbox_raw()
                    records = dev.decode_blackbox_records(raw_records)
                except (AttributeError, NotImplementedError):
                    # device doesn't implement the blackbox interface
                    continue

                if records:
                    dev_records[f"{dev.get_name()}:{dev.get_model()}"] = [
                        record.get_raw().hex() for record in records
                    ]
                    if clear_blackbox:
                        try:
                            dev.clear_blackbox()
                        except (AttributeError, NotImplementedError, OSError):
                            # A clear failure must not discard records already read.
                            pass

            if dev_records:
                payloads[src] = dev_records
        return payloads

    def clear_blackbox(self, source: str) -> None:
        """Clears device blackboxes. One source, or every source if 'all'."""
        for src, device_list in self._devices.items():
            if source != ALL_SOURCES and src != source:
                continue
            for dev in device_list:
                try:
                    dev.clear_blackbox()
                except (AttributeError, NotImplementedError):
                    # device doesn't implement the blackbox interface
                    continue

    def build_snapshot(self, source: str, payload: dict) -> BlackBoxSnapshot:
        """Wraps a payload in a timestamped snapshot for the given source."""
        return BlackBoxSnapshot(
            log_type=source,
            gen_time=self._now(),
            schema_version=SCHEMA_VERSION,
            payload=payload,
        )

    def write_blackbox_log(self, source: str, payload: dict) -> bool:
        """Logs only the device records not already present in the source's log.
        Returns True if anything was written, False otherwise.

        For a filtered source, each record is compared against every record
        already logged for its device, so unchanged device blackboxes (e.g. DCDC
        records that can't be cleared) aren't re-logged on every poll while new
        fault records are still captured. Comparing against the whole logged
        history -- not just the last snapshot -- is what keeps a device with
        multiple records from oscillating as partial snapshots are written.
        """
        logger = self._loggers.get(source)
        if logger is None or not payload:
            return False

        if self._blackbox_filter.get(source):
            seen = self._logged_records(source)
            new_records: dict[str, list[str]] = {}
            for device_key, records in payload.items():
                for record in records:
                    if record not in seen.get(device_key, ()):
                        new_records.setdefault(device_key, []).append(record)
            if not new_records:
                return False
            payload = new_records

        logger.save_data(self.build_snapshot(source, payload).as_dict())
        return True

    def _logged_records(self, source: str) -> dict[str, set[str]]:
        """Every raw-hex record already logged for the source, keyed by device."""
        snapshot_list, _, _ = self.read_all_blackbox_logs(source)
        seen: dict[str, set[str]] = {}
        for snapshot in snapshot_list:
            if isinstance(snapshot, SkippedEntries):
                continue
            for device_key, records in snapshot.payload.items():
                seen.setdefault(device_key, set()).update(records)
        return seen

    def read_blackbox_log(self, source: str) -> BlackBoxSnapshot | None:
        """Returns the most recent snapshot for a source, or None."""
        logger = self._loggers.get(source)
        if logger is None:
            return None

        snapshot = logger.load()
        if snapshot is None:
            return None

        return BlackBoxSnapshot(**snapshot)

    def read_all_blackbox_logs(
        self, source: str
    ) -> tuple[list[BlackBoxSnapshot | SkippedEntries], int, int]:
        """Returns all snapshots for a source with total and skipped counts."""
        logger = self._loggers.get(source)
        if logger is None:
            return [], 0, 0

        items, total, skipped = logger.load_all()
        snapshots: list[BlackBoxSnapshot | SkippedEntries] = [
            item if isinstance(item, SkippedEntries) else BlackBoxSnapshot(**item)
            for item in items
        ]
        return snapshots, total, skipped

    def decode_blackbox_snapshot(self, snapshot: BlackBoxSnapshot) -> dict[str, list[BlackBoxRecordBase]]:
        """Decodes a snapshot's hex payloads into per-device blackbox records."""
        result: dict[str, list[BlackBoxRecordBase]] = {}
        for device_key, raw_hexes in snapshot.payload.items():
            name, model = device_key.split(":", 1)
            record_cls = BlackBoxRecordBase.get_record_class(model)
            if record_cls is None:
                # Fallback to log_type for matching to devices like PSUs which have
                # a standardized record format.
                record_cls = BlackBoxRecordBase.get_record_class(snapshot.log_type)
                if record_cls is None:
                    raise ValueError(
                        f"No blackbox record class registered for model '{model}' "
                        f"(device '{name}')"
                    )
            result[name] = [
                record_cls.from_bytes(bytes.fromhex(raw_hex), device_key)
                for raw_hex in raw_hexes
            ]
        return result

    def rotate_blackbox_logs(self, source: str) -> None:
        """Drains initial+recent into history. One source, or every source if 'all'."""
        if source != ALL_SOURCES and source not in self._loggers:
            raise ValueError(f"Unknown blackbox source: '{source}'. Valid options are: {list(self._loggers.keys())}")

        targets = self._loggers.values() if source == ALL_SOURCES else [self._loggers[source]]
        for logger in targets:
            logger.drain_to_history()

    def clear_blackbox_log(self, source: str) -> None:
        """Clears logged snapshots. One source, or every source if 'all'."""
        if source != ALL_SOURCES and source not in self._loggers:
            raise ValueError(f"Unknown blackbox source: '{source}'. Valid options are: {list(self._loggers.keys())}")

        targets = self._loggers.values() if source == ALL_SOURCES else [self._loggers[source]]
        for logger in targets:
            logger.clear_log()
