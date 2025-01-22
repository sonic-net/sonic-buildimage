## ref: https://github.com/p/redis-dump-load/blob/7bbdb1eaea0a51ed4758d3ce6ca01d497a4e7429/redisdl.py

def sonic_db_dump_load():
    import optparse
    import os.path
    import re
    import sys
    from redisdl import dump, load
    from swsscommon.swsscommon import SonicDBConfig

    DUMP = 1
    LOAD = 2

    def options_to_kwargs(options):
        args = {}
        if options.password:
            args['password'] = options.password
        if options.encoding:
            args['encoding'] = options.encoding
        # dump only
        if hasattr(options, 'pretty') and options.pretty:
            args['pretty'] = True
        if hasattr(options, 'keys') and options.keys:
            args['keys'] = options.keys
        # load only
        if hasattr(options, 'use_expireat') and options.use_expireat:
            args['use_expireat'] = True
        if hasattr(options, 'empty') and options.empty:
            args['empty'] = True
        if hasattr(options, 'backend') and options.backend:
            args['streaming_backend'] = options.backend
        if hasattr(options, 'dbname') and options.dbname:
            if options.conntype == 'tcp':
                args['host'] = SonicDBConfig.getDbHostname(options.dbname)
                args['port'] = SonicDBConfig.getDbPort(options.dbname)
                args['db'] = SonicDBConfig.getDbId(options.dbname)
                args['unix_socket_path'] = None
            elif options.conntype == "unix_socket":
                args['host'] = None
                args['port'] = None
                args['db'] = SonicDBConfig.getDbId(options.dbname)
                args['unix_socket_path'] = SonicDBConfig.getDbSock(options.dbname)
            else:
                raise TypeError('redis connection type is tcp or unix_socket')

        return args

    def do_dump(options):
        if options.output:
            output = open(options.output, 'w')
        else:
            output = sys.stdout

        kwargs = options_to_kwargs(options)
        dump(output, **kwargs)

        if options.output:
            output.close()

    def do_load(options, args):
        if len(args) > 0:
            input = open(args[0], 'rb')
        else:
            input = sys.stdin

        kwargs = options_to_kwargs(options)
        load(input, **kwargs)

        if len(args) > 0:
            input.close()

    script_name = os.path.basename(sys.argv[0])
    if re.search(r'load(?:$|\.)', script_name):
        action = help = LOAD
    elif re.search(r'dump(?:$|\.)', script_name):
        action = help = DUMP
    else:
        # default is dump, however if dump is specifically requested
        # we don't show help text for toggling between dumping and loading
        action = DUMP
        help = None

    if help == LOAD:
        usage = "Usage: %prog [options] [FILE]"
        usage += "\n\nLoad data from FILE (which must be a JSON dump previously created"
        usage += "\nby redisdl) into specified or default redis."
        usage += "\n\nIf FILE is omitted standard input is read."
    elif help == DUMP:
        usage = "Usage: %prog [options]"
        usage += "\n\nDump data from specified or default redis."
        usage += "\n\nIf no output file is specified, dump to standard output."
    else:
        usage = "Usage: %prog [options]"
        usage += "\n       %prog -l [options] [FILE]"
        usage += "\n\nDump data from redis or load data into redis."
        usage += "\n\nIf input or output file is specified, dump to standard output and load"
        usage += "\nfrom standard input."
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-w', '--password', help='connect with PASSWORD')
    if help == DUMP:
        parser.add_option('-n', '--dbname', help='dump DATABASE (APPL_DB/ASIC_DB...)')
        parser.add_option('-t', '--conntype', help='indicate redis connection type (tcp[default] or unix_socket)', default='tcp')
        parser.add_option('-k', '--keys', help='dump only keys matching specified glob-style pattern')
        parser.add_option('-o', '--output', help='write to OUTPUT instead of stdout')
        parser.add_option('-y', '--pretty', help='split output on multiple lines and indent it', action='store_true')
        parser.add_option('-E', '--encoding', help='set encoding to use while decoding data from redis', default='utf-8')
    elif help == LOAD:
        parser.add_option('-n', '--dbname', help='dump DATABASE (APPL_DB/ASIC_DB...)')
        parser.add_option('-t', '--conntype', help='indicate redis connection type (tcp[default] or unix_socket)', default='tcp')
        parser.add_option('-e', '--empty', help='delete all keys in destination db prior to loading', action='store_true')
        parser.add_option('-E', '--encoding', help='set encoding to use while encoding data to redis', default='utf-8')
        parser.add_option('-B', '--backend', help='use specified streaming backend')
        parser.add_option('-A', '--use-expireat', help='use EXPIREAT rather than TTL/EXPIRE', action='store_true')
    else:
        parser.add_option('-l', '--load', help='load data into redis (default is to dump data from redis)', action='store_true')
        parser.add_option('-n', '--dbname', help='dump DATABASE (APPL_DB/ASIC_DB/COUNTERS_DB/CONFIG_DB...)')
        parser.add_option('-t', '--conntype', help='indicate redis connection type (tcp[default] or unix_socket)', default='tcp')
        parser.add_option('-k', '--keys', help='dump only keys matching specified glob-style pattern')
        parser.add_option('-o', '--output', help='write to OUTPUT instead of stdout (dump mode only)')
        parser.add_option('-y', '--pretty', help='split output on multiple lines and indent it (dump mode only)', action='store_true')
        parser.add_option('-e', '--empty', help='delete all keys in destination db prior to loading (load mode only)', action='store_true')
        parser.add_option('-E', '--encoding', help='set encoding to use while decoding data from redis', default='utf-8')
        parser.add_option('-A', '--use-expireat', help='use EXPIREAT rather than TTL/EXPIRE', action='store_true')
        parser.add_option('-B', '--backend', help='use specified streaming backend (load mode only)')
    options, args = parser.parse_args()

    if hasattr(options, 'load') and options.load:
        action = LOAD

    if action == DUMP:
        if len(args) > 0:
            parser.print_help()
            exit(4)
        do_dump(options)
    else:
        if len(args) > 1:
            parser.print_help()
            exit(4)
        do_load(options, args)
