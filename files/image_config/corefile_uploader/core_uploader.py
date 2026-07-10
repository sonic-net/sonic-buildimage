#!/usr/bin/env python3

import json
import os
import socket
import tarfile
import time
import subprocess
import yaml
from azure.storage.file import FileService
from sonic_py_common.logger import Logger
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

SYSLOG_IDENTIFIER = os.path.basename(__file__)

cfg = ""

CORE_FILE_PATH = "/var/core/"
RC_FILE = "/etc/sonic/core_analyzer.rc.json"
INIT_CWD = "/tmp"

hostname = ""
sonicversion = ""
asicname = ""
acctname = ""
acctkey = ""
sharename = ""
cwd = []

HOURS_4 = (4 * 60 * 60)
PAUSE_ON_FAIL = (60 * 60)
WAIT_FILE_WRITE1 = (10 * 60)
WAIT_FILE_WRITE2 = (5 * 60)
POLL_SLEEP = (60 * 60)
MAX_RETRIES = 5
UPLOAD_PREFIX = "UPLOADED_"

# Global logger instance
logger = Logger(SYSLOG_IDENTIFIER)
logger.set_min_log_priority_info()


def make_new_dir(p):
    subprocess.call(["rm", "-rf", p])
    subprocess.call(["mkdir", "-p", p])


def parse_a_json(data, prefix, val):
    for i in data:
        if type(data[i]) == dict:
            parse_a_json(data[i], prefix + (i,), val)
        else:
            val[prefix + (i,)] = data[i]


class config:
    parsed_data = {}
    cfg_data = {}

    def __init__(self):
        while not os.path.exists(RC_FILE):
            # Wait here until service restart
            logger.log_error("Unable to retrieve Azure storage credentials")
            time.sleep(HOURS_4)

        with open(RC_FILE, 'r') as f:
            self.parsed_data = json.load(f)
            parse_a_json(self.parsed_data, (), self.cfg_data)

    def get_data(self, k):
        return self.cfg_data[k] if k in self.cfg_data else ""

    def get_dict(self):
        return self.parsed_data

    def get_core_info(self, corepath, devicename):
        info = {}
        info["corefname"] = os.path.basename(corepath)
        info["tstamp"] = str(os.stat(corepath).st_ctime)
        info["devicename"] = devicename

        lpath = self.get_data(("metadata_files_in_archive", "core_info"))
        f = open(lpath, "w+")
        f.write(json.dumps(info, indent=4))
        f.close()

        return lpath


class Watcher:

    def __init__(self):
        self.observer = Observer()

    def run(self):
        event_handler = Handler()
        self.observer.schedule(event_handler, CORE_FILE_PATH)
        self.observer.start()
        try:
            while True:
                time.sleep(POLL_SLEEP)
        except:
            self.observer.stop()
            logger.log_error("Error in watcher")

        self.observer.join()


def set_env(lst):
    for k in lst:
        if lst[k]:
            os.environ[k] = lst[k]
            logger.log_debug("set env {} = {}".format(k, lst[k]))


class Handler(FileSystemEventHandler):

    @staticmethod
    def init():
        global hostname, sonicversion, asicname, acctname, acctkey, sharename
        global cwd, cfg

        cfg = config()

        set_env(cfg.get_dict()["env"])

        hostname = socket.gethostname()
        if not hostname:
            raise Exception("Failed to read hostname")

        acctname = cfg.get_data(("azure_sonic_core_storage", "account_name"))
        acctkey = cfg.get_data(("azure_sonic_core_storage", "account_key"))
        sharename = cfg.get_data(("azure_sonic_core_storage", "share_name"))

        if not acctname or not acctkey or not sharename:
            while True:
                # Wait here until service restart
                logger.log_error("Unable to retrieve Azure storage credentials")
                time.sleep(HOURS_4)

        with open("/etc/sonic/sonic_version.yml", 'r') as stream:
            l = yaml.safe_load(stream)
            sonicversion = l['build_version']
            asicname = l['asic_type']

        if not sonicversion:
            raise Exception("Failed to read build_version from /etc/sonic/sonic_version.yml")

        if not asicname:
            raise Exception("Failed to read asic_type from /etc/sonic/sonic_version.yml")

        cwd = cfg.get_data(("local_work", "core_upload")).split("/")
        if not len(cwd) > 2:
            raise Exception("Invalid path for core_upload. Expect a min of two elements in path")

        os.chdir(INIT_CWD)

    @staticmethod
    def on_any_event(event):
        if event.is_directory:
            return None

        elif event.event_type == 'created':
            # Take any action here when a file is first created.
            logger.log_debug("Received create event - " + event.src_path)
            Handler.wait_for_file_write_complete(event.src_path)
            Handler.handle_file(event.src_path)

    @staticmethod
    def wait_for_file_write_complete(path):
        mtime = 0

        # Sleep for ample time enough for file dump to complete.
        time.sleep(WAIT_FILE_WRITE1)

        # Give another chance & poll until mtime stabilizes
        while mtime != os.stat(path).st_mtime:
            mtime = os.stat(path).st_mtime
            time.sleep(10)

        # A safety pause for double confirmation
        time.sleep(WAIT_FILE_WRITE2)
        if mtime != os.stat(path).st_mtime:
            raise Exception("Dump file creation is too slow: " + path)
            # Give up as something is terribly wrong with this file.

        logger.log_debug("File write complete - " + path)

    @staticmethod
    def handle_file(path):
        lpath = "/".join(cwd)
        make_new_dir(lpath)
        os.chdir(lpath)

        # Create a new archive with core & more.
        metafiles = cfg.get_dict()["metadata_files_in_archive"]

        fname = os.path.basename(path)
        tarf_name = fname + ".tar.gz"

        cfg.get_core_info(path, hostname)

        tar = tarfile.open(tarf_name, "w:gz")
        for e in metafiles:
            tar.add(metafiles[e])
        tar.add(path)
        tar.close()
        logger.log_debug("Tar file for upload created: " + tarf_name)

        Handler.upload_file(tarf_name, tarf_name, path)

        logger.log_debug("File uploaded - " + path)
        os.chdir(INIT_CWD)

    @staticmethod
    def upload_file(fname, fpath, coref):
        daemonname = fname.split(".")[0]
        i = 0

        while True:
            try:
                svc = FileService(account_name=acctname, account_key=acctkey)

                l = [sonicversion, asicname, daemonname, hostname]
                e = []
                while len(e) != len(l):
                    e.append(l[len(e)])
                    svc.create_directory(sharename, "/".join(e))

                logger.log_debug("Remote dir created: " + "/".join(e))

                svc.create_file_from_path(sharename, "/".join(l), fname, fpath)
                logger.log_debug("Remote file created: name{} path{}".format(fname, fpath))
                newcoref = os.path.dirname(coref) + "/" + UPLOAD_PREFIX + os.path.basename(coref)
                os.rename(coref, newcoref)
                break

            except Exception as ex:
                logger.log_error("core uploader failed: Failed during upload (" +
                                 coref + ") err: (" + str(ex) + ") retry:" + str(i))
                if not os.path.exists(fpath):
                    break
                i += 1
                time.sleep(PAUSE_ON_FAIL)

    @staticmethod
    def scan():
        for e in os.listdir(CORE_FILE_PATH):
            fl = CORE_FILE_PATH + e
            if os.path.isfile(fl) and not e.startswith(UPLOAD_PREFIX):
                Handler.handle_file(fl)


if __name__ == '__main__':
    try:
        Handler.init()
        w = Watcher()
        Handler.scan()
        w.run()
    except Exception as e:
        logger.log_err("core uploader failed: " + str(e) + " Exiting ...")
