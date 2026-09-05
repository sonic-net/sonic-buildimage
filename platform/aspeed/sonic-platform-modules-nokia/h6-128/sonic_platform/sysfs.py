"""
    Nokia H6-128 BMC sysfs functions
"""

try:
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

sonic_logger = logger.Logger('sysfs')


def read_sysfs_file(sysfs_file):
    """
    Read a sysfs attribute file.

    Returns:
        str: The file contents on success, or 'ERR' on failure.
    """
    rv = 'ERR'

    try:
        with open(sysfs_file, 'r', encoding='utf-8') as fd:
            rv = fd.read()
    except FileNotFoundError:
        sonic_logger.log_error("Error: {} doesn't exist.".format(sysfs_file))
    except PermissionError:
        sonic_logger.log_error("Error: Permission denied when reading file {}.".format(sysfs_file))
    except IOError:
        sonic_logger.log_error("IOError: An error occurred while reading file {}.".format(sysfs_file))
    if rv != 'ERR':
        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
    return rv


def write_sysfs_file(sysfs_file, value):
    """
    Write a value to a sysfs attribute file.

    Returns:
        int or str: Number of bytes written on success, or 'ERR' on failure.
    """
    rv = 'ERR'

    try:
        with open(sysfs_file, 'w', encoding='utf-8') as fd:
            rv = fd.write(value)
    except FileNotFoundError:
        sonic_logger.log_error("Error: {} doesn't exist.".format(sysfs_file))
    except PermissionError:
        sonic_logger.log_error("Error: Permission denied when writing file {}.".format(sysfs_file))
    except IOError:
        sonic_logger.log_error("IOError: An error occurred while writing file {}.".format(sysfs_file))

    return rv
