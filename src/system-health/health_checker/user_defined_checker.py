from .health_checker import HealthChecker
from . import utils


class UserDefinedChecker(HealthChecker):
    """
    User could implement a script or program to perform customize check for particular system. In order to enable a
    user defined checker:
        1. Add an element to "user_defined_checkers" in the configuration file. The element must be an command string
        that can be executed by shell. For example: "python my_checker.py".
        2. The command output must match the following pattern:
        ${UserDefineCategory}
        ${Object1}:${ObjectStatusMessage1}
        ${Object2}:${ObjectStatusMessage2}

    An example of the command output:
    MyCategory
    Device1:OK
    Device2:OK
    Device3:Out of power
    """

    def __init__(self, cmd):
        """
        Constructor.
        :param cmd: Command string of the user defined checker.
        """
        HealthChecker.__init__(self)
        self._cmd = cmd
        self._category = None

    def reset(self):
        self._category = 'UserDefine'
        self._info = {}

    def get_category(self):
        return self._category

    def check(self, config):
        """
        Execute the user defined command and parse the output.
        :param config: Health checker configuration.
        :return:
        """
        self.reset()

        output = utils.run_command(self._cmd)
        if not output:
            self.set_object_not_ok('UserDefine', str(self), 'Failed to get output of command \"{}\"'.format(self._cmd))
            return

        output = output.strip()
        if not output:
            self.set_object_not_ok('UserDefine', str(self), 'Failed to get output of command \"{}\"'.format(self._cmd))
            return

        raw_lines = output.splitlines()
        if not raw_lines:
            self.set_object_not_ok('UserDefine', str(self), 'Invalid output of command \"{}\"'.format(self._cmd))
            return

        lines = []
        for line in raw_lines:
            line = line.strip()
            if not line:
                continue

            lines.append(line)

        if not lines:
            self.set_object_not_ok('UserDefine', str(self), 'Invalid output of command \"{}\"'.format(self._cmd))
            return

        self._category = lines[0]
        if len(lines) > 1:
            for line in lines[1:]:
                pos = line.find(':')
                if pos == -1:
                    continue
                obj_name = line[:pos].strip()
                msg = line[pos + 1:].strip()
                if msg != 'OK':
                    self.set_object_not_ok('UserDefine', obj_name, msg)
                else:
                    self.set_object_ok('UserDefine', obj_name)
        return

    def __str__(self):
        return 'UserDefinedChecker - {}'.format(self._cmd)
