import json
import os
import sys
import signal
import atexit
import time

TEST_PASS = 0
TEST_FAIL_ERR = 2
NORUN_ERR = 3


# test daemon class define
class TCDaemon():
    def __init__(self, module_name, logger, stdin=os.devnull, stdout=os.devnull, stderr=os.devnull):
        self.module_name = module_name
        self.logger = logger
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr
        self.pidfile = "/tmp/" + module_name + ".pid"
        self.resultfile = "/tmp/" + module_name + ".result"
        self.start_time_file = "/tmp/" + module_name + "_start_time"
        self.log_result = "/var/log/" + module_name + ".result"
        self.auto_end = False  # auto test end flag, if true, test-daemon will stop when count reatch to test_count
        self.test_count = -1  # test-daemon execute count
        self.skip_err = True  # when test error happened, skip and continue execution

    def set_skip_err(self, flag):
        self.skip_err = flag

    def set_auto_end(self, count):
        """
        set test daemon auto stop when count==self.test_count
        """
        self.auto_end = True
        self.test_count = count

    def daemonize(self):
        if os.path.exists(self.pidfile):
            self.logger.log_warn('{} already running'.format(self.module_name))
            raise RuntimeError('Already running')

        try:
            if os.fork() > 0:
                raise SystemExit(0)
        except OSError:
            self.logger.log_err('{} daemon fork failed'.format(self.module_name))
            raise RuntimeError('fork failed!')

        os.chdir('/')
        os.setsid()
        os.umask(0o22)

        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)
        except OSError:
            self.logger.log_err('fork #2 failed: %d (%s)\n' % (self.module_name))
            raise RuntimeError('fork failed!')

        sys.stdout.flush()
        sys.stderr.flush()

        with open(self.stdin, 'rb', 0) as f:
            os.dup2(f.fileno(), sys.stdin.fileno())
        with open(self.stdout, 'ab', 0) as f:
            os.dup2(f.fileno(), sys.stdout.fileno())
        with open(self.stderr, 'ab', 0) as f:
            os.dup2(f.fileno(), sys.stderr.fileno())

        # Write the PID file
        with open(self.pidfile, 'w') as f:
            f.write(str(os.getpid()))

        # Arrange to have the PID file removed on exit/signal
        # atexit.register(lambda: os.remove(self.pidfile))

        signal.signal(signal.SIGTERM, self.__signal_handler)

    # Signal handler for termination (required)
    @staticmethod
    def __signal_handler(signo, frame):
        raise SystemExit(1)

    def daemon_start(self, *argv):
        try:
            self.daemonize()
        except RuntimeError:
            raise SystemExit(1)
        self.before_daemon_run()
        self.daemon_run(*argv)

    def daemon_stop_previous(self):
        """ previous operations before daemon_stop """
        pass

    def record_result_dict_to_file(self, result_dict, json_file):
        with open(json_file, 'w') as f:
            json.dump(result_dict, f, indent=4, separators=(',', ': '))

    def daemon_stop(self):
        ret = TEST_PASS
        try:
            self.daemon_stop_previous()
            start_time = time.time()
            end_time = time.time()
            diff_time = round((end_time - start_time) / 60, 2)
            if os.path.exists(self.pidfile):
                with open(self.pidfile) as f:
                    os.kill(int(f.read()), signal.SIGTERM)
                os.remove(self.pidfile)
                if os.path.exists(self.start_time_file):
                    with open(self.start_time_file, "r") as f:
                        start_time = float(f.read())
                        end_time = time.time()
                        diff_time = round((end_time - start_time) / 60, 2)
                        self.logger.log_info("{} stress test {} mins.".format(self.module_name, diff_time), True)
                    os.remove(self.start_time_file)

                if os.path.exists(self.resultfile):
                    with open(self.resultfile, 'r') as f:
                        result = f.read()
                        if result.find('fail') >= 0:
                            self.logger.log_err('{} stress test fail!'.format(self.module_name), True)
                            ret = TEST_FAIL_ERR
                        else:
                            self.logger.log_info('{} stress test pass!'.format(self.module_name), True)
                    os.remove(self.resultfile)
                else:
                    self.logger.log_info('{} stress test pass!'.format(self.module_name), True)
            else:
                self.logger.log_warn('{} not running'.format(self.module_name), True)
                return NORUN_ERR
        except OSError as e:
            if 'No such process' in str(e) and os.path.exists(self.pidfile):
                os.remove(self.pidfile)
            if os.path.exists(self.resultfile):
                with open(self.resultfile, 'r') as f:
                    result = f.read()
                    if result.find('fail') >= 0:
                        self.logger.log_err('{} stress test fail!'.format(self.module_name), True)
                        ret = TEST_FAIL_ERR
                    else:
                        self.logger.log_info('{} stress test pass!'.format(self.module_name), True)
                os.remove(self.resultfile)
        result_dict = {}
        result_dict['testNo'] = 1
        result_dict['testItem'] = self.module_name
        result_dict['startTime'] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(end_time))
        result_dict['endTime'] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(start_time))
        result_dict['duration'] = str(round(end_time - start_time, 2)) + 's'
        result_dict['result'] = "Pass" if ret == TEST_PASS else "Fail"
        result_dict['errorCode'] = ''
        result_dict['failReason'] = ''
        self.record_result_dict_to_file(result_dict, self.log_result)
        return ret

    # run test
    def daemon_run(self, *argv):
        """ detail test process """
        pass

    def before_daemon_run(self):
        start_time = time.time()
        with open(self.start_time_file, "w") as f:
            f.write(str(start_time))
