class ConfigMgr(object):
    """ The class represents frr configuration """
    def __init__(self, frr):
        self.frr = frr
        self.current_config = None
        self.current_config_raw = None
        self.changes = ""
        self.peer_groups_to_restart = []

    def reset(self):
        """ Reset stored config """
        self.current_config = None
        self.current_config_raw = None
        self.changes = ""
        self.peer_groups_to_restart = []

    def update(self):
        """ Read current config from FRR """
        self.current_config = None
        self.current_config_raw = None
        out = self.frr.get_config()
        text = []
        for line in out.split('\n'):
            if line.lstrip().startswith('!'):
                continue
            text.append(line)
        text += ["     "]  # Add empty line to have something to work on, if there is no text
        self.current_config_raw = text
        self.current_config = self.to_canonical(out)  # FIXME: use text as an input

    def push_list(self, cmdlist):
        """
        Prepare new changes for FRR. The changes should be committed by self.commit()
        :param cmdlist: configuration change for FRR. Type: List of Strings
        """
        self.changes += "\n".join(cmdlist) + "\n"

    def push(self, cmd):
        """
        Prepare new changes for FRR. The changes should be committed by self.commit()
        :param cmd: configuration change for FRR. Type: String
        """
        self.changes += cmd + "\n"
        return True

    def restart_peer_groups(self, peer_groups):
        """
        Schedule peer_groups for restart on commit
        :param peer_groups: List of peer_groups
        """
        self.peer_groups_to_restart.extend(peer_groups)

    def commit(self):
        """
        Write configuration change to FRR.
        :return: True if change was applied successfully, False otherwise
        """
        if self.changes.strip() == "":
            return True
        rc_write = self.frr.write(self.changes)
        rc_restart = self.frr.restart_peer_groups(self.peer_groups_to_restart)
        self.reset()
        return rc_write and rc_restart

    def get_text(self):
        return self.current_config_raw

    @staticmethod
    def to_canonical(raw_config):
        """
        Convert FRR config into canonical format
        :param raw_config: config in frr format
        :return: frr config in canonical format
        """
        parsed_config = []
        lines_with_comments = raw_config.split("\n")
        lines = [line for line in lines_with_comments
                 if not line.strip().startswith('!') and line.strip() != '']
        if len(lines) == 0:
            return []
        cur_path = [lines[0]]
        cur_offset = ConfigMgr.count_spaces(lines[0])
        for line in lines:
            n_spaces = ConfigMgr.count_spaces(line)
            s_line = line.strip()
            if n_spaces == cur_offset:
                cur_path[-1] = s_line
            elif n_spaces > cur_offset:
                cur_path.append(s_line)
            elif n_spaces < cur_offset:
                cur_path = cur_path[:-2]
                cur_path.append(s_line)
            parsed_config.append(cur_path[:])
            cur_offset = n_spaces
        return parsed_config

    @staticmethod
    def count_spaces(line):
        """ Count leading spaces in the line """
        return len(line) - len(line.lstrip())

    @staticmethod
    def from_canonical(canonical_config):
        """
        Convert config from canonical format into FRR raw format
        :param canonical_config: config in a canonical format
        :return: config in the FRR raw format
        """
        out = ""
        for lines in canonical_config:
            spaces = len(lines) - 1
            out += " " * spaces + lines[-1] + "\n"

        return out