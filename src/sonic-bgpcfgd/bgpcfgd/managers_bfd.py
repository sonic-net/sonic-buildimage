import subprocess

from .log import log_warn, log_err, log_info, log_warn, log_crit
from .manager import Manager
from .utils import run_command
import json


class BfdMgr(Manager):
    """ This class updates BFD sessions in FRR whenever software BFD state table
        is updated """

    MULTIPLIER = 3
    RX_INTERVAL_MS = 200
    TX_INTERVAL_MS = 200

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(BfdMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

        if(self.check_and_start_bfdd()):
            self.bfd_sessions = self.load_bfd_sessions()

    def check_and_start_bfdd(self):
        """
        Check if bfdd is already running. If it's not, start the process.
        :return: True if bfdd process is running or was successfully started
        """
        try:
            # Use pgrep to check if the process is running
            subprocess.check_output(["pgrep", "-f", "bfdd"])
            return True
        except subprocess.CalledProcessError:
            # Start bfdd process
            log_warn("bfdd process is not running, starting now...")
            command = ["/usr/lib/frr/bfdd", "-A", "127.0.0.1", "-d"]
            proc_out = subprocess.run(command)
            if proc_out.returncode != 0:
                log_err("Can't start bfdd: %s" % proc_out.returncode)
                return False

            return True

    def split_key(self, key):
         """
         Split key into vrf, interface, and peer ip address.
         :param key: key to split
         :return: vrf, interface, peer ip address extracted from the key
         """
         return tuple(key.split('|', 2))

    def dict_to_fs(self, d):
        return frozenset(d.items())

    def get_def_res_fields(self):
        return {
            # DSCP is 48 by default
            'multihop': False,
            'local': '',
            'detect-multiplier': self.MULTIPLIER,
            'receive-interval_ms': self.RX_INTERVAL_MS,
            'transmit-interval_ms': self.TX_INTERVAL_MS,
            'passive-mode': True,
        }

    def redis_to_local_res(self, data):
        """
        Convert fields in redis state DB format to fields in local DB
        format which also aligns with fields in frr bfd session.
        :param data: dict of fields from redis state DB
        :return: dict of fields in local DB format
        """
        # init local result data to default values
        res_data = self.get_def_res_fields()

        # Go through field from redis session table and overwrite default
        # value in local result data structure if needed.
        if "multihop" in data: 
            if data["multihop"] == "true":
                res_data["multihop"] = True
            else:
                res_data["multihop"] = False

        if "local_addr" in data:
            res_data["local"] = data["local_addr"]

        if "multiplier" in data:
            res_data["detect-multiplier"] = int(data["multiplier"])

        if "rx_interval" in data:
            res_data["receive-interval_ms"] = int(data["rx_interval"])

        if "tx_interval" in data:
            res_data["transmit-interval_ms"] = int(data["tx_interval"])

        if "type" in data:
            if data["type"] == "async_active":
                res_data["passive-mode"] = False
            else:
                res_data["passive-mode"] = True

        return res_data

    def create_bfd_peer_cmd(self, session_key, res_data, is_delete):
        """
        Creates an array of one more bfd commands to be sent to
        frr in vtysh. Each command is preceeded by a "-c" which is
        required by vtysh.
        :param session_key: dict of bfd session key from state DB
        :param data: dict of bfd session parameters in local DB format
        :param data: True if deleting a session, False otherwise
        :return: array of command strings
        """
        bfd_cmd = "peer %s " % session_key["peer"]
        if res_data["multihop"] == True and \
            session_key["interface"] == "default":
            bfd_cmd += "multihop "

        if res_data["local"] != "":
            bfd_cmd += "local-address %s " % res_data["local"]

        if session_key["interface"] != "default":
            bfd_cmd += "interface %s " % session_key["interface"]
        bfd_cmd += "vrf %s" % session_key["vrf"]

        if is_delete:
            bfd_cmd = "no " + bfd_cmd 
            return ["-c", bfd_cmd]

        # if adding a session, configure additional fields under peer
        bfd_cmds = ["-c", bfd_cmd]

        bfd_cmds.append("-c")
        bfd_cmds.append("detect-multiplier " + str(res_data["detect-multiplier"]))
        bfd_cmds.append("-c")
        bfd_cmds.append("receive-interval " + str(res_data["receive-interval_ms"]))
        bfd_cmds.append("-c")
        bfd_cmds.append("transmit-interval " + str(res_data["transmit-interval_ms"]))
        bfd_cmds.append("-c")

        if res_data["multihop"] == True:
            bfd_cmds.append("minimum-ttl 1")
            bfd_cmds.append("-c")

        if (res_data["passive-mode"] == True):
            bfd_cmds.append("passive-mode")
        else:
            bfd_cmds.append("no passive-mode")

        return bfd_cmds

    def add_frr_session(self, session_key, data):
        """
        Add a new bfd session to frr.
        :param session_key: dict of bfd session key from state DB
        :param data: dict of bfd session parameters from state DB
        :return: True if bfd session was added successfully
        """
        res_data = {'multihop': False, 'local': ''}
        res_data = self.redis_to_local_res(data)
        cmd_add = self.create_bfd_peer_cmd(session_key, res_data, False)
        command = ["vtysh", "-c", "conf t", "-c", "bfd"] + cmd_add

        ret_code, out, err = run_command(command)
        if ret_code != 0:
            log_err("Can't add bfd session: %s, err: %s" % (str(session_key), err))
            return False

        # Add the new session's key to local session DB
        self.bfd_sessions[self.dict_to_fs(session_key)] = res_data
        return True

    def update_frr_session(self, session_key, data):
        """
        Update existing bfd session in frr if some parameters don't match.
        The update is done by deleting the session and then recreating it
        if at least one of the fields in frr bfd session key is modified.
        :param session_key: dict of bfd session key from state DB
        :param data: dict of bfd session parameters in local DB format
        :return: True if no error occurred during updated
        """
        res_data = self.redis_to_local_res(data)
        old_res_data = self.bfd_sessions[self.dict_to_fs(session_key)]
        if old_res_data == res_data:
            log_warn("bfd session fields are same, skipping update in frr")
            return True

        if (res_data["multihop"] != old_res_data["multihop"]) or \
            (res_data["local"] != old_res_data["local"]):
            # These fields are part of frr bfd key so the session
            # needs to be recreated.
            
            cmd_del = self.create_bfd_peer_cmd(session_key, old_res_data, True)
            cmd_add = self.create_bfd_peer_cmd(session_key, res_data, False)
            command = ["vtysh", "-c", "conf t", "-c", "bfd"]
            command = command + cmd_del + cmd_add
        else:
            # update some fields in the existing bfd frr peer  
            cmds = self.create_bfd_peer_cmd(session_key, res_data, False)
            command = ["vtysh", "-c", "conf t", "-c", "bfd"] + cmds
         
        if command != "":
            ret_code, out, err = run_command(command)
            if ret_code != 0:
                log_err("Can't update bfd session: %s, err: %s" % (str(session_key), err))
                return False

        # update session in local session DB
        self.bfd_sessions[self.dict_to_fs(session_key)] = res_data

        return True

    def del_frr_session(self, session_key):
        """
        Delete an existing bfd session in frr
        :param session_key: dict of bfd session key from state DB
        :return: True if session was successfully deleted
        """
        res_data = self.bfd_sessions[self.dict_to_fs(session_key)]
        cmd_del = self.create_bfd_peer_cmd(session_key, res_data, True)
        command = ["vtysh", "-c", "conf t", "-c", "bfd"] + cmd_del
        ret_code, out, err = run_command(command)
        if ret_code != 0:
            log_err("Can't delete bfd session: %s" % err)
            return False

        # Delete the sessions from local session DB
        del self.bfd_sessions[self.dict_to_fs(session_key)]

        return True

    def load_bfd_sessions(self):
        """
        Load existing bfd sessions from FRR.
        :return: set of bfd sessions, which are already installed in FRR
        """
        sessions_db = dict()
        command = ["vtysh", "-c", "show bfd peers json"]
        ret_code, out, err = run_command(command)
        if ret_code != 0:
            log_err("Can't read bfd peers: %s" % err)
            return sessions_db
        sessions = json.loads(out)
        
        # Specify all the BFD session fields to keep track of and their default
        # value.
        # "peer" is the only field which is mandatory and must be in the json
        # output.
        all_key_fields = {'vrf': 'default', 'interface': 'default', 'peer': ''}
        all_res_fields = self.get_def_res_fields()
        for session in sessions:
            sessions_db_key_entry = dict()
            sessions_db_res_entry = dict()
            if "peer" not in session:
                log_err("Peer is not set in frr bfd session, skipping")
                continue;

            for field in all_key_fields:
                if field not in session:
                    value = all_key_fields[field]
                else:
                    value = session[field]
                sessions_db_key_entry[field] = value

            for field in all_res_fields:
                if field not in session:
                    value = all_res_fields[field]
                else:
                    value = session[field]
                sessions_db_res_entry[field] = value

            # Add session to the sessions DB set
            sessions_db[self.dict_to_fs(sessions_db_key_entry)] = sessions_db_res_entry
        return sessions_db

    def set_handler(self, key, data):
        """ Implementation of 'SET' command. """
        vrf, interface, peer = self.split_key(key)
        session_to_add = {'vrf': vrf, 'interface': interface, 'peer': peer}

        if self.dict_to_fs(session_to_add) in self.bfd_sessions:
            log_warn("found existing session, update if needed")
            return self.update_frr_session(session_to_add, data)
        else:
            log_warn("add a new bfd session to frr")
            return self.add_frr_session(session_to_add, data)

        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command """
        vrf, interface, peer = self.split_key(key)
        session_to_del = {'vrf': vrf, 'interface': interface, 'peer': peer}
        if self.dict_to_fs(session_to_del) in self.bfd_sessions:
            log_warn("found existing bfd session to delete, key: %s" % key)
            return self.del_frr_session(session_to_del)
        else:
            log_err("no existing bfd session found, key: %s" % key)

        return True

