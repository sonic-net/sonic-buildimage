import json
import subprocess
import time
import syslog
from datetime import datetime, timezone
from swsscommon import swsscommon
from sonic_py_common import device_info
from sonic_py_common.general import getstatusoutput_noshell

class BfdFrrMon:
    def __init__(self):
        # Initialize local sets to store current BFD peer states
        self.local_v4_peers = set()
        self.local_v6_peers = set()
        self.status_table = "DPU_BFD_PROBE_STATE"
        self.db_connector = swsscommon.DBConnector("STATE_DB", 0)
        self.table = swsscommon.Table(self.db_connector, self.status_table)

        self.bfdd_running = False
        self.init_done = False
        self.MAX_RETRY_ATTEMPTS = 3

        self.remote_status_table = "DASH_BFD_PROBE_STATE"
        switch_type = device_info.get_localhost_info("switch_type")
        if switch_type and switch_type == "dpu":
            self.remote_db_connector = swsscommon.DBConnector("DPU_STATE_DB", 0, True)
            self.remote_table = swsscommon.Table(self.remote_db_connector, self.remote_status_table)
        else:
            self.remote_db_connector = None
            self.remote_table = None

    def check_bfdd(self):
        """
        Check if bfdd is running.
        Return: True if bfdd process is running, False otherwise.
        """
        try:
            # Use pgrep to check if the process is running
            rc, output = getstatusoutput_noshell(["pgrep", "-f", "bfdd"])
            if not rc:
                self.bfdd_running = True
                return True
        except Exception as e:    
            return False
        
        return False

    def get_bfd_sessions(self):
        """
        Get BFD session information from FRR using vtysh.
        Updates two sets: one for IPv4 peers and another for IPv6 peers whose BFD state is 'up'.
        Returns True if peer info was retreived successfully, False otherwise.
        """
    
        self.frr_v4_peers = set()
        self.frr_v6_peers = set()

        # Update bfdd state if it wasn't previously running
        if not self.bfdd_running:
            self.bfdd_running = self.check_bfdd()
            
        if not self.bfdd_running:
            syslog.syslog(syslog.LOG_WARNING, "*WARNING* bfdd not currently running")
            return False
    
        retry_attempt = 0
        cmd = ['vtysh', '-c', 'show bfd peers json']
        while retry_attempt < self.MAX_RETRY_ATTEMPTS:
            try:
                rc, output = getstatusoutput_noshell(cmd)
                if rc:
                    syslog.syslog(syslog.LOG_ERR, "*ERROR* Failed with rc:{} when execute: {}".format(rc, cmd))
                    return False
                if len(output) == 0:
                    syslog.syslog(syslog.LOG_WARNING, "*WARNING* output none when execute: {}".format(cmd))
                    return False

                bfd_data = json.loads(output)
                if bfd_data:
                    for session in bfd_data:
                        if "status" in session and session["status"] == "up":
                            if "peer" in session:
                                if ":" in session["peer"]:  # IPv6
                                    self.frr_v6_peers.add(session["peer"])
                                else:  # IPv4
                                    self.frr_v4_peers.add(session["peer"])
                return True
            except json.JSONDecodeError as e:
                # Log the exception and retry if within the maximum attempts
                retry_attempt += 1
                syslog.syslog(syslog.LOG_WARNING,
                    "*WARNING* JSONDecodeError: {} when execute: {} Retry attempt: {}".format(e, cmd, retry_attempt))
                time.sleep(1)
                continue
            except Exception as e:
                # Log other exceptions and return failure
                retry_attempt += 1
                syslog.syslog(syslog.LOG_WARNING,
                    "*WARNING* An unexpected error occurred: {} when execute: {} Retry attempt: {}".format(
                    e, cmd, retry_attempt))
                time.sleep(1)
                continue

        # Log an error if the maximum retry attempts are reached
        syslog.syslog(syslog.LOG_ERR,
            "*ERROR* Maximum retry attempts reached. Failed to execute: {}".format(cmd))
        return False
    
    def update_state_db(self):
        """
        Update the state DB only with changes (additions or deletions) to the peer list.
        """
        # Check differences between local sets and new data
        new_v4_peers = self.frr_v4_peers - self.local_v4_peers  # Peers to add
        removed_v4_peers = self.local_v4_peers - self.frr_v4_peers  # Peers to remove
    
        new_v6_peers = self.frr_v6_peers - self.local_v6_peers  # Peers to add
        removed_v6_peers = self.local_v6_peers - self.frr_v6_peers  # Peers to remove
    
        if new_v4_peers or removed_v4_peers or new_v6_peers or removed_v6_peers or not self.init_done:
            # Update local sets with the new data
            self.local_v4_peers = self.frr_v4_peers
            self.local_v6_peers = self.frr_v6_peers

            # Update Redis with the new peer sets
            values = [
                ("v4_bfd_up_sessions", json.dumps(list(self.local_v4_peers)).strip("[]")),
                ("v6_bfd_up_sessions", json.dumps(list(self.local_v6_peers)).strip("[]"))
            ]

            timestamp = datetime.now(timezone.utc).strftime("%a %b %d %I:%M:%S %p UTC %Y")
            if new_v4_peers or removed_v4_peers:
                values.append(("v4_bfd_up_sessions_timestamp", json.dumps(timestamp)))
            if new_v6_peers or removed_v6_peers:
                values.append(("v6_bfd_up_sessions_timestamp", json.dumps(timestamp)))

            self.table.set("", values)
            syslog.syslog(syslog.LOG_INFO,
                "{} table in STATE_DB updated. v4_peers: {}, v6_peers: {}".format(
                self.status_table, self.local_v4_peers, self.local_v6_peers))

            if self.remote_table:
                self.remote_table.set("dash_ha", values)
                syslog.syslog(syslog.LOG_INFO,
                    "{} table in DPU_STATE_DB updated. v4_peers: {}, v6_peers: {}".format(
                    self.remote_status_table, self.local_v4_peers, self.local_v6_peers))

            self.init_done = True
    
def main():
    SLEEP_TIME = 2 # Wait in seconds between each iteration
    syslog.syslog(syslog.LOG_INFO, "bfdmon service started")
    bfd_mon = BfdFrrMon()

    while True:
        # Sleep for a while before checking again (adjust as necessary)
        time.sleep(SLEEP_TIME)

        if bfd_mon.get_bfd_sessions():
            bfd_mon.update_state_db()

    syslog.syslog(syslog.LOG_INFO, "bfdmon service stopped")

if __name__ == "__main__":
    main()
