#!/usr/bin/env python3

import os
import shutil
from collections import defaultdict
from datetime import datetime

from sonic_py_common.logger import Logger

SYSLOG_IDENTIFIER = 'core_cleanup.py'
CORE_FILE_DIR = '/var/core/'
KERNEL_DUMP_DIR = '/var/dump/'
MAX_CORE_FILES = 4
EXPIRE_DAYS = 90

def delete_dump(file_path)
    try:
        os.remove(file_path)
    except:
        logger.log_error('Unexpected error occured trying to delete {}'.format(file_path))

def main():
    logger = Logger(SYSLOG_IDENTIFIER)
    logger.set_min_log_priority_info()

    if os.getuid() != 0:
        logger.log_error('Root required to clean up core files')
        return
        
    expire_date = date.today() + timedelta(days=EXPIRE_DAYS)

    logger.log_info('Cleaning up core files')
    core_files = [f for f in os.listdir(CORE_FILE_DIR) if os.path.isfile(os.path.join(CORE_FILE_DIR, f))]

    core_files_by_process = defaultdict(list)
    for f in core_files:
        # delete expired core files
        dump_date = datetime.utcfromtimestamp(int(f.split('.')[1]))
        if dump_date < expire_date:
            delete_dump(os.path.join(CORE_FILE_DIR, f))
            continue
            
        # for none expired core files, only keep recent MAX_CORE_FILES
        process = f.split('.')[0]
        curr_files = core_files_by_process[process]
        curr_files.append(f)

        if len(curr_files) > MAX_CORE_FILES:
            curr_files.sort(reverse = True, key = lambda x: datetime.utcfromtimestamp(int(x.split('.')[1])))
            oldest_core = curr_files[MAX_CORE_FILES]
            logger.log_info('Deleting {}'.format(oldest_core))
            delete_dump(os.path.join(CORE_FILE_DIR, oldest_core))
            core_files_by_process[process] = curr_files[0:MAX_CORE_FILES]

    logger.log_info('Finished cleaning up core files')
    
    # cleanup kernel dumps
    logger.log_info('Cleaning up kernel dump files')

    kernel_dumps = [f for f in os.listdir(KERNEL_DUMP_DIR) if os.path.isfile(os.path.join(KERNEL_DUMP_DIR, f))]
    not_expired_dumps = []
    for kernel_dump in kernel_dumps:
        # delete expired kernel dump
        dump_date = datetime.utcfromtimestamp(int(kernel_dump.split('_')[3]))
        if dump_date < expire_date:
            delete_dump(os.path.join(KERNEL_DUMP_DIR, kernel_dump))
            continue

        not_expired_dumps.append(f)
        if len(not_expired_dumps) > MAX_CORE_FILES:
            not_expired_dumps.sort(reverse = True, key = lambda x: datetime.utcfromtimestamp(int(kernel_dump.split('_')[3])))
            oldest_dump = not_expired_dumps[MAX_CORE_FILES]
            logger.log_info('Deleting {}'.format(oldest_dump))
            delete_dump(os.path.join(KERNEL_DUMP_DIR, oldest_dump))
            not_expired_dumps = not_expired_dumps[0:MAX_CORE_FILES]

    logger.log_info('Finished cleaning up kernel dump files')

if __name__ == '__main__':
    main()
