#!/usr/bin/env python3

""""
Description: bgpmon.py -- populating bgp related information in stateDB.
    script is started by supervisord in bgp docker when the docker is started.

    Initial creation of this daemon is to assist SNMP agent in obtaining the
    BGP related information for its MIB support. The MIB that this daemon is
    assisting is for the CiscoBgp4MIB (Neighbor state only). Also for chassis use-case
    it identify if the given BGP neighbors as i-BGP vs e-BGP. If there are other
    BGP related items that needs to be updated in a periodic manner in the
    future, then more can be added into this process.

    The script check if there are any bgp activities by monitoring the bgp
    frr.log file timestamp.  If activity is detected, then it will request bgp
    neighbor state via vtysh cli interface. This bgp activity monitoring is
    done periodically (every 15 second). When triggered, it looks specifically
    for the neighbor state in the json output of show ip bgp neighbors json
    and update the state DB for each neighbor accordingly.
    In order to not disturb and hold on to the State DB access too long and
    removal of the stale neighbors (neighbors that was there previously on
    previous get request but no longer there in the current get request), a
    "previous" neighbor dictionary will be kept and used to determine if there
    is a need to perform update or the peer is stale to be removed from the
    state DB
"""
import json
import os
import sys
import syslog
from swsscommon import swsscommon
import time
from sonic_py_common.general import getstatusoutput_noshell

PIPE_BATCH_MAX_COUNT = 50

class BgpStateGet:
    def __init__(self):
        # set peer_l stores the Neighbor peer Ip address
        # dic peer_state stores the Neighbor peer state entries
        # set new_peer_l stores the new snapshot of Neighbor peer ip address
        # dic new_peer_state stores the new snapshot of Neighbor peer states
        self.peer_l = set()
        self.peer_state = {}
        self.new_peer_l = set()
        self.new_peer_state = {}
        self.cached_timestamp = 0
        self.db = swsscommon.SonicV2Connector()
        self.db.connect(self.db.STATE_DB, False)
        self.pipe = swsscommon.RedisPipeline(self.db.get_redis_client(self.db.STATE_DB))
        self.db.delete_all_by_pattern(self.db.STATE_DB, "NEIGH_STATE_TABLE|*" )
        self.MAX_RETRY_ATTEMPTS = 3

    # A quick way to check if there are anything happening within BGP is to
    # check its log file has any activities. This is by checking its modified
    # timestamp against the cached timestamp that we keep and if there is a
    # difference, there is activity detected. In case the log file got wiped
    # out, it will default back to constant pulling every 15 seconds
    def bgp_activity_detected(self):
        try:
            timestamp = os.stat("/var/log/frr/frr.log").st_mtime
            if timestamp != self.cached_timestamp:
                self.cached_timestamp = timestamp
                return True
            else:
                return False
        except (IOError, OSError):
            return True

    def update_new_peer_states(self, peer_dict):
        peer_l = peer_dict["peers"].keys()
        self.new_peer_l.update(peer_l)
        for peer in peer_l:
            self.new_peer_state[peer] = (peer_dict["peers"][peer]["state"], 
                                         peer_dict["peers"][peer]["remoteAs"],
                                         peer_dict["peers"][peer]["localAs"])

    # Get a new snapshot of BGP neighbors and store them in the "new" location
    def get_all_neigh_states(self):
        cmd = ["vtysh", "-H", "/dev/null",  "-c", 'show bgp summary json']
        retry_attempt = 0

        while retry_attempt < self.MAX_RETRY_ATTEMPTS:
            try:
                rc, output = getstatusoutput_noshell(cmd)
                if rc:
                    syslog.syslog(syslog.LOG_ERR, "*ERROR* Failed with rc:{} when execute: {}".format(rc, cmd))
                    return
                if len(output) == 0:
                    syslog.syslog(syslog.LOG_WARNING, "*WARNING* output none when execute: {}".format(cmd))
                    return

                peer_info = json.loads(output)
                # cmd ran successfully, safe to Clean the "new" set/dict for new snapshot
                self.new_peer_l.clear()
                self.new_peer_state.clear()
                for key, value in peer_info.items():
                    if key == "ipv4Unicast" or key == "ipv6Unicast":
                        self.update_new_peer_states(value)
                return

            except json.JSONDecodeError as decode_error:
                # Log the exception and retry if within the maximum attempts
                retry_attempt += 1
                syslog.syslog(syslog.LOG_WARNING, "*WARNING* JSONDecodeError: {} when execute: {} Retry attempt: {}".format(decode_error, cmd, retry_attempt))
                time.sleep(1)
                continue
            except Exception as e:
                # Log other exceptions and return failure
                retry_attempt += 1
                syslog.syslog(syslog.LOG_WARNING, "*WARNING* An unexpected error occurred: {} when execute: {} Retry attempt: {}".format(e, cmd, retry_attempt))
                time.sleep(1)
                continue

        # Log an error if the maximum retry attempts are reached
        syslog.syslog(syslog.LOG_ERR, "*ERROR* Maximum retry attempts reached. Failed to execute: {} Output: {}".format(cmd, output))
        sys.exit(1)


    # This method will take the caller's dictionary which contains the peer state operation
    # That need to be updated in StateDB using Redis pipeline.
    # The data{} will be cleared at the end of this method before returning to caller.
    def flush_pipe(self, data):
        """Dump each entry in data{} into State DB via redis pipeline.
        Args:
            data: Neighbor state in dictionary format
            {
                'NEIGH_STATE_TABLE|ip_address_a': {'state':state},
                'NEIGH_STATE_TABLE|ip_address_b': {'state':state},
                'NEIGH_STATE_TABLE|ip_address_c': {'state':state},
                'NEIGH_STATE_TABLE|ip_address_x': None,
                'NEIGH_STATE_TABLE|ip_address_z': None
                ...
            }
        """
        for key, value in data.items():
            if value is None:
                # delete case
                command = swsscommon.RedisCommand()
                command.formatDEL(key)
                self.pipe.push(command)
            else:
                # Add or Modify case
                command = swsscommon.RedisCommand()
                command.formatHSET(key, value)
                self.pipe.push(command)

        self.pipe.flush()
        data.clear()

    def update_neigh_states(self):
        data = {}
        for peer in self.new_peer_l:
            key = "NEIGH_STATE_TABLE|%s" % peer
            if peer in self.peer_l:
                # only update the entry if state changed
                if self.peer_state[peer] != self.new_peer_state[peer][0]:
                    # state changed. Update state DB for this entry
                    state = self.new_peer_state[peer][0]
                    peerType = "i-BGP" if self.new_peer_state[peer][1] == self.new_peer_state[peer][2] else "e-BGP"
                    data[key] = {'state':state, 'peerType':peerType}
                    self.peer_state[peer] = state
                # remove this neighbor from old set since it is accounted for
                self.peer_l.remove(peer)
            else:
                # New neighbor found case. Add to dictionary and state DB
                state = self.new_peer_state[peer][0]
                peerType = "i-BGP" if self.new_peer_state[peer][1] == self.new_peer_state[peer][2] else "e-BGP"
                data[key] = {'state':state, 'peerType':peerType}
                self.peer_state[peer] = state
            if len(data) > PIPE_BATCH_MAX_COUNT:
                self.flush_pipe(data)
        # Check for stale state entries to be cleaned up
        for peer in self.peer_l:
            # remove this from the stateDB and the current neighbor state entry
            del_key = "NEIGH_STATE_TABLE|%s" % peer
            data[del_key] = None
            if peer in self.peer_state:
                del self.peer_state[peer]
            if len(data) > PIPE_BATCH_MAX_COUNT:
                self.flush_pipe(data)
        # If anything in the pipeline not yet flushed, flush them now
        if len(data) > 0:
            self.flush_pipe(data)
        # Save the new set
        self.peer_l = self.new_peer_l.copy()

def main():

    syslog.syslog(syslog.LOG_INFO, "bgpmon service started")
    bgp_state_get = None
    try:
        bgp_state_get = BgpStateGet()
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, "{}: error exit 1, reason {}".format("THIS_MODULE", str(e)))
        sys.exit(1)

    # periodically obtain the new neighbor information and update if necessary
    while True:
        time.sleep(15)
        if bgp_state_get.bgp_activity_detected():
            bgp_state_get.get_all_neigh_states()
            bgp_state_get.update_neigh_states()

if __name__ == '__main__':
    main()
