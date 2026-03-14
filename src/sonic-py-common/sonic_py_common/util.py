"""
Syslog and daemon script utility library.
"""

from __future__ import print_function
import json
import logging
import logging.config
import sys
from getopt import getopt


# TODO: move to dbsync project.
def usage(script_name):
    print('Usage: python ', script_name,
          '-t [host] -p [port] -s [unix_socket_path] -d [logging_level] -f [update_frequency] -h [help]')


# TODO: move to dbsync project.
def process_options(script_name):
    """
    Process command line options
    """
    options, remainders = getopt(sys.argv[1:], "t:p:s:d:f:h", ["host=", "port=", "unix_socket_path=", "debug=", "frequency=", "help"])

    args = {}
    for (opt, arg) in options:
        try:
            if opt in ('-d', '--debug'):
                args['log_level'] = int(arg)
            elif opt in ('-t', '--host'):
                args['host'] = arg
            elif opt in ('-p', '--port'):
                args['port'] = int(arg)
            elif opt in ('-s', 'unix_socket_path'):
                args['unix_socket_path'] = arg
            elif opt in ('-f', '--frequency'):
                args['update_frequency'] = int(arg)
            elif opt in ('-h', '--help'):
                usage(script_name)
        except ValueError as e:
            print('Invalid option for {}: {}'.format(opt, e))
            sys.exit(1)

    return args


# TODO: move
def setup_logging(config_file_path, log_level=logging.INFO):
    """
    Logging configuration helper.

    :param config_file_path: file path to logging configuration file.
    https://docs.python.org/3/library/logging.config.html#object-connections
    :param log_level: defaults to logging.INFO
    :return: None - access the logger by name as described in the config--or the "root" logger as a backup.
    """
    try:
        with open(config_file_path, 'rt') as f:
            config = json.load(f)
        logging.config.dictConfig(config)
    except (ValueError, IOError, OSError):
        # json.JSONDecodeError is throwable in Python3.5+ -- subclass of ValueError
        logging.basicConfig(log_level=log_level)
        logging.root.exception(
            "Could not load specified logging configuration '{}'. Verify the filepath exists and is compliant with: "
            "[https://docs.python.org/3/library/logging.config.html#object-connections]".format(config_file_path))
