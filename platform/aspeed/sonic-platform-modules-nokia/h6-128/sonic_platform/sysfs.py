"""
    Nokia H6-128 BMC sysfs functions
"""

def read_sysfs_file(sysfs_file):
    """
    On successful read, returns the value read from given
    reg_name and on failure returns ERR
    """
    rv = 'ERR'

    try:
        with open(sysfs_file, 'r', encoding='utf-8') as fd:
            rv = fd.read()
            fd.close()
    except FileNotFoundError:
        print(f"Error: {sysfs_file} doesn't exist.")
    except PermissionError:
        print(f"Error: Permission denied when reading file {sysfs_file}.")
    except IOError:
        print(f"IOError: An error occurred while reading file {sysfs_file}.")
    if rv != 'ERR':
        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
    return rv

def write_sysfs_file(sysfs_file, value):
    """
    On successful write, the value read will be written on
    reg_name and on failure returns ERR
    """
    rv = 'ERR'

    try:
        with open(sysfs_file, 'w', encoding='utf-8') as fd:
            rv = fd.write(value)
            fd.close()
    except FileNotFoundError:
        print(f"Error: {sysfs_file} doesn't exist.")
    except PermissionError:
        print(f"Error: Permission denied when writing file {sysfs_file}.")
    except IOError:
        print(f"IOError: An error occurred while writing file {sysfs_file}.")

    return rv
