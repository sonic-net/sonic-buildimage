#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import os
import sys
import click
import subprocess


def io_rd(reg_addr, read_len=1):
    u'''io read'''
    try:
        regaddr = 0
        if isinstance(reg_addr, int):
            regaddr = reg_addr
        else:
            regaddr = int(reg_addr, 16)
        devfile = "/dev/port"
        fd = os.open(devfile, os.O_RDWR | os.O_CREAT)
        os.lseek(fd, regaddr, os.SEEK_SET)
        str = os.read(fd, read_len)
        return "".join(["%02x" % item for item in str])
    except ValueError:
        return None
    except Exception as e:
        print(e)
        return None
    finally:
        os.close(fd)
    return None


@click.group()
def cli():
    pass


@cli.command()
@click.argument('addr', required=True)
@click.argument('length', required=True, type=int)
def reg_read(addr, length):
    try:
        value = io_rd(addr, length)
        if value is None:
            print("read failed")
        else:
            print(value)
    except Exception as e:
        print(e)
        return None


if __name__ == '__main__':
    cli()
