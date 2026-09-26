# SPDX-License-Identifier: Apache-2.0

"""Concrete JSONL-backed blackbox logger.

Provides:
- SkippedEntries: marker for entries that were dropped from the recent
  circular buffer or persisted into history as a separator line.
- BlackBoxLogger: concrete three-tier (initial / recent / history) JSONL
  logger. Each entry is a plain JSON-serializable dict; the logger does not
  parse entries into records.

This three-tier design ensures that during a burst of N entries, the logger
retains the first initial_max and last recent_max entries for investigation.

Three-tier queue semantics:
- Initial tier (initial file): first initial_max entries since the last
  drain, append-only.
- Recent tier (recent file): last recent_max entries since the last drain,
  circular buffer that drops the oldest entry when full. First line is
  metadata: {SKIP_COUNTER_KEY: N}.
- History tier (history file): long-term log of up to history_max data
  entries. Populated by drain_to_history(), which moves the initial and
  recent tiers into history and then clears them. May contain
  {SKIP_COUNTER_KEY: N} separator lines between drained groups.
"""

import json
import os
import tempfile

from dataclasses import dataclass


@dataclass
class SkippedEntries:
    """Marker for entries that were skipped (lost to the circular buffer)."""

    count: int


class BlackBoxLogger:
    """Three-tier (initial / recent / history) JSONL blackbox logger.

    Concrete; instantiate directly with the storage parameters. Entries are
    plain JSON-serializable dicts. See module docstring for queue semantics.
    """

    # Key used for the skip-counter metadata line
    SKIP_COUNTER_KEY = "skipped_entries"

    # Per-tier filenames (joined with history_dir at access time)
    INITIAL_FILE = "initial.jsonl"
    RECENT_FILE = "recent.jsonl"
    HISTORY_FILE = "history.jsonl"

    def __init__(
        self,
        history_dir: str,
        initial_max: int,
        recent_max: int,
        history_max: int,
    ) -> None:
        self._history_dir = history_dir
        self._initial_max = initial_max
        self._recent_max = recent_max
        self._history_max = history_max

    @property
    def _initial_path(self) -> str:
        return os.path.join(self._history_dir, self.INITIAL_FILE)

    @property
    def _recent_path(self) -> str:
        return os.path.join(self._history_dir, self.RECENT_FILE)

    @property
    def _history_path(self) -> str:
        return os.path.join(self._history_dir, self.HISTORY_FILE)

    # ---------- low-level file IO ----------

    def _count_lines(self, filepath: str) -> int:
        """Counts lines in a file without loading all content."""
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                return sum(1 for _ in f)
        except FileNotFoundError:
            return 0

    def _read_raw_lines(self, filepath: str) -> list[str]:
        """Reads a file and returns all non-empty lines."""
        lines: list[str] = []
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if line:
                        lines.append(line)
        except FileNotFoundError:
            pass
        return lines

    def _is_skip_marker(self, obj) -> bool:
        """Returns whether obj is a skip-counter marker line."""
        return isinstance(obj, dict) and len(obj) == 1 and self.SKIP_COUNTER_KEY in obj

    def _append_entry(self, filepath: str, entry: dict) -> None:
        """Appends a single data entry as a JSON line to a file."""
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        with open(filepath, "a", encoding="utf-8") as f:
            f.write(json.dumps(entry) + "\n")

    def _load_entries(self, filepath: str) -> list[dict]:
        """Reads a JSONL file and returns all data entries (skipping non-data lines)."""
        entries: list[dict] = []
        for line in self._read_raw_lines(filepath):
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(obj, dict) and not self._is_skip_marker(obj):
                entries.append(obj)
        return entries

    def _atomic_write_lines(self, filepath: str, lines: list[str]) -> None:
        """Atomically writes lines to a file using write-to-temp + rename."""
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        fd, tmp_path = tempfile.mkstemp(dir=os.path.dirname(filepath))
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                for line in lines:
                    f.write(line + "\n")
            # allow root to read/write, group/others to read.
            os.chmod(tmp_path, 0o644)
            os.rename(tmp_path, filepath)
        except Exception:
            os.unlink(tmp_path)
            raise

    # ---------- per-tier load/save ----------

    def _load_initial(self) -> list[dict]:
        """Loads entries from the `initial` file."""
        return self._load_entries(self._initial_path)

    def _load_recent(self) -> tuple[list[dict], int]:
        """Loads from the `recent` file, returning (entries, skipped_entries)."""
        skipped = 0
        lines = self._read_raw_lines(self._recent_path)
        if lines:
            try:
                meta = json.loads(lines[0])
                if self._is_skip_marker(meta):
                    skipped = meta[self.SKIP_COUNTER_KEY]
            except json.JSONDecodeError:
                pass
        return self._load_entries(self._recent_path), skipped

    def _save_recent(self, skipped: int, entries: list[dict]) -> None:
        """Writes recent entries with metadata first line + data entries."""
        lines = [json.dumps({self.SKIP_COUNTER_KEY: skipped})]
        lines.extend(json.dumps(e) for e in entries)
        self._atomic_write_lines(self._recent_path, lines)

    def _load_history(self) -> tuple[list[dict | SkippedEntries], int]:
        """Loads history log preserving skip markers between entries.

        Returns:
            A tuple of (items, total_skipped) where items is a list of json dict
            entries interleaved with SkippedEntries markers, and total_skipped
            is the sum of all skipped entries.
        """
        items: list[dict | SkippedEntries] = []
        total_skipped = 0
        for line in self._read_raw_lines(self._history_path):
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            if not isinstance(obj, dict):
                continue
            if self._is_skip_marker(obj):
                total_skipped += obj[self.SKIP_COUNTER_KEY]
                items.append(SkippedEntries(count=obj[self.SKIP_COUNTER_KEY]))
            else:
                items.append(obj)
        return items, total_skipped

    def _write_to_history(self, new_lines: list[str], prepend: bool) -> None:
        """Merges new lines into the history log, trims, and atomically writes.

        Args:
            new_lines: JSON lines to add.
            prepend: If True, new lines go before existing; otherwise after.
        """
        existing_lines = self._read_raw_lines(self._history_path)
        if prepend:
            all_lines = new_lines + existing_lines
        else:
            all_lines = existing_lines + new_lines
        all_lines = self._trim_history_lines(all_lines)
        self._atomic_write_lines(self._history_path, all_lines)

    def _is_data_line(self, line: str) -> bool:
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            return False
        return isinstance(obj, dict) and not self._is_skip_marker(obj)

    def _trim_history_lines(self, raw_lines: list[str]) -> list[str]:
        """Trims history to history_max data entries (counting from newest).

        Separator lines don't count toward the limit. If trimming leaves a
        separator as the first line, it is dropped.
        """
        kept = 0
        cutoff = -1
        # Walk backwards to find where the last HISTORY_MAX data entries start
        for i in range(len(raw_lines) - 1, -1, -1):
            if self._is_data_line(raw_lines[i]):
                kept += 1
                if kept >= self._history_max:
                    cutoff = i
                    break

        if cutoff == -1:
            return raw_lines

        # Slice from cutoff, drop leading separator if present
        result = raw_lines[cutoff:]
        if result and not self._is_data_line(result[0]):
            result = result[1:]
        return result

    # ---------- public API ----------

    def save_data(self, data: dict) -> None:
        """Saves an entry. Pushes to initial first; once full, circular into recent."""
        os.makedirs(self._history_dir, exist_ok=True)

        # Try initial entries first
        initial_count = self._count_lines(self._initial_path)
        if initial_count < self._initial_max:
            self._append_entry(self._initial_path, data)
            return

        # Initial entries full, push to recent entries
        recent_entries, skipped = self._load_recent()
        recent_entries.append(data)
        if len(recent_entries) > self._recent_max:
            recent_entries.pop(0)
            skipped += 1
        self._save_recent(skipped, recent_entries)

    def clear_log(self) -> None:
        """Removes the initial, recent, and history files."""
        for path in (self._initial_path, self._recent_path, self._history_path):
            if os.path.exists(path):
                os.unlink(path)

    def drain_to_history(self) -> None:
        """Drains initial + recent into history (with skip marker if any)."""
        initial_entries = self._load_initial()
        recent_entries, skipped = self._load_recent()
        if not initial_entries and not recent_entries:
            return

        # Build new lines: initial + separator (if skipped) + recent
        new_lines = [json.dumps(e) for e in initial_entries]
        if skipped > 0:
            new_lines.append(json.dumps({self.SKIP_COUNTER_KEY: skipped}))
        new_lines.extend(json.dumps(e) for e in recent_entries)

        # Append to history log and trim
        self._write_to_history(new_lines, prepend=False)

        # Clear initial/recent entries logs
        for path in (self._initial_path, self._recent_path):
            if os.path.exists(path):
                os.unlink(path)

    def load(self) -> dict | None:
        """Returns the most recent entry: recent -> initial -> history."""
        # Check recent entries last entry
        recent_entries, _ = self._load_recent()
        if recent_entries:
            return recent_entries[-1]

        # Check initial entries last entry
        initial_entries = self._load_initial()
        if initial_entries:
            return initial_entries[-1]

        # Check history last entry
        history_items, _ = self._load_history()
        for item in reversed(history_items):
            if isinstance(item, dict):
                return item
        return None

    def load_all(self) -> tuple[list[dict | SkippedEntries], int, int]:
        """Returns (items, total, skipped) in chronological order.

        items spans history + initial + recent, interleaved with SkippedEntries
        markers where applicable.

        Returns:
            A tuple of (items, total, skipped) where:
            - items: All data entries in chronological order (oldest to newest)
                     from history + initial entries + recent entries, interleaved with
                     SkippedEntries markers where entries were skipped.
            - total: Total number of entries including skipped ones.
            - skipped: Number of entries that were skipped and not shown.
        """
        history_items, history_skipped = self._load_history()
        initial_entries = self._load_initial()
        recent_entries, recent_skipped = self._load_recent()

        # Bundle history + initial + recent (with skip markers)
        all_items: list[dict | SkippedEntries] = list(history_items)
        all_items.extend(initial_entries)
        if recent_skipped > 0:
            all_items.append(SkippedEntries(count=recent_skipped))
        all_items.extend(recent_entries)

        skipped = history_skipped + recent_skipped
        entry_count = sum(1 for item in all_items if isinstance(item, dict))
        total = entry_count + skipped

        return all_items, total, skipped
