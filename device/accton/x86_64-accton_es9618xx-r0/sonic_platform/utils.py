from sonic_py_common import device_info
from sonic_py_common.logger import Logger

logger = Logger()


def fread(file_path, target_type, default='', raise_exception=False, log_func=logger.log_error):
    """
    Read content from file and convert to target type
    """
    try:
        with open(file_path, 'r') as f:
            value = f.read()
            if value is None:
                raise ValueError('File content of {} is None'.format(file_path))
            else:
                value = target_type(value.strip())
    except (ValueError, IOError) as e:
        if log_func:
            log_func('Failed to read from file {} - {}'.format(file_path, repr(e)))
        if not raise_exception:
            value = default
        else:
            raise e

    return value


def fread_str(file_path, default='', raise_exception=False, log_func=logger.log_error):
    """
    Read string content from file
    """
    return fread(file_path=file_path, target_type=str, default=default, raise_exception=raise_exception, log_func=log_func)


def fread_int(file_path, default=0, raise_exception=False, log_func=logger.log_error):
    """
    Read content from file and cast it to integer
    """
    return fread(file_path=file_path, target_type=int, default=default, raise_exception=raise_exception, log_func=log_func)
