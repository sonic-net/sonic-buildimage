#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

try:
    import click
    from platform_intf import platform_reg_read, platform_reg_write, platform_get_optoe_type
    from platform_intf import platform_set_optoe_type, platform_sfp_read, platform_sfp_write
except ImportError as error:
    raise ImportError('%s - required module not found' % str(error)) from error


CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
        return None


def print_reg(info, offset):
    try:
        size = len(info)
        j = offset % 16
        tmp = j
        offset -= j
        print_buf = "\n            "

        for i in range(16):
            print_buf = print_buf + "%2x " % i
        print(print_buf)

        print_buf = None
        for i in range(size + j):
            if i % 16 == 0:
                print_buf = ""
                print_buf = "0x%08x  " % offset
                offset = offset + 16
            if tmp:
                print_buf = print_buf + "   "
                tmp = tmp - 1
            else:
                print_buf = print_buf + "%02x " % info[i - j]
            if (i + 1) % 16 == 0 or i == size + j - 1:
                print(print_buf)
    except Exception as e:
        msg = str(e)
        print("i = %d, j = %d," % (i, j))
        print(msg)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''platform_test main'''


@main.command()
@click.argument('dev_type', required=True)
@click.argument('dev_id', required=True)
@click.argument('offset', required=True)
@click.argument('size', required=True)
def reg_rd(dev_type, dev_id, offset, size):
    '''read cpld/fpga reg'''
    ret, info = platform_reg_read(int(dev_type), int(dev_id), int(offset), int(size))
    print(ret)
    if ret is True:
        print_reg(info, int(offset))
    else:
        print(info)


@main.command()
@click.argument('dev_type', required=True)
@click.argument('dev_id', required=True)
@click.argument('offset', required=True)
@click.argument('value', required=True)
def reg_wr(dev_type, dev_id, offset, value):
    '''write cpld/fpga reg'''
    value_list = []
    value_list.append(int(value))
    ret, info = platform_reg_write(int(dev_type), int(dev_id), int(offset), value_list)
    print(ret)
    print(info)


@main.command()
@click.argument('port', required=True)
def get_optoe_type(port):
    '''get optoe type'''
    ret, info = platform_get_optoe_type(int(port))
    print(ret)
    print(info)


@main.command()
@click.argument('port', required=True)
@click.argument('optoe_type', required=True)
def set_optoe_type(port, optoe_type):
    '''set optoe type'''
    ret, info = platform_set_optoe_type(int(port), int(optoe_type))
    print(ret)
    print(info)


@main.command()
@click.argument('port_id', required=True)
@click.argument('offset', required=True)
@click.argument('size', required=True)
def sfp_rd(port_id, offset, size):
    '''read sfp'''
    ret, info = platform_sfp_read(int(port_id), int(offset), int(size))
    print(ret)
    if ret is True:
        print_reg(info, int(offset))
    else:
        print(info)


@main.command()
@click.argument('port_id', required=True)
@click.argument('offset', required=True)
@click.argument('value', required=True)
def sfp_wr(port_id, offset, value):
    '''write sfp'''
    value_list = []
    value_list.append(int(value))
    ret, info = platform_sfp_write(int(port_id), int(offset), value_list)
    print(ret)
    print(info)


if __name__ == '__main__':
    main()
