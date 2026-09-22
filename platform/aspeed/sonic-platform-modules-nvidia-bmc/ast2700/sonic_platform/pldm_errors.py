#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
PLDM for Firmware Update (DSP0267 1.3.0) error code tables and annotation.

Translates numeric codes emitted by pldm-fw into human-readable descriptions
sourced from the DMTF DSP0267 1.3.0 specification.
"""

import re

# ---------------------------------------------------------------------------
# DSP0240 base completion codes (subset used in firmware update)
# ---------------------------------------------------------------------------
_BASE_CODES = {
    0x00: ("SUCCESS",                   "Command completed successfully"),
    0x01: ("ERROR",                     "General error"),
    0x02: ("ERROR_INVALID_DATA",        "Invalid data in the request"),
    0x03: ("ERROR_INVALID_LENGTH",      "Invalid message length"),
    0x04: ("ERROR_NOT_READY",           "Receiver not ready"),
    0x05: ("ERROR_UNSUPPORTED_PLDM_CMD", "Command not supported by this PLDM type"),
    0x20: ("ERROR_INVALID_PLDM_TYPE",   "Invalid PLDM type"),
}

# ---------------------------------------------------------------------------
# DSP0267 1.3.0 Table 1 – PLDM Firmware Update Completion Codes
# ---------------------------------------------------------------------------
COMPLETION_CODES = {
    **_BASE_CODES,
    0x80: ("NOT_IN_UPDATE_MODE",
           "Received PLDM firmware update command when the FD/FDP is not in update mode"),
    0x81: ("ALREADY_IN_UPDATE_MODE",
           "FD/FDP received RequestUpdate when it is already in update mode"),
    0x82: ("DATA_OUT_OF_RANGE",
           "The requested component image portion has an initial offset not contained within "
           "the image data, or the offset plus length exceeds the range permitted by the UA"),
    0x83: ("INVALID_TRANSFER_LENGTH",
           "The requested component image portion length exceeds MaximumTransferSize or is "
           "less than the firmware update baseline transfer size"),
    0x84: ("INVALID_STATE_FOR_COMMAND",
           "The FD/FDP is not in a state to expect this command"),
    0x85: ("INCOMPLETE_UPDATE",
           "One or more component transfers failed to complete"),
    0x86: ("BUSY_IN_BACKGROUND",
           "The FD/FDP is performing a critical background task and cannot execute the command"),
    0x87: ("CANCEL_PENDING",
           "UA received a RequestFirmwareData command after sending CancelUpdate or "
           "CancelUpdateComponent"),
    0x88: ("COMMAND_NOT_EXPECTED",
           "UA received a command from the FD/FDP out of sequence"),
    0x89: ("RETRY_REQUEST_FW_DATA",
           "UA requested a retry of RequestFirmwareData; it needs more time to retrieve "
           "the firmware section to transfer"),
    0x8A: ("UNABLE_TO_INITIATE_UPDATE",
           "The FD/FDP is not able to enter update mode to begin a transfer"),
    0x8B: ("ACTIVATION_NOT_REQUIRED",
           "The FD/FDP has already enabled firmware components to become active on the next "
           "external activation, or the components are already activated"),
    0x8C: ("SELF_CONTAINED_ACTIVATION_NOT_PERMITTED",
           "The firmware device does not permit self-contained activation"),
    0x8D: ("NO_DEVICE_METADATA",
           "The FD/FDP has no metadata that must be retrieved by the UA prior to component "
           "image transfers"),
    0x8E: ("RETRY_REQUEST_UPDATE",
           "The FD/FDP has requested a retry of RequestUpdate; it needs more time to prepare "
           "for a firmware update"),
    0x8F: ("NO_PACKAGE_DATA",
           "The UA has no package data available for the firmware device"),
    0x90: ("INVALID_TRANSFER_HANDLE",
           "The data transfer handle requested was invalid"),
    0x91: ("INVALID_TRANSFER_OPERATION_FLAG",
           "The transfer operation flag used in the request was invalid"),
    0x92: ("ACTIVATE_PENDING_IMAGE_NOT_PERMITTED",
           "The firmware device does not support activating a pending component image or "
           "component image set"),
    0x93: ("PACKAGE_DATA_ERROR",
           "The FD/FDP received invalid Package Data and will not proceed with the firmware "
           "update"),
    0x94: ("NO_OPAQUE_DATA",
           "The UA has no component opaque data available for the firmware device"),
    0x95: ("UPDATE_SECURITY_REVISION_NOT_PERMITTED",
           "The FD/FDP does not support updating the security revision number of the component "
           "image"),
    0x96: ("DOWNSTREAM_DEVICE_LIST_CHANGED",
           "The FD/FDP must end the transfer as one or more downstream devices were added or "
           "removed during the inventory transfer"),
}

# ---------------------------------------------------------------------------
# DSP0267 1.3.0 Table 35 – TransferResult codes (TransferComplete command)
# ---------------------------------------------------------------------------
TRANSFER_RESULT_CODES = {
    0x00: ("TRANSFER_SUCCESS",
           "Transfer has completed without error"),
    0x01: ("TRANSFER_IMAGE_CORRUPT",
           "Transfer completed with error: the image received is corrupt"),
    0x02: ("TRANSFER_VERSION_MISMATCH",
           "Transfer completed with error: the image version does not match the version "
           "expected from the UpdateComponent command"),
    0x03: ("TRANSFER_FD_ABORTED",
           "Firmware Device has aborted the transfer"),
    0x09: ("TRANSFER_TIMEOUT",
           "Timeout occurred while performing action"),
    0x0A: ("TRANSFER_GENERIC_ERROR",
           "Generic error has occurred"),
    0x0B: ("TRANSFER_LOW_POWER_ABORT",
           "FD/FDP aborted the transfer because it must enter a low-power state"),
    0x0C: ("TRANSFER_RESET_ABORT",
           "FD/FDP aborted the transfer because it must perform a reset"),
    0x0D: ("TRANSFER_STORAGE_ERROR",
           "FD/FDP aborted the transfer due to an issue storing the firmware data on the "
           "device"),
    0x0E: ("TRANSFER_INVALID_OPAQUE_DATA",
           "FD/FDP aborted the transfer due to invalid ComponentOpaqueData"),
    0x0F: ("TRANSFER_DOWNSTREAM_FAILURE",
           "FD/FDP aborted the transfer because one or more downstream devices of the same "
           "type being updated could not complete the transfer"),
    0x10: ("TRANSFER_SECURITY_REVISION_ERROR",
           "Transfer aborted: the image will not be updated due to a security revision error"),
}

# ---------------------------------------------------------------------------
# DSP0267 1.3.0 Table 36 – VerifyResult codes (VerifyComplete command)
# ---------------------------------------------------------------------------
VERIFY_RESULT_CODES = {
    0x00: ("VERIFY_SUCCESS",
           "Verify has completed without error"),
    0x01: ("VERIFY_FAILURE",
           "Verify completed with a verification failure; FD will not apply the component"),
    0x02: ("VERIFY_VERSION_MISMATCH",
           "Verify completed with error: image version does not match the version expected "
           "from the UpdateComponent command; FD will not apply the component"),
    0x03: ("VERIFY_SECURITY_CHECK_FAILED",
           "Verify completed with error: image failed the FD security checks; FD will not "
           "apply the component"),
    0x04: ("VERIFY_INCOMPLETE_IMAGE",
           "Verify completed with error: the image transferred was incomplete; FD will not "
           "apply the component"),
    0x09: ("VERIFY_TIMEOUT",
           "Timeout occurred while performing action; FD will not apply the component"),
    0x0A: ("VERIFY_GENERIC_ERROR",
           "Generic error has occurred; FD will not apply the component"),
    0x10: ("VERIFY_SECURITY_REVISION_ERROR",
           "Verify completed with error: image will not be updated due to a security revision "
           "error"),
}

# ---------------------------------------------------------------------------
# DSP0267 1.3.0 Table 37 – ApplyResult codes (ApplyComplete command)
# ---------------------------------------------------------------------------
APPLY_RESULT_CODES = {
    0x00: ("APPLY_SUCCESS",
           "Apply has completed without error"),
    0x01: ("APPLY_SUCCESS_ACTIVATION_MODIFIED",
           "Apply completed successfully; the FD has modified its activation method — see "
           "ComponentActivationMethodsModification field"),
    0x02: ("APPLY_MEMORY_WRITE_FAILURE",
           "Apply completed with failure due to a memory write issue"),
    0x09: ("APPLY_TIMEOUT",
           "Timeout occurred while performing action"),
    0x0A: ("APPLY_GENERIC_ERROR",
           "Generic error has occurred"),
    0x0B: ("APPLY_RETRY_REQUESTED",
           "Apply was not attempted but could succeed if the UA re-initiates the transfer; "
           "use CancelUpdate and restart"),
    0x10: ("APPLY_SECURITY_REVISION_ERROR",
           "Apply completed with error: image will not be updated due to a security revision "
           "error"),
}

# ---------------------------------------------------------------------------
# DSP0267 1.3.0 – ComponentResponseCode (PassComponentTable response)
# ---------------------------------------------------------------------------
COMPONENT_RESPONSE_CODES = {
    0x00: ("COMPONENT_CAN_BE_UPDATED",
           "Component can be updated"),
    0x01: ("COMPONENT_COMPARISON_STAMP_IDENTICAL",
           "Component comparison stamp is identical to the firmware in the FD/downstream "
           "device; set Force update flag in UpdateComponent to proceed"),
    0x02: ("COMPONENT_COMPARISON_STAMP_LOWER",
           "Component comparison stamp is lower than the firmware in the FD/downstream "
           "device; set Force update flag in UpdateComponent to proceed"),
    0x03: ("INVALID_COMPARISON_STAMP",
           "Invalid component comparison stamp"),
    0x04: ("COMPONENT_CONFLICT",
           "Component conflicts with another component provided in a separate "
           "PassComponentTable command"),
    0x05: ("PREREQUISITES_NOT_MET",
           "Pre-requisites for this component have not been met"),
    0x06: ("COMPONENT_NOT_SUPPORTED",
           "Component is not supported on the FD or downstream device"),
    0x07: ("SECURITY_DOWNGRADE_RESTRICTED",
           "Security restrictions prevent component from being downgraded"),
    0x08: ("INCOMPLETE_COMPONENT_IMAGE_SET",
           "Incomplete component image set was received; all UpdateComponent commands will "
           "be rejected"),
    0x09: ("ACTIVE_IMAGE_NOT_RESTORABLE",
           "If this new component image is activated, the FD/downstream device will not be "
           "able to subsequently update to the currently running active component image"),
    0x0A: ("COMPONENT_VERSION_STRING_IDENTICAL",
           "Component version string is identical to the firmware version in the "
           "FD/downstream device; set Force update flag in UpdateComponent to proceed"),
    0x0B: ("COMPONENT_VERSION_STRING_LOWER",
           "Component version string is lower than the firmware version in the "
           "FD/downstream device; set Force update flag in UpdateComponent to proceed"),
}

# ---------------------------------------------------------------------------
# Context-keyword → table mapping for annotate()
# ---------------------------------------------------------------------------
_CONTEXT_TABLES = [
    (re.compile(r'\btransfer\b', re.IGNORECASE), TRANSFER_RESULT_CODES),
    (re.compile(r'\bverif(?:y|ication)\b', re.IGNORECASE), VERIFY_RESULT_CODES),
    (re.compile(r'\bapply\b', re.IGNORECASE), APPLY_RESULT_CODES),
    (re.compile(r'\bcomponent\s*response\b', re.IGNORECASE), COMPONENT_RESPONSE_CODES),
]
_CONTEXT_WINDOW = 60  # characters to look before the hex code for context keywords


def _lookup(val, context=""):
    """Return (name, description) for *val* using *context* to pick the table.

    Context keywords within ``_CONTEXT_WINDOW`` characters before the code are
    used to select the most appropriate result table.  Falls back to the
    completion code table (Table 1) when no keyword matches.
    """
    for pattern, table in _CONTEXT_TABLES:
        if pattern.search(context) and val in table:
            return table[val]
    return COMPLETION_CODES.get(val)


def annotate(text):
    """Return *text* with PLDM code values annotated with spec descriptions.

    Scans for ``0xNN`` byte-sized hex literals and appends
    ``(SYMBOLIC_NAME: human-readable description)`` from DSP0267 where known.
    Unrecognised codes are left unchanged.

    Example::

        >>> annotate("failed: completion code 0x84")
        "failed: completion code 0x84 (INVALID_STATE_FOR_COMMAND: The FD/FDP is not in a state to expect this command)"
    """
    def _replace(m):
        val = int(m.group(0), 16)
        start = max(0, m.start() - _CONTEXT_WINDOW)
        context = text[start:m.start()]
        entry = _lookup(val, context)
        if entry:
            name, desc = entry
            return f"{m.group(0)} ({name}: {desc})"
        return m.group(0)

    return re.sub(r'0x[0-9a-fA-F]{2}\b', _replace, text)
