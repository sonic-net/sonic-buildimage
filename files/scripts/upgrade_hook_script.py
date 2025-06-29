#!/usr/bin/env python3

import argparse
import click
import subprocess
import os
from sonic_py_common import logger


SYSLOG_IDENTIFIER = "upgrade_hook_script"
LOG_ERR = logger.Logger.LOG_PRIORITY_ERROR
LOG_WARN = logger.Logger.LOG_PRIORITY_WARNING
LOG_NOTICE = logger.Logger.LOG_PRIORITY_NOTICE
# Global logger instance
log = logger.Logger(SYSLOG_IDENTIFIER)


class UpgradeHookException(Exception):
    """ Runtime Exception class used to report upgrade hook script related errors
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.notes = []

    def __str__(self):
        msg = super().__str__()
        if self.notes:
            msg += "\n" + "\n".join(self.notes)
        return msg

    def add_note(self, note):
        self.notes.append(note)


def echo_and_log(msg, priority=LOG_NOTICE, fg=None):
    if priority == LOG_ERR:
        # Print to stderr if priority is error
        click.secho(msg, fg=fg, err=True)
    else:
        click.secho(msg, fg=fg)
    log.log(priority, msg, False)


def run_command_or_raise(command_args, raise_exception=True):
    """ Run bash command and return output, raise if it fails
    """
    echo_and_log(f"Command: {command_args}")
    proc = subprocess.Popen(command_args, text=True, stdout=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode != 0:
        sre = UpgradeHookException("Failed to run command '{0}'".format(command_args))
        if out:
            sre.add_note("\nSTDOUT:\n{}".format(out.rstrip("\n")))
        if err:
            sre.add_note("\nSTDERR:\n{}".format(err.rstrip("\n")))
        echo_and_log(str(sre), LOG_WARN)
        if raise_exception:
            raise sre
    return out.rstrip("\n")


def update_localtime(new_image_dir):
    """ Update copy the symlink of /etc/localtime to the new image
        to synchronize localtime on the new image with the current time
    """
    LOCALTIME = "/etc/localtime"
    # Check if mounted fs is rw
    stat = os.statvfs(new_image_dir)
    if bool(stat.f_flag & os.ST_RDONLY):
        echo_and_log("Cannot set localtime, new image filesystem is read only", LOG_WARN)
        return
    full_path = os.path.join(os.path.dirname(LOCALTIME), os.readlink(LOCALTIME))
    if os.path.exists(new_image_dir + full_path):
        run_command_or_raise(["cp", "-af", LOCALTIME, os.path.join(new_image_dir, "etc", "localtime")], raise_exception=False)
    else:
        echo_and_log("The localtime file does not exist on new image", LOG_WARN)


def parse_args():
    parser = argparse.ArgumentParser('Script to perform upon image upgrade')
    parser.add_argument('--update-localtime', action='store_true',
                        help='Copy /etc/localtime into the mounted fs of the next image')
    parser.add_argument('--image-dir',
                        help='A path to a mounted rw fs of the next image')
    return parser.parse_args()


def main():
    echo_and_log("Executing upgrade hook script")
    args = parse_args()
    if args.update_localtime:
        # If image_dir is not given we do nothing
        image_dir = args.image_dir
        if image_dir:
            update_localtime(image_dir)
        else:
            echo_and_log("Image mount directory not given", LOG_WARN)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        echo_and_log(str(e), LOG_ERR)
        exit(1)
