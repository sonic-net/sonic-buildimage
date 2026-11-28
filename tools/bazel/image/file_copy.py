"""Copy immutable build outputs without sharing writable inodes."""

import errno
import fcntl
import os
import shutil


FICLONE = 0x40049409
UNSUPPORTED = {errno.EXDEV, errno.EOPNOTSUPP, errno.ENOTTY, errno.EINVAL,
               errno.ENOSYS, errno.EPERM, errno.EACCES}


def copy_file(source, destination):
    if os.path.exists(destination) and os.path.samefile(source, destination):
        raise shutil.SameFileError("source and destination are the same file")
    with open(source, "rb") as original, open(destination, "wb") as output:
        try:
            fcntl.ioctl(output.fileno(), FICLONE, original.fileno())
            return
        except OSError as error:
            if error.errno not in UNSUPPORTED:
                raise
    shutil.copyfile(source, destination)
