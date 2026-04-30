from .log import log_err, log_info
from .manager import Manager
from .utils import run_command

BFD_PROFILE_TABLE_NAME = "BFD_PROFILE"

# Maps CONFIG_DB field names to FRR command names
FIELD_TO_FRR_CMD = {
    'detect_multiplier': 'detect-multiplier',
    'receive_interval': 'receive-interval',
    'transmit_interval': 'transmit-interval',
    'echo_interval': 'echo-interval',
    'minimum_ttl': 'minimum-ttl',
}

# Boolean fields that render as "command" / "no command"
BOOLEAN_FIELDS = {
    'echo_mode': 'echo-mode',
    'passive_mode': 'passive-mode',
}


class BfdProfileMgr(Manager):
    """This class manages BFD profiles in FRR based on CONFIG_DB BFD_PROFILE table."""

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(BfdProfileMgr, self).__init__(
            common_objs,
            [],  # no dependencies
            db,
            table,
        )
        self.profiles = {}  # name -> data dict, tracks current state

    def set_handler(self, key, data):
        """Implementation of 'SET' command."""
        if not key:
            log_err("BFD profile name is empty")
            return True  # don't re-queue an invalid key

        prev = self.profiles.get(key, {})
        cmds = self._build_profile_cmds(key, data, prev)
        command = ["vtysh", "-c", "conf t", "-c", "bfd"] + cmds
        ret_code, out, err = run_command(command)
        if ret_code != 0:
            log_err("Can't configure BFD profile '%s': %s" % (key, err))
            return False

        self.profiles[key] = dict(data)
        log_info("BFD profile '%s' configured" % key)
        return True

    def del_handler(self, key):
        """Implementation of 'DEL' command."""
        command = ["vtysh", "-c", "conf t", "-c", "bfd",
                   "-c", "no profile %s" % key]
        ret_code, out, err = run_command(command)
        if ret_code != 0:
            log_err("Can't remove BFD profile '%s': %s" % (key, err))
            return

        if key in self.profiles:
            del self.profiles[key]
        log_info("BFD profile '%s' removed" % key)

    def _build_profile_cmds(self, name, data, prev=None):
        """
        Build vtysh commands for a BFD profile configuration.
        :param name: profile name
        :param data: dict of profile fields from CONFIG_DB
        :param prev: dict of previously applied fields, for diff-based cleanup
        :return: list of vtysh command arguments
        """
        prev = prev or {}
        cmds = ["-c", "profile %s" % name]

        # Clear fields that existed previously but were removed in this update.
        # Without this, FRR retains stale values when a field is deleted from
        # CONFIG_DB while the profile itself still exists.
        for db_field, frr_cmd in FIELD_TO_FRR_CMD.items():
            if db_field in prev and db_field not in data:
                cmds.extend(["-c", "no %s" % frr_cmd])
        for db_field, frr_cmd in BOOLEAN_FIELDS.items():
            if db_field in prev and db_field not in data:
                cmds.extend(["-c", "no %s" % frr_cmd])

        # Numeric fields: render if present in data
        for db_field, frr_cmd in FIELD_TO_FRR_CMD.items():
            if db_field in data:
                cmds.extend(["-c", "%s %s" % (frr_cmd, data[db_field])])

        # Boolean fields: render as "cmd" or "no cmd"
        for db_field, frr_cmd in BOOLEAN_FIELDS.items():
            if db_field in data:
                if data[db_field].lower() == 'true':
                    cmds.extend(["-c", frr_cmd])
                else:
                    cmds.extend(["-c", "no %s" % frr_cmd])

        cmds.extend(["-c", "exit"])  # exit profile
        return cmds
