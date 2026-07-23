#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared test helpers for ADM1266 testing.

This module contains common utilities used by both unit and integration tests
for ADM1266 functionality. It helps avoid code duplication between test files.
"""

import datetime

# This must match the value in adm1266.py
EXPECTED_CUSTOM_EPOCH = datetime.datetime(2024, 1, 1, tzinfo=datetime.timezone.utc)


def create_raw_adm1266_blackbox_record(
    uid: int,
    empty: bool = False,
    action_index: int = 0,
    rule_index: int = 0,
    vh_ov: int = 0,
    vh_uv: int = 0,
    current_state: int = 9,
    last_state: int = 8,
    vp_ov: int = 0,
    vp_uv: int = 0,
    gpio_in: int = 0,
    gpio_out: int = 0,
    pdio_in: int = 0,
    pdio_out: int = 0,
    powerup_counter: int = 1,
    timestamp: datetime.timedelta | datetime.datetime = datetime.timedelta(seconds=0),
    crc: int = 0xFF,
) -> bytes:
    """Returns a raw 64-byte blackbox record containing the given values."""
    record = bytearray(64)
    record[0] = uid & 0xFF
    record[1] = (uid >> 8) & 0xFF
    record[2] = 0b1 if empty else 0b0
    record[3] = action_index & 0xFF
    record[4] = rule_index & 0xFF
    record[5] = (vh_ov & 0b1111) | ((vh_uv & 0b1111) << 4)
    record[6] = current_state & 0xFF
    record[7] = (current_state >> 8) & 0xFF
    record[8] = last_state & 0xFF
    record[9] = (last_state >> 8) & 0xFF
    record[10] = vp_ov & 0xFF
    record[11] = (vp_ov >> 8) & 0xFF
    record[12] = vp_uv & 0xFF
    record[13] = (vp_uv >> 8) & 0xFF
    record[14] = gpio_in & 0xFF
    record[15] = (gpio_in >> 8) & 0xFF
    record[16] = gpio_out & 0xFF
    record[17] = (gpio_out >> 8) & 0xFF
    record[18] = pdio_in & 0xFF
    record[19] = (pdio_in >> 8) & 0xFF
    record[20] = pdio_out & 0xFF
    record[21] = (pdio_out >> 8) & 0xFF
    record[22] = powerup_counter & 0xFF
    record[23] = (powerup_counter >> 8) & 0xFF
    # Timestamp
    if isinstance(timestamp, datetime.datetime):
        assert timestamp >= EXPECTED_CUSTOM_EPOCH, f"Timestamp {timestamp} must be after {EXPECTED_CUSTOM_EPOCH}."
        delta = timestamp - EXPECTED_CUSTOM_EPOCH
    else:
        delta = timestamp
    seconds = int(delta.total_seconds())
    fraction = int(delta.microseconds / 1000000 * 65536)
    record[24] = fraction & 0xFF
    record[25] = (fraction >> 8) & 0xFF
    record[26] = seconds & 0xFF
    record[27] = (seconds >> 8) & 0xFF
    record[28] = (seconds >> 16) & 0xFF
    record[29] = (seconds >> 24) & 0xFF
    record[30] = 0x0
    record[31] = 0x0
    # Reserved
    record[32:] = b"\xff" * 32
    # CRC-8
    record[63] = crc & 0xFF
    return bytes(record)
