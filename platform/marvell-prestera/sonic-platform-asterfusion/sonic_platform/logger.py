####################################################################
# Asterfusion CX-N Devices Logger API                              #
#                                                                  #
# Module contains an implementation of SONiC Platform Base API and #
# provides the logger api                                          #
#                                                                  #
####################################################################

try:
    import logging
    import os
    import yaml

    from inspect import getframeinfo, stack, Traceback
    from logging import config
    from pathlib import Path

    from .constants import *
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


@singleton
class Logger(object):
    """Platform-specific Logger class"""

    def __init__(self):
        logging_config_filepath = Path(Path(__file__).parent, LOGGING_CONFIG_NAME)
        if not logging_config_filepath.exists():
            logging.warning("Logging configuration file is missing")
            return
        logging_config = yaml.load(logging_config_filepath.read_text(), yaml.SafeLoader)
        try:
            config.dictConfig(logging_config)
        except Exception as err:
            if os.getuid() == 0:
                logging.warning(err)
                logging.warning(
                    "Logging configuration remains unchanged",
                )

    def _inject_caller_info(self, caller, msg):
        # type: (Traceback, str) -> str
        filename = Path(caller.filename).name
        lineno = caller.lineno
        funcname = caller.function
        return "{}:{} {} - {}".format(filename, lineno, funcname, msg)

    def log_fatal(self, msg, *args, **kwargs):
        # type: (str, object, object) -> None
        caller = getframeinfo(stack()[1][0])
        msg = self._inject_caller_info(caller, msg)
        stack_info = kwargs.get("stack_info", LOGGING_STACK_INFO)
        logging.fatal(msg, stack_info=stack_info, *args, **kwargs)

    def log_error(self, msg, *args, **kwargs):
        # type: (str, object, object) -> None
        caller = getframeinfo(stack()[1][0])
        msg = self._inject_caller_info(caller, msg)
        stack_info = kwargs.get("stack_info", LOGGING_STACK_INFO)
        logging.error(msg, stack_info=stack_info, *args, **kwargs)

    def log_warning(self, msg, *args, **kwargs):
        # type: (str, object, object) -> None
        caller = getframeinfo(stack()[1][0])
        msg = self._inject_caller_info(caller, msg)
        stack_info = kwargs.get("stack_info", LOGGING_STACK_INFO)
        logging.warning(msg, stack_info=stack_info, *args, **kwargs)

    def log_info(self, msg, *args, **kwargs):
        # type: (str, object, object) -> None
        caller = getframeinfo(stack()[1][0])
        msg = self._inject_caller_info(caller, msg)
        stack_info = kwargs.get("stack_info", LOGGING_STACK_INFO)
        logging.info(msg, stack_info=stack_info, *args, **kwargs)

    def log_debug(self, msg, *args, **kwargs):
        # type: (str, object, object) -> None
        caller = getframeinfo(stack()[1][0])
        msg = self._inject_caller_info(caller, msg)
        stack_info = kwargs.get("stack_info", LOGGING_STACK_INFO)
        logging.debug(msg, stack_info=stack_info, *args, **kwargs)
