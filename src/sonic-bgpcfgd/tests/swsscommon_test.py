from unittest.mock import MagicMock


swsscommon = MagicMock(CFG_DEVICE_METADATA_TABLE_NAME = "DEVICE_METADATA")


def _is_common_name_valid(name):
    if not isinstance(name, str) or not name or len(name) > 15 or name in (".", ".."):
        return False

    for index, character in enumerate(name):
        is_alphanumeric = character.isascii() and character.isalnum()
        if is_alphanumeric or character in "_." or (index > 0 and character == "-"):
            continue
        return False

    return True


swsscommon.isInterfaceNameValid.side_effect = _is_common_name_valid
swsscommon.isVrfNameValid.side_effect = _is_common_name_valid
