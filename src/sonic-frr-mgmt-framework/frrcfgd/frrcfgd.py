#!/usr/bin/env python

import copy
import subprocess
import time
import syslog
import os
from swsscommon.swsscommon import ConfigDBConnector
import socket
import threading
import queue
import signal
import re
import logging
import netaddr
import io
import struct

class CachedDataWithOp:
    OP_NONE = 0
    OP_ADD = 1
    OP_DELETE = 2
    OP_UPDATE = 3

    STAT_SUCC = 0
    STAT_FAIL = 1

    def __init__(self, data = None, op = OP_NONE):
        self.data = data
        self.op = op
        self.status = self.STAT_FAIL

    def __repr__(self):
        op_str = ''
        if self.op == self.OP_NONE:
            op_str = 'NONE'
        elif self.op == self.OP_ADD:
            op_str = 'ADD'
        elif self.op == self.OP_DELETE:
            op_str = 'DELETE'
        elif self.op == self.OP_UPDATE:
            op_str = 'UPDATE'
        return '(%s, %s)' % (self.data, op_str)

bgpd_client = None

def g_run_command(table, command, use_bgpd_client, daemons, ignore_fail = False):
    syslog.syslog(syslog.LOG_DEBUG, "execute command {} for table {}.".format(command, table))
    if not command.startswith('vtysh '):
        use_bgpd_client = False
    if use_bgpd_client:
        if not bgpd_client.run_vtysh_command(table, command, daemons) and not ignore_fail:
            syslog.syslog(syslog.LOG_ERR, 'command execution failure. Command: "{}"'.format(command))
            return False
    else:
        p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
        stdout = p.communicate()[0]
        p.wait()
        if p.returncode != 0 and not ignore_fail:
            syslog.syslog(syslog.LOG_ERR, '[bgp cfgd] command execution returned {}. Command: "{}", stdout: "{}"'.\
                            format(p.returncode, command, stdout))
            return False
    return True

def extract_cmd_daemons(cmd_str):
    # daemon list could be given within brackets at head of input lines
    dm_mark = re.match(r'\[(?P<daemons>.+)\]', cmd_str)
    if dm_mark is not None and 'daemons' in dm_mark.groupdict():
        cmd_str = cmd_str[dm_mark.end():]
        daemons = dm_mark.groupdict()['daemons'].split(',')
    else:
        daemons = None
    return (daemons, cmd_str)

class BgpdClientMgr(threading.Thread):
    VTYSH_MARK = 'vtysh '
    PROXY_SERVER_ADDR = '/etc/frr/bgpd_client_sock'
    ALL_DAEMONS = ['bgpd', 'zebra', 'staticd', 'bfdd', 'ospfd', 'pimd', 'mgmtd']
    TABLE_DAEMON = {
            'DEVICE_METADATA': ['bgpd'],
            'BGP_GLOBALS': ['bgpd'],
            'BGP_GLOBALS_AF': ['bgpd'],
            'PREFIX_SET': ['bgpd'],
            'COMMUNITY_SET': ['bgpd'],
            'EXTENDED_COMMUNITY_SET': ['bgpd'],
            'ROUTE_MAP': ['zebra', 'bgpd', 'ospfd'],
            'PREFIX': ['zebra', 'bgpd', 'ospfd', 'pimd'],
            'BGP_PEER_GROUP': ['bgpd'],
            'BGP_NEIGHBOR': ['bgpd'],
            'BGP_PEER_GROUP_AF': ['bgpd'],
            'BGP_NEIGHBOR_AF': ['bgpd'],
            'BGP_GLOBALS_LISTEN_PREFIX': ['bgpd'],
            'NEIGHBOR_SET': ['bgpd'],
            'NEXTHOP_SET': ['bgpd'],
            'TAG_SET': ['bgpd'],
            'AS_PATH_SET': ['bgpd'],
            'ROUTE_REDISTRIBUTE': ['bgpd'],
            'BGP_GLOBALS_AF_AGGREGATE_ADDR': ['bgpd'],
            'BGP_GLOBALS_AF_NETWORK': ['bgpd'],
            'VRF': ['zebra'],
            'BGP_GLOBALS_EVPN_VNI': ['bgpd'],
            'BGP_GLOBALS_EVPN_RT': ['bgpd'],
            'BGP_GLOBALS_EVPN_VNI_RT': ['bgpd'],
            'BFD_PEER': ['bfdd'],
            'BFD_PEER_SINGLE_HOP': ['bfdd'],
            'BFD_PEER_MULTI_HOP': ['bfdd'],
            'IP_SLA': ['iptrackd'],
            'OSPFV2_ROUTER': ['ospfd'],
            'OSPFV2_ROUTER_AREA': ['ospfd'],
            'OSPFV2_ROUTER_AREA_VIRTUAL_LINK': ['ospfd'],
            'OSPFV2_ROUTER_AREA_NETWORK': ['ospfd'],
            'OSPFV2_ROUTER_AREA_POLICY_ADDRESS_RANGE': ['ospfd'],
            'OSPFV2_ROUTER_DISTRIBUTE_ROUTE': ['ospfd'],
            'OSPFV2_INTERFACE': ['ospfd'],
            'OSPFV2_ROUTER_PASSIVE_INTERFACE': ['ospfd'],
            'STATIC_ROUTE': ['staticd'],
            'PIM_GLOBALS': ['pimd'],
            'PIM_INTERFACE': ['pimd'],
            'IGMP_INTERFACE': ['pimd'],
            'IGMP_INTERFACE_QUERY': ['pimd'],
            'SRV6_MY_LOCATORS': ['zebra'],
            'SRV6_MY_SOURCE': ['zebra'],
            'SRV6_MY_SIDS': ['mgmtd']

    }
    VTYSH_CMD_DAEMON = [(r'show (ip|ipv6) route($|\s+\S+)', ['zebra']),
                        (r'show ip mroute($|\s+\S+)', ['pimd']),
                        (r'show bfd($|\s+\S+)', ['bfdd']),
                        (r'clear bfd($|\s+\S+)', ['bfdd']),
                        (r'clear ip mroute($|\s+\S+)', ['pimd']),
                        (r'clear ip pim($|\s+\S+)', ['pimd']),
                        (r'show ip ospf($|\s+\S+)', ['ospfd']),
                        (r'show ip pim($|\s+\S+)', ['pimd']),
                        (r'show ip igmp($|\s+\S+)', ['pimd']),
                        (r'clear ip ospf($|\s+\S+)', ['ospfd']),
                        (r'show ip sla($|\s+\S+)', ['iptrackd']),
                        (r'clear ip sla($|\s+\S+)', ['iptrackd']),
                        (r'clear ip igmp($|\s+\S+)', ['pimd']),
                        (r'.*', ['bgpd'])]
    @staticmethod
    def __create_proxy_socket():
        try:
            os.unlink(BgpdClientMgr.PROXY_SERVER_ADDR)
        except OSError:
            if os.path.exists(BgpdClientMgr.PROXY_SERVER_ADDR):
                raise
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.bind(BgpdClientMgr.PROXY_SERVER_ADDR)
        sock.listen(1)
        return sock
    @staticmethod
    def __get_reply(sock):
        reply_msg = None
        ret_code = None
        msg_buf = io.StringIO()
        while True:
            try:
                rd_msg = sock.recv(16384)
                msg_buf.write(rd_msg.decode())
            except socket.timeout:
                syslog.syslog(syslog.LOG_ERR, 'socket reading timeout')
                break
            if len(rd_msg) < 4:
                rd_msg = msg_buf.getvalue()
                if len(rd_msg) < 4:
                    continue
            msg_tail = rd_msg[-4:]
            if isinstance(msg_tail, str):
                msg_tail = bytes(msg_tail, 'utf-8')
            if msg_tail[0] == 0 and msg_tail[1] == 0 and msg_tail[2] == 0:
                ret_code = msg_tail[3]
                reply_msg = msg_buf.getvalue()[:-4]
                break
        msg_buf.close()
        return (ret_code, reply_msg)
    @staticmethod
    def __send_data(sock, data):
        if isinstance(data, str):
            data = bytes(data, 'utf-8')
        sock.sendall(data)
    def __create_frr_client(self):
        self.client_socks = {}
        for daemon in self.ALL_DAEMONS:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            serv_addr = '/run/frr/%s.vty' % daemon
            retry_cnt = 0
            while True:
                try:
                    sock.connect(serv_addr)
                    break
                except socket.error as msg:
                    syslog.syslog(syslog.LOG_ERR, 'failed to connect to frr daemon %s: %s' % (daemon, msg))
                    retry_cnt += 1
                    if retry_cnt > 100 or not main_loop:
                        syslog.syslog(syslog.LOG_ERR, 're-tried too many times, give up')
                        for _, sock in self.client_socks.items():
                            sock.close()
                        return False
                    time.sleep(2)
                    continue
            sock.settimeout(120)
            self.client_socks[daemon] = sock
        for daemon, sock in self.client_socks.items():
            syslog.syslog(syslog.LOG_DEBUG, 'send initial enable command to %s' % daemon)
            try:
                self.__send_data(sock, 'enable\0')
            except socket.error as msg:
                syslog.syslog(syslog.LOG_ERR, 'failed to send initial enable command to %s' % daemon)
                return False
            ret_code, reply = self.__get_reply(sock)
            if ret_code is None:
                syslog.syslog(syslog.LOG_ERR, 'failed to get command response for enable command from %s' % daemon)
                return False
            if ret_code != 0:
                syslog.syslog(syslog.LOG_ERR, 'enable command failed: ret_code=%d' % ret_code)
                syslog.syslog(syslog.LOG_ERR, reply)
                return False
        return True
    def __init__(self):
        super(BgpdClientMgr, self).__init__(name = 'VTYSH sub-process manager')
        if not self.__create_frr_client():
            syslog.syslog(syslog.LOG_ERR, 'failed to create socket to FRR daemon')
            raise RuntimeError('connect to FRR daemon failed')
        self.proxy_running = True
        self.lock = threading.Lock()
        self.proxy_sock = self.__create_proxy_socket()
        self.cmd_to_daemon = []
        for pat, daemons in self.VTYSH_CMD_DAEMON:
            try:
                self.cmd_to_daemon.append((re.compile(pat), daemons))
            except Exception:
                syslog.syslog(syslog.LOG_ERR, 'invalid regex format: %s' % pat)
                continue
    def __get_cmd_daemons(self, cmd_list):
        cmn_daemons = None
        for cmd in cmd_list:
            found = False
            for re_comp, daemons in self.cmd_to_daemon:
                if re_comp.match(cmd.strip()) is not None:
                    found = True
                    break
            if not found:
                syslog.syslog(syslog.LOG_ERR, 'no matched daemons found for command %s' % cmd)
                return None
            if cmn_daemons is None:
                cmn_daemons = set(daemons)
            else:
                cmn_daemons = cmn_daemons.intersection(set(daemons))
                if len(cmn_daemons) == 0:
                    return []
        return list(cmn_daemons)
    def __proc_command(self, command, daemons):
        syslog.syslog(syslog.LOG_DEBUG, 'VTYSH CMD: %s daemons: %s' % (command, daemons))
        resp = ''
        ret_val = False
        for daemon in daemons:
            sock = self.client_socks.get(daemon, None)
            if sock is None:
                syslog.syslog(syslog.LOG_ERR, 'daemon %s is not connected' % daemon)
                continue
            try:
                self.__send_data(sock, command + '\0')
            except socket.error as msg:
                syslog.syslog(syslog.LOG_ERR, 'failed to send command to frr daemon: %s' % msg)
                return (False, None)
            ret_code, reply = self.__get_reply(sock)
            if ret_code is None or ret_code != 0:
                if ret_code is None:
                    syslog.syslog(syslog.LOG_ERR, 'failed to get reply from frr daemon')
                    continue
                else:
                    syslog.syslog(syslog.LOG_DEBUG, '[%s] command return code: %d' % (daemon, ret_code))
                    syslog.syslog(syslog.LOG_DEBUG, reply)
            else:
                # command is running successfully by at least one daemon
                ret_val = True
            resp += reply
        return (ret_val, resp)
    def run_vtysh_command(self, table, command, daemons):
        if not command.startswith(self.VTYSH_MARK):
            syslog.syslog(syslog.LOG_ERR, 'command %s is not for vtysh config' % command)
            return False
        cmd_line = command[len(self.VTYSH_MARK):]
        cmd_list = re.findall(r"-c\s+'([^']+)'\s*", cmd_line)
        cmd_list.append('end')
        if daemons is None:
            daemons = self.TABLE_DAEMON.get(table, None)
        if daemons is None:
            daemons = self.__get_cmd_daemons(cmd_list)
        if daemons is None or len(daemons) == 0:
            syslog.syslog(syslog.LOG_ERR, 'no common daemon list found for given commands')
            return False
        ret_val = True
        with self.lock:
            for cmd in cmd_list:
                succ, _ = self.__proc_command(cmd.strip(), daemons)
                if not succ:
                    ret_val = False
        return ret_val
    @staticmethod
    def __read_all(sock, data_len):
        in_buf = io.StringIO()
        left_len = data_len
        while left_len > 0:
            data = sock.recv(left_len)
            if data is None:
                break
            in_buf.write(data)
            left_len -= len(data)
        return in_buf.getvalue()
    def shutdown(self):
        syslog.syslog(syslog.LOG_DEBUG, 'terminate bgpd client manager')
        if self.is_alive():
            self.proxy_running = False
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                sock.connect(self.PROXY_SERVER_ADDR)
            finally:
                sock.close()
            self.join()
        for _, sock in self.client_socks.items():
            sock.close()
    def run(self):
        syslog.syslog(syslog.LOG_DEBUG, 'entering VTYSH proxy thread')
        while self.proxy_running:
            syslog.syslog(syslog.LOG_DEBUG, 'waiting for client connection ...')
            conn_sock, clnt_addr = self.proxy_sock.accept()
            if not self.proxy_running:
                conn_sock.close()
                break
            try:
                syslog.syslog(syslog.LOG_DEBUG, 'client connection from %s' % clnt_addr)
                data = self.__read_all(conn_sock, 4)
                if len(data) == 4:
                    data_len = struct.unpack('>I', data)[0]
                    in_cmd = self.__read_all(conn_sock, data_len)
                    if len(in_cmd) == data_len:
                        daemons, in_cmd = extract_cmd_daemons(in_cmd)
                        in_lines = in_cmd.splitlines()
                        if daemons is None:
                            daemons = self.__get_cmd_daemons(in_lines)
                        if daemons is not None and len(daemons) > 0:
                            with self.lock:
                                for line in in_lines:
                                    _, reply = self.__proc_command(line.strip(), daemons)
                                    if reply is not None:
                                        self.__send_data(conn_sock, reply)
                                    else:
                                        syslog.syslog(syslog.LOG_ERR, 'failed running VTYSH command')
                        else:
                            syslog.syslog(syslog.LOG_ERR, 'could not find common daemons for input commands')
                    else:
                        syslog.syslog(syslog.LOG_ERR, 'read data of length %d is not expected length %d' % (data_len, len(in_cmd)))
                else:
                    syslog.syslog(syslog.LOG_ERR, 'invalid data length %d' % len(data))
            except socket.error as msg:
                syslog.syslog(syslog.LOG_ERR, 'socket writing failed: %s' % msg)
            finally:
                syslog.syslog(syslog.LOG_DEBUG, 'closing data socket from client')
                conn_sock.close()
        syslog.syslog(syslog.LOG_DEBUG, 'leaving VTYSH proxy thread')
class BGPPeerGroup:
    def __init__(self, vrf):
        self.vrf = vrf
        self.ref_nbrs = set()

def get_command_cmn(daemon, cmd_str, op, st_idx, vals, bool_values):
    chk_val = None
    if op == CachedDataWithOp.OP_DELETE:
        if bool_values is not None and len(bool_values) >= 3:
            # set to default if given
            cmd_enable = bool_values[2]
        else:
            cmd_enable = False
    else:
        cmd_enable = True
        if bool_values is not None:
            if len(vals) <= st_idx:
                syslog.syslog(syslog.LOG_ERR, 'No bool token of index %d for running cmd: %s' % (st_idx, cmd_str))
                return None
            chk_val = vals[st_idx]
            if type(chk_val) is dict:
                cmd_enable = False
                for _, v in chk_val.items():
                    if not v[1]:
                        continue
                    if v[0] == bool_values[0]:
                        cmd_enable = True
                        break
            else:
                if chk_val == bool_values[0]:
                    cmd_enable = True
                elif chk_val == bool_values[1]:
                    cmd_enable = False
                else:
                    syslog.syslog(syslog.LOG_ERR, 'Input token %s is neither %s or %s for cmd: %s' %
                                              (chk_val, bool_values[0], bool_values[1], cmd_str))
                    return None
        else:
            cmd_enable = True
    cmd_args = []
    for idx in range(len(vals)):
        if bool_values is not None and idx == st_idx:
            continue
        cmd_args.append(CommandArgument(daemon, cmd_enable, vals[idx]))
    return [cmd_str.format(*cmd_args, no = CommandArgument(daemon, cmd_enable))]


def hdl_set_extcomm(daemon, cmd_str, op, st_idx, args, is_inline):
    if is_inline:
        if type(args[0]) is list:
            com_list = args[0]
        else:
            com_list = [args[0]]
    else:
        com_set = daemon.extcomm_set_list.get(args[0], None)
        if com_set is None or not com_set.is_configurable():
            syslog.syslog(syslog.LOG_ERR, 'extended community set %s not found or configured' % args[0])
            return None
        com_list = com_set.mbr_list
    rt_cnt = soo_cnt = 0
    for comm in com_list:
        if comm.startswith(CommunityList.RT_TYPE_MARK):
            rt_cnt += 1
        elif comm.startswith(CommunityList.SOO_TYPE_MARK):
            soo_cnt += 1
    cmd_list = []
    if op != CachedDataWithOp.OP_DELETE:
        for comm_type in ['rt', 'soo']:
            cmd_list.append(('no set extcommunity %s' % comm_type, True))
    if rt_cnt > 0:
        new_args = ((args[0], True),) + args[1:]
        cmd_list += get_command_cmn(daemon, cmd_str, op, st_idx, new_args, None)
    if soo_cnt > 0:
        new_args = ((args[0], False),) + args[1:]
        cmd_list += get_command_cmn(daemon, cmd_str, op, st_idx, new_args, None)
    return cmd_list

def hdl_set_asn(daemon, cmd_str, op, st_idx, args, data):
    if 0 not in args[0]:
        return None
    if op == CachedDataWithOp.OP_DELETE:
        if args[0][0][1]:
            args[0][1] = ('0', True)
        else:
            args[0].pop(1, None)
            op = CachedDataWithOp.OP_UPDATE
    return get_command_cmn(daemon, cmd_str, op, st_idx, args, data)

def hdl_set_asn_list(daemon, cmd_str, op, st_idx, args, data):
    if op == CachedDataWithOp.OP_DELETE:
        args = ('',)
    return get_command_cmn(daemon, cmd_str, op, st_idx, args, data)

def hdl_set_pim_hello_parms (daemon, cmd_str, op, st_idx, args, data):
    if op == CachedDataWithOp.OP_DELETE:
        args = ('',)
    return get_command_cmn(daemon, cmd_str, op, st_idx, args, data)

def handle_rmap_set_metric(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    syslog.syslog(syslog.LOG_INFO, 'handle_rmap_set_metric cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    arg_len = len(args)
    metric_action = args[0] if arg_len >= 1  else ''
    metric_value = args[1] if arg_len >= 2  else ''
    med_value = args[2] if arg_len >= 3  else ''
    metric_param = ''

    if metric_action != '' :
        if metric_value != '' :
            if metric_action == 'METRIC_SET_VALUE' :
                metric_param = "{}".format(metric_value)
            elif metric_action == 'METRIC_ADD_VALUE' :
                metric_param = "+{}".format(metric_value)
            elif metric_action == 'METRIC_SUBTRACT_VALUE' :
                metric_param = "-{}".format(metric_value)
        if metric_action == 'METRIC_SET_RTT' :
            metric_param = "rtt"
        elif metric_action == 'METRIC_ADD_RTT' :
            metric_param = "+rtt"
        elif metric_action == 'METRIC_SUBTRACT_RTT' :
            metric_param = "-rtt"

    if metric_param == '' and med_value != '' :
        metric_param = "{}".format(med_value)

    if op == CachedDataWithOp.OP_DELETE :
        metric_param = ''
    else :
        if metric_param == '' :
            syslog.syslog(syslog.LOG_ERR, 'handle_rmap_set_metric not set for {}'.format(args))
            return None

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, metric_param)))
    return cmd_list

class BGPKeyMapInfo:
    def __init__(self, cmd_str, hdlr, data):
        self.daemons, self.run_cmd = extract_cmd_daemons(cmd_str)
        if hdlr is None:
            self.hdl_func = get_command_cmn
        else:
            self.hdl_func = hdlr
        self.data = data
    def same_daemons(self, other):
        dset = lambda d: set() if d is None else set(d)
        return dset(self.daemons) == dset(other.daemons)
    def __eq__(self, other):
        return self.same_daemons(other) and self.run_cmd == other.run_cmd
    def __ne__(self, other):
        return not self.same_daemons(other) or self.run_cmd != other.run_cmd
    def __hash__(self):
        return hash(self.run_cmd)
    def get_command(self, daemon, op, st_idx, *vals):
        return self.hdl_func(daemon, self.run_cmd, op, st_idx, vals, self.data)
    def __str__(self):
        ret_str = '[CMD: %s' % self.run_cmd
        if self.hdl_func == get_command_cmn and self.data is not None:
            ret_str += ' BOOL: %s/%s' % (self.data[0], self.data[1])
            if len(self.data) >= 3:
                ret_str += ' DFT: %s' % self.data[2]
        ret_str += ']'
        return ret_str

class BGPKeyMapList(list):
    def __init__(self, key_map_list, table_name, table_key = None):
        super(BGPKeyMapList, self).__init__()
        self.table_name = table_name
        self.table_key = table_key
        for key_map in key_map_list:
            if len(key_map) < 2:
                continue
            db_field = key_map[0]
            cmd_str = key_map[1]
            hdl_data = None
            hdl_func = None
            if len(key_map) >= 3:
                if callable(key_map[2]):
                    hdl_func = key_map[2]
                    if len(key_map) >= 4:
                        hdl_data = key_map[3]
                else:
                    if len(key_map[2]) < 2:
                        continue
                    hdl_data = key_map[2]
            fld_name, fld_key = self.get_map_field_key(db_field)
            if fld_name is not None:
                if table_key is not None and fld_name in table_key and table_key[fld_name] != fld_key:
                    continue
                if type(db_field) is str:
                    db_field = fld_name
                elif type(db_field) is list:
                    db_field = copy.copy(db_field)
                    try:
                        idx = db_field.index('|'.join([fld_name, fld_key]))
                        db_field[idx] = fld_name
                    except ValueError:
                        pass
            super(BGPKeyMapList, self).append((db_field, BGPKeyMapInfo(cmd_str, hdl_func, hdl_data)))
    def __eq__(self, other):
        return super(BGPKeyMapList, self).__eq__(other) and self.table_name == other.table_name and self.table_key == other.table_key
    def __ne__(self, other):
        return super(BGPKeyMapList, self).__ne__(other) or self.table_name != other.table_name or self.table_key != other.table_key
    @staticmethod
    def get_map_field_key(field):
        if type(field) is str:
            field = [field]
        elif type(field) is not list:
            return (None, None)
        for idx in range(len(field)):
            tokens = field[idx].split('|', 1)
            if len(tokens) == 2:
                return tokens
        return (None, None)
    @staticmethod
    def get_cmd_data(key_list, req_idx_list, opt_idx_list, data, chg_list, no_chg_list, merge_data, is_del):
        for idx in req_idx_list:
            if idx not in chg_list and idx not in no_chg_list:
                syslog.syslog(syslog.LOG_DEBUG, 'mandatory key %s of idx %d not found in list' % (key_list[idx], idx))
                return None
        if merge_data:
            op = CachedDataWithOp.OP_DELETE if is_del else CachedDataWithOp.OP_UPDATE
            cmd_data = {}
            for idx in chg_list:
                key = key_list[idx]
                cmd_data[idx] = (data[key].data, True)
            for idx in no_chg_list:
                key = key_list[idx]
                cmd_data[idx] = (data[key].data, False)
            return ((cmd_data,), op)
        else:
            min_id = lambda id_set: None if len(id_set) == 0 else sorted(list(id_set))[0]
            min_chg_id = min_id(chg_list)
            if min_chg_id is None:
                min_id_chged = False
            else:
                if min_chg_id in opt_idx_list:
                    min_id_chged = len(no_chg_list) == 0
                else:
                    min_unchg_id = min_id(no_chg_list)
                    min_id_chged = min_unchg_id is None or min_chg_id < min_unchg_id
            op = CachedDataWithOp.OP_DELETE if is_del and min_id_chged else CachedDataWithOp.OP_UPDATE
            cmd_data = []
            for idx in range(len(key_list)):
                if (((not is_del or op == CachedDataWithOp.OP_DELETE) and (idx in chg_list or idx in no_chg_list)) or
                    (is_del and op == CachedDataWithOp.OP_UPDATE and idx in no_chg_list)):
                    cmd_data.append(data[key_list[idx]].data)
                else:
                    if idx in opt_idx_list:
                        cmd_data.append('')
                        continue
                    else:
                        # stop adding following tokens
                        break
            if len(cmd_data) == 0:
                return None
            idx = len(cmd_data)
            while idx < len(key_list):
                cmd_data.append('')
                idx += 1
            return (tuple(cmd_data), op)
    @staticmethod
    def is_cmd_covered(src_cmd, dst_cmd):
        src_tks = src_cmd.split()
        dst_tks = dst_cmd.split()
        if len(src_tks) >= len(dst_tks):
            return False
        for idx in range(len(src_tks)):
            if src_tks[idx] != dst_tks[idx]:
                return False
        return True
    @staticmethod
    def is_cmd_list_covered(src_list, dst_list):
        if len(src_list) == 0:
            return True
        if len(src_list) > len(dst_list):
            return False
        for idx in range(len(src_list)):
            if not BGPKeyMapList.is_cmd_covered(src_list[idx], dst_list[idx]):
                return False
        return True
    def run_command(self, daemon, table, data, prefix_list=None, *upper_vals):
        start_idx = len(upper_vals)
        ret_val = False
        run_cmd_cnt = 0
        for db_field, key_map in self:
            merge_vals = False
            if type(db_field) is not list and type(db_field) is not tuple:
                db_field = [db_field]
            elif type(db_field) is tuple:
                db_field = list(db_field)
                merge_vals = True

            idx = 0
            req_idx_list = []
            key_list_list = []
            opt_idx_list = set()
            run_cmd = True
            for dkey in db_field:
                optional = False
                if len(dkey) > 0 and dkey[0] == '+':
                    if len(dkey) > 1 and dkey[1] == '+':
                        opt_idx_list.add(idx)
                        dkey = dkey[2:]
                    else:
                        dkey = dkey[1:]
                    optional = True
                else:
                    req_idx_list.append(idx)
                key_list = []
                for k in dkey.split('&'):
                    if k in data and isinstance(data[k], CachedDataWithOp):
                        key_list.append(k)
                if not optional and len(key_list) == 0:
                    run_cmd = False
                    break
                if len(key_list) == 0:
                    if len(key_list_list) == 0:
                        key_list_list.append([None])
                    else:
                        for k_lst in key_list_list:
                            k_lst.append(None)
                else:
                    new_list = []
                    if len(key_list_list) == 0:
                        for k in key_list:
                            key_list_list.append([k])
                    else:
                        for k_lst in key_list_list:
                            if len(key_list) == 1:
                                k_lst.append(key_list[0])
                            else:
                                for k in key_list:
                                    new_list.append(k_lst + [k])
                    if len(new_list) > 0:
                        key_list_list = new_list
                idx += 1
            if not run_cmd:
                continue

            cmd_list_list = []
            for key_list in key_list_list:
                upd_id_list = set()
                del_id_list = set()
                no_chg_id_list = set()
                idx = 0
                for dkey in key_list:
                    if dkey is not None:
                        dval = data[dkey]
                        if dval.op == CachedDataWithOp.OP_NONE:
                            no_chg_id_list.add(idx)
                        elif dval.op == CachedDataWithOp.OP_ADD or dval.op == CachedDataWithOp.OP_UPDATE:
                            upd_id_list.add(idx)
                        elif dval.op == CachedDataWithOp.OP_DELETE:
                            del_id_list.add(idx)
                    idx += 1
                cmd_list = []
                if len(del_id_list) > 0:
                    data_val_op = self.get_cmd_data(key_list, req_idx_list, opt_idx_list, data, del_id_list, no_chg_id_list, merge_vals, True)
                    if data_val_op is not None:
                        cmd = key_map.get_command(daemon, data_val_op[1], start_idx, *(upper_vals + data_val_op[0]))
                        if cmd is not None:
                            cmd_list += cmd
                        else:
                            syslog.syslog(syslog.LOG_ERR, 'failed to get del cmd from value: %s' % data_val_op[0])
                if len(upd_id_list) > 0:
                    data_val_op = self.get_cmd_data(key_list, req_idx_list, opt_idx_list, data, upd_id_list, no_chg_id_list, merge_vals, False)
                    if data_val_op is not None:
                        cmd = key_map.get_command(daemon, data_val_op[1], start_idx, *(upper_vals + data_val_op[0]))
                        if cmd is not None:
                            cmd_list += cmd
                        else:
                            syslog.syslog(syslog.LOG_ERR, 'failed to get upd cmd from value: %s' % str(data_val_op[0]))
                if len(cmd_list) > 0:
                    cmd_list_list.append(cmd_list)
            cmd_list = []
            for chk_list in cmd_list_list:
               if self.is_cmd_list_covered(cmd_list, chk_list):
                   cmd_list = chk_list
            failed = False
            if len(cmd_list) > 0:
                run_cmd_cnt += 1
                cmd_prefix = 'vtysh '
                for pfx in prefix_list:
                    cmd_prefix += "-c '%s' " % pfx
                for cmd in cmd_list:
                    ignore_fail = False
                    if type(cmd) is tuple:
                        cmd, ignore_fail = cmd
                    if not g_run_command(table, cmd_prefix + "-c '%s'" % cmd, True, key_map.daemons, ignore_fail):
                        syslog.syslog(syslog.LOG_ERR, 'failed running FRR command: %s' % cmd)
                        failed = True
                        break
                if not failed:
                    ret_val = True
            if not failed:
                for key_list in key_list_list:
                    for dkey in key_list:
                        if dkey in data:
                            data[dkey].status = CachedDataWithOp.STAT_SUCC
        if run_cmd_cnt == 0:
            return True
        return ret_val

class CommandArgument(object):
    def __init__(self, daemon, enabled, val = None):
        self.daemon = daemon
        self.enabled = enabled
        self.value = val
        self.tolower = False
    def to_str(self):
        if type(self.value) is list or type(self.value) is tuple:
            ret_val = ' '.join([v for v in self.value])
        elif type(self.value) is dict:
            id_list = self.value.keys()
            id_list.sort()
            ret_val = ' '.join([self.value[i][0] for i in id_list])
        else:
            ret_val = str(self.value)
        if self.tolower:
            ret_val = ret_val.lower()
        return ret_val
    @staticmethod
    def parse_ext_community(com_str, is_rt = None):
        if com_str.startswith(CommunityList.RT_TYPE_MARK):
            com_str = com_str[len(CommunityList.RT_TYPE_MARK):]
            if is_rt is None:
                return 'rt %s' % com_str
            else:
                return (com_str if is_rt else None)
        elif com_str.startswith(CommunityList.SOO_TYPE_MARK):
            com_str = com_str[len(CommunityList.SOO_TYPE_MARK):]
            if is_rt is None:
                return 'soo %s' % com_str
            else:
                return (None if is_rt else com_str)
        return None
    def __format__(self, format):
        bool_format = {'allow-as-in': 'origin',
                       'match-clust-len': 'equal-cluster-length',
                       'network-backdoor': 'backdoor',
                       'aggr-as-set': 'as-set',
                       'aggr-summary-only': 'summary-only',
                       'uchg-as-path': 'as-path',
                       'uchg-med': 'med',
                       'uchg-nh': 'next-hop',
                       'rm-as-all': 'all',
                       'rm-as-repl': 'replace-AS',
                       'mp-as-set': ('as-set', 'no-as-set'),
                       'no-prepend': 'no-prepend',
                       'replace-as': 'replace-as',
                       'blackhole': 'blackhole'
        }
        if format == 'no-prefix':
            return 'no ' if not self.enabled else ''
        elif format == 'enable-only' and not self.enabled:
            return ''
        elif format == 'com-ref':
            com_set = self.daemon.comm_set_list.get(self.value, None)
            if com_set is not None and com_set.is_configurable():
                return ' '.join(com_set.mbr_list)
        elif format == 'ext-com-list':
            if type(self.value) is tuple:
                com_val, is_rt = self.value
            else:
                com_val = self.value
                is_rt = None
            if type(com_val) is list:
                com_list = com_val
            else:
                com_list = [com_val]
            frr_com_list = []
            for comm in com_list:
                frr_comm = self.parse_ext_community(comm, is_rt)
                if frr_comm is not None:
                    frr_com_list.append(frr_comm)
            if is_rt is None:
                return ' '.join(frr_com_list)
            else:
                return ('rt ' if is_rt else 'soo ') + ' '.join(frr_com_list)
        elif format == 'ext-com-ref':
            com_set_name, is_rt = self.value
            com_set = self.daemon.extcomm_set_list.get(com_set_name, None)
            if com_set is not None and com_set.is_configurable():
                frr_com_list = []
                for comm in com_set.mbr_list:
                    frr_comm = self.parse_ext_community(comm, is_rt)
                    if frr_comm is not None:
                        frr_com_list.append(frr_comm)
                return ('rt ' if is_rt else 'soo ') + ' '.join(frr_com_list)
        elif format == 'repeat' and type(self.value) is dict:
            if 1 in self.value:
                rep_cnt = int(self.value[1][0])
            else:
                rep_cnt = 1
            if 0 in self.value:
                return ' '.join([self.value[0][0]] * rep_cnt)
        elif format == 'neighbor-set':
            ret_val = BGPConfigDaemon.get_prefix_set_name(self.value, 'NEIGHBOR_SET')
            return ret_val
        elif format == 'nexthop-set':
            ret_val = BGPConfigDaemon.get_prefix_set_name(self.value, 'NEXTHOP_SET')
            return ret_val
        elif format == 'peer-ip':
            if type(self.value) is list and len(self.value) > 0:
                return self.value[0]
            else:
                return self.value
        elif format == 'tx-add-paths':
            if self.value == 'tx_all_paths':
                return 'addpath-tx-all-paths'
            elif self.value == 'tx_best_path_per_as':
                return 'addpath-tx-bestpath-per-AS'
        elif format == 'shutdown-msg':
            if len(self.value) > 0:
                self.value = 'message %s' % self.value
        elif format == 'default-rmap':
            if len(self.value) > 0:
                self.value = 'route-map %s' % self.value
        elif format in bool_format:
            false_val = ''
            if type(bool_format[format]) is tuple:
                if len(bool_format[format]) == 2:
                    true_val, false_val = bool_format[format]
                else:
                    true_val = bool_format[format][0]
            else:
                true_val = bool_format[format]
            if self.value == 'true':
                self.value = true_val
            elif self.value == 'false':
                self.value = false_val
        elif format == 'restart':
            if self.value == 'true':
                self.value = 'warning-only'
            elif self.value == 'false':
                self.value = ''
            elif len(self.value) > 0:
                self.value = 'restart %s' % self.value
        elif format == 'redist-route-map':
            if len(self.value) > 0:
                self.value = 'route-map %s' % self.to_str()
        elif format == 'redist-metric':
            if len(self.value) > 0:
                self.value = 'metric %s' % self.to_str()
        elif format == 'track':
            if len(self.value) > 0:
                self.value = 'track %s' % self.to_str()
        elif format == 'network-policy':
            if len(self.value) > 0:
                self.value = 'route-map %s' % self.to_str()
        elif format == 'src-proto':
            if self.value == 'ospf3':
                self.value = 'ospf6'
        elif format == 'aggr-policy':
            if len(self.value) > 0:
                self.value = 'route-map %s' % self.to_str()
        elif format == 'asn_list':
            self.value = ' '.join(self.value.split(','))
        elif format == 'nh-tag':
            if len(self.value) > 0:
                self.value = 'tag %s' % self.to_str()
        elif format == 'nh-vrf':
            if len(self.value) > 0:
                self.value = 'nexthop-vrf %s' % self.to_str()
        elif format == 'tolower':
            self.tolower = True
        elif format == 'pim_hello_parms':
            self.value = ' '.join(self.value.split(','))
        return self.to_str()

def hdl_send_com(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 2:
        return None
    cmd_list = []
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, 'all'), no = CommandArgument(daemon, False)))
    if op == CachedDataWithOp.OP_DELETE:
        com_type = 'all'
    else:
        com_type = args[1]
    if com_type != 'none':
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, com_type), no = CommandArgument(daemon, True)))
    return cmd_list

def hdl_rm_priv_as(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, ''), CommandArgument(daemon, True, ''),
                                   no = CommandArgument(daemon, False)))
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, 'true'), CommandArgument(daemon, True, ''),
                                   no = CommandArgument(daemon, False)))
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, ''), CommandArgument(daemon, True, 'true'),
                                   no = CommandArgument(daemon, False)))
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, 'true'), CommandArgument(daemon, True, 'true'),
                                   no = CommandArgument(daemon, False)))
    if op != CachedDataWithOp.OP_DELETE:
        cmd_list += get_command_cmn(daemon, cmd_str, op, st_idx, args, data)
    return cmd_list

def hdl_capa_orf_pfxlist(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 2:
        return None
    cmd_list = []
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, 'both'), no = CommandArgument(daemon, False)))
    if op != CachedDataWithOp.OP_DELETE:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[0]), CommandArgument(daemon, True, args[1]), no = CommandArgument(daemon, True)))
    return cmd_list

def hdl_com_set(daemon, cmd_str, op, st_idx, args, extended):
    if len(args) < 2 or 0 not in args[1] or 1 not in args[1] or 2 not in args[1]:
        return None
    com_name = args[0]
    set_type = args[1][0][0].lower()
    arg_str = '{} {}'.format(set_type, com_name)
    cmd_list = []
    com_set_list = daemon.comm_set_list if not extended else daemon.extcomm_set_list
    if com_name in com_set_list and com_set_list[com_name].is_configurable():
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, arg_str), no = CommandArgument(daemon, False)))
    if op != CachedDataWithOp.OP_DELETE:
        match_action = args[1][1][0].lower()
        member_list = args[1][2][0]
        if match_action == 'all':
            if extended and set_type == 'standard':
                mbr_str = '{} permit {:ext-com-list}'.format(arg_str, CommandArgument(daemon, True, member_list))
            else:
                mbr_str = '{} permit {}'.format(arg_str, ' '.join(member_list))
            cmd_list.append(cmd_str.format(CommandArgument(daemon, True, mbr_str), no = CommandArgument(daemon, True)))
        elif match_action == 'any':
            for member in member_list:
                if extended and set_type == 'standard':
                    mbr_str = '{} permit {:ext-com-list}'.format(arg_str, CommandArgument(daemon, True, member))
                else:
                    mbr_str = '{} permit {}'.format(arg_str, member)
                cmd_list.append(cmd_str.format(CommandArgument(daemon, True, mbr_str), no = CommandArgument(daemon, True)))
    return cmd_list

def hdl_aspath_set(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 2:
        return None
    cmd_list = []
    as_set_name = args[0]
    if as_set_name in daemon.as_path_set_list:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, as_set_name), no = CommandArgument(daemon, False)))
    if op != CachedDataWithOp.OP_DELETE and len(args[1]) > 0:
        for asn in args[1]:
            mbr_str = '{} permit {}'.format(as_set_name, asn)
            cmd_list.append(cmd_str.format(CommandArgument(daemon, True, mbr_str), no = CommandArgument(daemon, True)))
    return cmd_list

def hdl_ibgp_maxpath(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    if op != CachedDataWithOp.OP_DELETE:
        # blindly run no command first
        cmd_list = [cmd_str.format(CommandArgument(daemon, True, args[0]),
                                   CommandArgument(daemon, True, 'false'),
                                   no = CommandArgument(daemon, False))]
    upd_cmd_list = get_command_cmn(daemon, cmd_str, op, st_idx, args, data)
    if upd_cmd_list is None:
        return None
    return cmd_list + upd_cmd_list

def hdl_ospf_log(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    if (no_op == 'no '):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, "")))
    elif (args[0] == "DETAIL"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0].lower())))
    elif (args[0] == "BRIEF"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, "")))

    return cmd_list

def handle_ospf_area_auth(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    if (args[1] == "MD5HMAC"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, "message-digest")))

    elif (args[1] == "TEXT"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, "")))
    elif (args[1] == "NONE"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, 'no '),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, "")))
    return cmd_list

def handle_ospf_area_shortcut(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    if (args[1] == "DEFAULT") and (no_op != 'no '):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, "default")))

    elif (args[1] == "ENABLE"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, "enable")))
    elif (args[1] == "DISABLE"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, "disable")))
    return cmd_list

def handle_ospf_area_vlink_auth(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    if (args[2] == "MD5HMAC"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, args[1]),
                                       CommandArgument(daemon, True, "message-digest")))
    elif (args[2] == "NONE"):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, args[1]),
                                       CommandArgument(daemon, True, "null")))

    return cmd_list

def handle_ospf_area_range_advt(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []


    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    if (args[2] == 'true'):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, args[1]),
                                       CommandArgument(daemon, True, "advertise")))
    elif (args[2] == 'false'):
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                       CommandArgument(daemon, True, args[0]),
                                       CommandArgument(daemon, True, args[1]),
                                       CommandArgument(daemon, True, "not-advertise")))
    return cmd_list

def handle_ospf_abrtype(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, args[0].lower())))

    return cmd_list


def handle_ospf_if_common(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    if_addr = "" if args[1] == '0.0.0.0' else args[1]
    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''
    param_value = args[2]

    syslog.syslog(syslog.LOG_INFO, 'handle_ospf_if_common cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, param_value),
                                   CommandArgument(daemon, True, if_addr)))
    return cmd_list


def handle_ospf_if_authtype(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    if_addr = "" if args[1] == '0.0.0.0' else args[1]
    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    authtype = ''
    if args[2] == 'TEXT' :
        authtype = ''
    elif args[2] == 'MD5HMAC' :
        authtype = 'message-digest'
    elif args[2] == 'NONE' :
        authtype = 'null'
    else :
        syslog.syslog(syslog.LOG_ERR, 'handle_ospf_if_nwtype invalid auth type args {}'.format(args))

    syslog.syslog(syslog.LOG_INFO, 'handle_ospf_if_authtype cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, authtype),
                                   CommandArgument(daemon, True, if_addr)))
    return cmd_list


def handle_ospf_if_md5key(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    if_addr = "" if args[1] == '0.0.0.0' else args[1]
    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    md5key_id = args[2]
    md5key = args[3]

    syslog.syslog(syslog.LOG_INFO, 'handle_ospf_if_md5key cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, md5key_id),
                                   CommandArgument(daemon, True, md5key),
                                   CommandArgument(daemon, True, if_addr)))
    return cmd_list


def handle_ospf_if_mtu_ignore(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    if_addr = "" if args[1] == '0.0.0.0' else args[1]
    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    syslog.syslog(syslog.LOG_INFO, 'handle_ospf_if_mtu_ignore cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, if_addr)))
    return cmd_list


def handle_ospf_if_nwtype(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    no_op = 'no ' if op == CachedDataWithOp.OP_DELETE else ''

    nwtype = ''
    if args[2] == 'POINT_TO_POINT_NETWORK' :
        nwtype = 'point-to-point'
    elif args[2] == 'BROADCAST_NETWORK' :
        nwtype = 'broadcast'
    else :
        syslog.syslog(syslog.LOG_ERR, 'handle_ospf_if_nwtype invalid nw type args {}'.format(args))

    syslog.syslog(syslog.LOG_INFO, 'handle_ospf_if_nwtype cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, no_op),
                                   CommandArgument(daemon, True, nwtype)))
    return cmd_list

def handle_igmp_if_common(daemon, cmd_str, op, st_idx, args, data):
    if len(args) != 1:
        return None
    cmd_list = []
    param_value = args[0]

    syslog.syslog(syslog.LOG_INFO, 'handle_igmp_if_common cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    if op != CachedDataWithOp.OP_DELETE:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, param_value)))
    else:
        cmd_list.append('no ' + cmd_str.format(CommandArgument(daemon, True, '')))

    syslog.syslog(syslog.LOG_INFO, 'handle_igmp_if_common param {}, cmd_list {}'.format(param_value, cmd_list))
    return cmd_list

def handle_igmp_if_enable(daemon, cmd_str, op, st_idx, args, data):
    if len(args) != 1:
        return None
    cmd_list = []
    param_value = args[0]

    syslog.syslog(syslog.LOG_INFO, 'handle_igmp_if_enable cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    if op != CachedDataWithOp.OP_DELETE:
        if param_value == 'false':
            cmd_list.append('no ' + cmd_str.format(CommandArgument(daemon, True, '')))
        else:
            cmd_list.append(cmd_str.format(CommandArgument(daemon, True, '')))
    else:
        cmd_list.append('no ' + cmd_str.format(CommandArgument(daemon, True, '')))

    syslog.syslog(syslog.LOG_INFO, 'handle_igmp_if_enable param {}, cmd_list {}'.format(param_value, cmd_list))
    return cmd_list

def handle_ip_sla_common(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    syslog.syslog(syslog.LOG_INFO, 'handle_ip_sla cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))

    if op != CachedDataWithOp.OP_DELETE:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[1])))
    else:
        cmd_list.append('no ' + cmd_str.format(CommandArgument(daemon, True, '')))

    return cmd_list

def handle_ip_sla_tcp_connect(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    syslog.syslog(syslog.LOG_INFO, 'handle_ip_sla_tcp_connect cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))
    tcp_cmd_token = ("tcp", "connect")
    tcp_cmd_str = "-".join(tcp_cmd_token)
    tcp_cmd_deconfig = ' no ' + tcp_cmd_str

    if op != CachedDataWithOp.OP_DELETE:
        cmd_list.append(' ')
    else:
        cmd_list.append(tcp_cmd_deconfig)

    return cmd_list

def handle_ip_sla_icmp_echo(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []

    syslog.syslog(syslog.LOG_INFO, 'handle_ip_sla_icmp_echo cmd_str {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))
    icmp_cmd_token = ("icmp", "echo")
    icmp_cmd_str = "-".join(icmp_cmd_token)
    icmp_cmd_deconfig = ' no ' + icmp_cmd_str

    if op != CachedDataWithOp.OP_DELETE:
        cmd_list.append(' ')
        syslog.syslog(syslog.LOG_INFO, 'handle_ip_sla_icmp_echo cmd_list {}'.format(cmd_list))
    else:
        cmd_list.append(icmp_cmd_deconfig)

    syslog.syslog(syslog.LOG_INFO, 'handle_ip_sla_icmp_echo cmd_list {}'.format(cmd_list))
    return cmd_list



def hdl_af_aggregate(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 5:
        return None
    cmd_list = []
    if op != CachedDataWithOp.OP_DELETE:
        vrf = args[0]
        af = args[1]
        ip_prefix = args[2]
        if vrf in daemon.af_aggr_list and ip_prefix in daemon.af_aggr_list[vrf]:
            cmd_list.append(cmd_str.format(CommandArgument(daemon, True, vrf), CommandArgument(daemon, True, af), CommandArgument(daemon, True, ip_prefix),
                                           CommandArgument(daemon, True, ''), CommandArgument(daemon, True, ''), CommandArgument(daemon, True, ''),
                                           no = CommandArgument(daemon, False)))
    upd_cmd_list = get_command_cmn(daemon, cmd_str, op, st_idx, args, data)
    if upd_cmd_list is None:
        return None
    return cmd_list + upd_cmd_list

def hdl_route_redist_set(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    if op != CachedDataWithOp.OP_DELETE:
        proto = args[0]
        # blindly run no command first
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, proto), CommandArgument(daemon, True, ''), CommandArgument(daemon, True, ''),
                                       no = CommandArgument(daemon, False)))
    upd_cmd_list = get_command_cmn(daemon, cmd_str, op, st_idx, args, data)
    if upd_cmd_list is None:
        return None
    return cmd_list + upd_cmd_list

def hdl_attr_unchanged(daemon, cmd_str, op, st_idx, args, data):
    # blindly run no command first
    cmd_list = [cmd_str.format(CommandArgument(daemon, True, args[0]),
                               CommandArgument(daemon, True, ''),
                               CommandArgument(daemon, True, ''),
                               CommandArgument(daemon, True, ''),
                               no = CommandArgument(daemon, False))]
    if op != CachedDataWithOp.OP_DELETE:
        upd_cmd_list = get_command_cmn(daemon, cmd_str, op, st_idx, args, data)
        if upd_cmd_list is None:
            return None
        cmd_list += upd_cmd_list
    return cmd_list

def hdl_leaf_list_expansion(daemon, cmd_str, op, st_idx, args, data, table_key, item_key):
    cmd_list = []
    old_list = []
    if op != CachedDataWithOp.OP_DELETE:
        new_list = args[st_idx]
    else:
        new_list = []

    syslog.syslog(syslog.LOG_DEBUG, 'handle_leaf_list_expansion {} op {} st_idx {} args {} data {} table_key {} item_key {}'.format(
                                    cmd_str, op, st_idx, args, data, table_key, item_key))

    if table_key in daemon.table_data_cache.keys():
        cache_tbl_data = daemon.table_data_cache[table_key]
        if item_key in cache_tbl_data:
            old_list = cache_tbl_data[item_key]

    del_list = list(set(old_list) - set(new_list))
    add_list = list(set(new_list) - set(old_list))

    for value in add_list:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, value), no = CommandArgument(daemon, True)))
    for value in del_list:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, value), no = CommandArgument(daemon, False)))

    syslog.syslog(syslog.LOG_DEBUG, 'cmd_list {}'.format(cmd_list))
    return cmd_list

def hdl_import_list(daemon, cmd_str, op, st_idx, args, data):
    return hdl_leaf_list_expansion(daemon, cmd_str, op, st_idx, args, data, daemon.tmp_cache_key, 'import-rts')

def hdl_export_list(daemon, cmd_str, op, st_idx, args, data):
    return hdl_leaf_list_expansion(daemon, cmd_str, op, st_idx, args, data, daemon.tmp_cache_key, 'export-rts')

def hdl_enum_conversion(daemon, cmd_str, op, st_idx, args, data):
    cmd_list = []
    syslog.syslog(syslog.LOG_DEBUG, 'handle_enum_conversion {} op {} st_idx {} args {} data {}'.format(
                                    cmd_str, op, st_idx, args, data))
    cmd_list.append(cmd_str.format(CommandArgument(daemon, True, args[st_idx].lower().replace('_','-')),
        no = CommandArgument(daemon, (op != CachedDataWithOp.OP_DELETE))))
    syslog.syslog(syslog.LOG_DEBUG, 'cmd_list {}'.format(cmd_list))
    return cmd_list

def hdl_confed_peers(daemon, cmd_str, op, st_idx, args, data):
    del_list = []
    add_list = []
    if op == CachedDataWithOp.OP_DELETE:
        del_list = list(daemon.upd_confed_peers)
        daemon.upd_confed_peers.clear()
    else:
        for peer in args[0]:
            if peer not in daemon.upd_confed_peers:
                add_list.append(peer)
            else:
                daemon.upd_confed_peers.remove(peer)
        del_list = list(daemon.upd_confed_peers)
        daemon.upd_confed_peers = set(args[0])
    cmd_list = []
    if len(del_list) > 0:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, del_list), no = CommandArgument(daemon, False)))
    if len(add_list) > 0:
        cmd_list.append(cmd_str.format(CommandArgument(daemon, True, add_list), no = CommandArgument(daemon, True)))
    return cmd_list

def hdl_static_route(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 6:
        return None
    vrf = args[0]
    ip_prefix = args[1]
    af = data
    if op == CachedDataWithOp.OP_DELETE:
        ip_nh_set = IpNextHopSet(af)
    else:
        arg_list = lambda v: v.split(',') if len(v.strip()) != 0 else None
        bkh_list = arg_list(args[2])
        nh_list = arg_list(args[3])
        track_list = arg_list(args[5])
        intf_list = arg_list(args[4])
        tag_list = arg_list(args[6])
        dist_list = arg_list(args[7])
        nh_vrf_list = arg_list(args[8])
        ip_nh_set = IpNextHopSet(af, bkh_list, nh_list, track_list, intf_list, tag_list, dist_list, nh_vrf_list)
    cur_nh_set = daemon.static_route_list.get(vrf, {}).get(ip_prefix, IpNextHopSet(af))
    diff_set = ip_nh_set.symmetric_difference(cur_nh_set)
    op_cmd_list = {}
    for ip_nh in diff_set:
        if ip_nh in cur_nh_set:
            op = CachedDataWithOp.OP_DELETE
        else:
            op = CachedDataWithOp.OP_ADD
        try:
            op_cmds = op_cmd_list.setdefault(op, [])
            op_cmds += get_command_cmn(daemon, cmd_str, op, st_idx, [ip_prefix] + ip_nh.get_arg_list(), None)
        except socket.error:
            syslog.syslog(syslog.LOG_ERR, 'Invalid IP in next-hop %s' % ip_nh)
            return None
    cmd_list = op_cmd_list.get(CachedDataWithOp.OP_DELETE, [])
    cmd_list += op_cmd_list.get(CachedDataWithOp.OP_ADD, [])
    daemon.upd_nh_set = ip_nh_set
    return cmd_list

def hdl_admin_status(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 1:
        return None

    cmd_list = []
    status = args[st_idx]

    # Convert up/down to true/false if needed
    if status == 'up':
        status = 'true'
    elif status == 'down':
        status = 'false'
    elif status not in ['true', 'false']:
        return None

    # For delete operation, treat as 'false'
    if op == CachedDataWithOp.OP_DELETE:
        status = 'false'

    # Apply the command with appropriate no prefix
    cmd_list.append(cmd_str.format(
        CommandArgument(daemon, True, args[0]),
        no=CommandArgument(daemon, (status == 'true'))))

    return cmd_list

def hdl_admin_status_shutdown_msg(daemon, cmd_str, op, st_idx, args, data):
    if len(args) < 2:
        return None

    cmd_list = []
    status = args[st_idx]
    shutdown_msg = args[st_idx + 1] if op != CachedDataWithOp.OP_DELETE else ""

    # Convert up/down to true/false if needed
    if status == 'up':
        status = 'true'
    elif status == 'down':
        status = 'false'
    elif status not in ['true', 'false']:
        return None

    # Apply the command with appropriate no prefix
    cmd_list.append(cmd_str.format(
        CommandArgument(daemon, True, args[0]),
        CommandArgument(daemon, True, shutdown_msg),
        no=CommandArgument(daemon, (status == 'false'))).rstrip())

    return cmd_list

class ExtConfigDBConnector(ConfigDBConnector):
    def __init__(self, ns_attrs = None):
        super(ExtConfigDBConnector, self).__init__()
        self.nosort_attrs = ns_attrs if ns_attrs is not None else {}
        self.__listen_thread_running = False
    def raw_to_typed(self, raw_data, table = ''):
        if len(raw_data) == 0:
            raw_data = None
        data = super(ExtConfigDBConnector, self).raw_to_typed(raw_data)
        if data is None:
            return None
        for key, val in data.items():
            if type(val) is list and key not in self.nosort_attrs.get(table, set()):
                val.sort()
        return data
    def sub_msg_handler(self, msg_item):
        if msg_item['type'] == 'pmessage':
            key = msg_item['channel'].split(':', 1)[1]
            try:
                (table, row) = key.split(self.TABLE_NAME_SEPARATOR, 1)
                if table in self.handlers:
                    client = self.get_redis_client(self.db_name)
                    data = self.raw_to_typed(client.hgetall(key), table)
                    super(ExtConfigDBConnector, self)._ConfigDBConnector__fire(table, row, data)
            except ValueError:
                pass    #Ignore non table-formated redis entries
            except Exception as e:
                syslog.syslog(syslog.LOG_ERR, '[bgp cfgd] Failed handling config DB update with exception:' + str(e))
                logging.exception(e)

    def listen_thread(self, timeout):
        self.__listen_thread_running = True
        sub_key_space = "__keyspace@{}__:*".format(self.get_dbid(self.db_name))
        self.pubsub.psubscribe(sub_key_space)
        while self.__listen_thread_running:
            msg = self.pubsub.get_message(timeout, True)
            if msg:
                self.sub_msg_handler(msg)

        self.pubsub.punsubscribe(sub_key_space)

    def listen(self):
        """Start listen Redis keyspace events and will trigger corresponding handlers when content of a table changes.
        """
        self.pubsub = self.get_redis_client(self.db_name).pubsub()
        self.sub_thread = threading.Thread(target=self.listen_thread, args=(10,))
        self.sub_thread.start()

    def stop_listen(self):
        self.__listen_thread_running = False

    @staticmethod
    def get_table_key(table, key):
        return table + '&&' + key
    def get_table_data(self, table_list):
        ret_data = {}
        for table in table_list:
            table_data = self.get_table(table)
            for key, data in table_data.items():
                table_key = self.get_table_key(table, self.serialize_key(key))
                ret_data[table_key] = data
        return ret_data

class CommunityList:
    MATCH_ALL = 0
    MATCH_ANY = 1
    RT_TYPE_MARK = 'route-target:'
    SOO_TYPE_MARK = 'route-origin:'
    def __init__(self, name, extended):
        self.name = name
        self.is_ext = extended
        self.match_action = None
        self.is_std = None
        self.mbr_list = []
    def is_configurable(self):
        return (self.match_action is not None and self.is_std is not None and
                len(self.mbr_list) > 0)
    def db_data_to_attr(self, name, val):
        if name == 'match_action':
            if val is None:
                self.match_action = None
            else:
                if val.lower() == 'all':
                    self.match_action = self.MATCH_ALL
                else:
                    self.match_action = self.MATCH_ANY
        elif name == 'set_type':
            if val is None:
                self.is_std = None
            else:
                self.is_std = (val.lower() == 'standard')
        elif name == 'community_member':
            self.mbr_list = []
            if val is not None:
                if type(val) is not list:
                    self.mbr_list = val.split(',')
                else:
                    self.mbr_list = val

class MatchPrefix:
    IPV4_MAXLEN = 32
    IPV6_MAXLEN = 128
    @staticmethod
    def normalize_ip_prefix(af, ip_prefix):
        ip_mask = ip_prefix.split('/')
        ip_addr = ip_mask[0]
        if len(ip_mask) < 2:
            mask_len = MatchPrefix.IPV4_MAXLEN if af == socket.AF_INET else MatchPrefix.IPV6_MAXLEN
            return '%s/%d' % (ip_addr, mask_len)
        mask_len = int(ip_mask[1])
        ip_net = netaddr.IPNetwork('%s/%d' % (ip_addr, mask_len))
        try:
            normal_ip = socket.inet_ntop(af, ip_net.cidr.ip.packed)
        except ValueError:
            return None
        return '%s/%d' % (normal_ip, mask_len)
    def __init__(self, af, ip_prefix, len_range = None, action = 'permit'):
        self.ip_prefix = self.normalize_ip_prefix(af, ip_prefix)
        if self.ip_prefix is None:
            raise ValueError
        if len_range is not None:
            min_len, max_len = len_range.split('..')
            self.min_len = int(min_len)
            self.max_len = int(max_len)
            _, pfx_len = self.ip_prefix.split('/')
            if int(pfx_len) >= self.min_len:
                self.min_len = None
            elif ((af == socket.AF_INET and self.max_len == self.IPV4_MAXLEN) or
                  (af == socket.AF_INET6 and self.max_len == self.IPV6_MAXLEN)):
                self.max_len = None
        else:
            self.min_len = self.max_len = None
        self.action = action
    def __hash__(self):
        return hash((self.ip_prefix, self.min_len, self.max_len))
    def __str__(self):
        ret_str = '%s %s' % (self.action.lower(), self.ip_prefix)
        if self.min_len is not None:
            ret_str += ' ge %d' % self.min_len
        if self.max_len is not None:
            ret_str += ' le %d' % self.max_len
        return ret_str
    def __eq__(self, other):
        return (self.ip_prefix == other.ip_prefix and
                self.min_len == other.min_len and
                self.max_len == other.max_len)
    def __ne__(self, other):
        return (self.ip_prefix != other.ip_prefix or
                self.min_len != other.min_len or
                self.max_len != other.max_len)

class MatchPrefixList(list):
    def __init__(self, af_mode = None):
        super(MatchPrefixList, self).__init__()
        if af_mode is None:
            self.af = None
        else:
            self.af = socket.AF_INET if af_mode == 'ipv4' else socket.AF_INET6
    def __eq__(self, other):
        return super(MatchPrefixList, self).__eq__(other) and self.af == other.af
    def __ne__(self, other):
        return super(MatchPrefixList, self).__ne__(other) or self.af != other.af
    @staticmethod
    def __get_ip_af(ip_pfx):
        ip_pfx = ip_pfx.split('/')
        ip_addr = ip_pfx[0]
        af_list = [socket.AF_INET, socket.AF_INET6]
        for af in af_list:
            try:
                socket.inet_pton(af, ip_addr)
                return af
            except socket.error:
                continue
        return None
    def add_prefix(self, ip_pfx, len_range = None, action = 'permit'):
        af = self.__get_ip_af(ip_pfx)
        if self.af is None:
            self.af = af
        else:
            if self.af != af:
                syslog.syslog(syslog.LOG_ERR, 'af of prefix %s is not the  same as prefix set' % ip_pfx)
                raise ValueError
        self.append(MatchPrefix(self.af, ip_pfx, len_range, action))
        return self[-1]
    def get_prefix(self, ip_pfx, len_range = None, action = 'permit'):
        if self.af is None:
            return (None, None)
        prefix = MatchPrefix(self.af, ip_pfx, len_range, action)
        try:
            idx = self.index(prefix)
        except ValueError:
            return (None, None)
        return (self[idx], idx)

class AggregateAddr:
    def __init__(self):
        self.as_set = False
        self.summary_only = False

class IpNextHop:
    def __init__(self, af_id, blackhole, dst_ip, track, if_name, tag, dist, vrf):
        zero_ip = lambda af: '0.0.0.0' if af == socket.AF_INET else '::'
        self.af = af_id
        self.blackhole = 'false' if blackhole is None or blackhole == '' else blackhole
        self.distance = 0 if dist is None else int(dist)
        self.track = 0 if track is None else int(track)
        if self.blackhole == 'true':
            dst_ip = if_name = vrf = None
        self.ip = zero_ip(af_id) if dst_ip is None else dst_ip
        self.interface = '' if if_name is None else if_name
        self.tag = 0 if tag is None else int(tag)
        self.nh_vrf = '' if vrf is None else vrf
        if not self.is_portchannel():
            self.is_ip_valid()
        if self.blackhole != 'true' and self.is_zero_ip() and not self.is_portchannel() and len(self.interface.strip()) == 0:
            syslog.syslog(syslog.LOG_ERR, 'Mandatory attribute not found for nexthop')
            raise ValueError
    def __eq__(self, other):
        return (self.af == other.af and self.blackhole == other.blackhole and
                self.ip == other.ip and self.track == other.track and self.interface == other.interface and
                self.tag == other.tag and self.distance == other.distance and self.nh_vrf == other.nh_vrf)
    def __ne__(self, other):
        return (self.af != other.af or self.blackhole != other.blackhole or
                self.ip != other.ip or self.track != other.track or self.interface != other.interface or
                self.tag != other.tag or self.distance != other.distance or self.nh_vrf != other.nh_vrf)
    def __hash__(self):
        return hash((self.af, self.blackhole, self.ip, self.track, self.interface, self.tag, self.distance, self.nh_vrf))
    def __str__(self):
        return 'AF %d BKH %s IP %s TRACK %d INTF %s TAG %d DIST %d VRF %s' % (
                self.af, self.blackhole, self.ip, self.track, self.interface, self.tag, self.distance, self.nh_vrf)
    def is_ip_valid(self):
        socket.inet_pton(self.af, self.ip)
    def is_zero_ip(self):
        try:
            return sum([x for x in socket.inet_pton(self.af, self.ip)]) == 0
        except socket.error:
            return False
    def is_portchannel(self):
        return True if self.ip.startswith('PortChannel') else False
    def get_arg_list(self):
        arg = lambda x: '' if x is None else x
        num_arg = lambda x: '' if x is None or x == 0 else str(x)
        ip_arg = lambda : '' if self.ip is None else ('' if self.is_zero_ip() else self.ip)
        return [self.blackhole, ip_arg(), arg(self.interface), num_arg(self.track), num_arg(self.tag), num_arg(self.distance), arg(self.nh_vrf)]

class IpNextHopSet(set):
    def __init__(self, af, bkh_list = None, ip_list = None, track_list = None, intf_list = None, tag_list = None, dist_list = None, vrf_list = None):
        super(IpNextHopSet, self).__init__()
        if bkh_list is None and ip_list is None and intf_list is None:
            # empty set, for delete case
            return
        nums = {len(x) for x in [bkh_list, ip_list, track_list, intf_list, tag_list, dist_list, vrf_list] if x is not None}
        if len(nums) != 1:
            syslog.syslog(syslog.LOG_ERR, 'Lists of next-hop attribute have different sizes: %s' % nums)
            for x in [bkh_list, ip_list, track_list, intf_list, tag_list, dist_list, vrf_list]:
                syslog.syslog(syslog.LOG_DEBUG, 'List: %s' % x)
            return
        nh_cnt = nums.pop()
        item = lambda lst, i: lst[i] if lst is not None else None
        for idx in range(nh_cnt):
            try:
                self.add(IpNextHop(af, item(bkh_list, idx), item(ip_list, idx), item(track_list, idx), item(intf_list, idx),
                                   item(tag_list, idx), item(dist_list, idx), item(vrf_list, idx), ))
            except ValueError:
                continue
    @staticmethod
    def get_af_norm_prefix(ip_prefix):
        for af_id in [socket.AF_INET, socket.AF_INET6]:
            new_prefix = MatchPrefix.normalize_ip_prefix(af_id, ip_prefix)
            if new_prefix is not None:
                return (af_id, new_prefix)
        return (None, None)

class BGPConfigDaemon:
    DEFAULT_VRF = 'default'

    global_key_map = [('router_id',                                     '{no:no-prefix}bgp router-id {}'),
                      ('sid_vpn_per_vrf_export_explicit',               '{no:no-prefix}sid vpn per-vrf export explicit', ['true', 'false']),
                      (['load_balance_mp_relax', '+as_path_mp_as_set'], '{no:no-prefix}bgp bestpath as-path multipath-relax {:mp-as-set}', ['true', 'false']),
                      ('always_compare_med',                            '{no:no-prefix}bgp always-compare-med', ['true', 'false']),
                      ('external_compare_router_id',                    '{no:no-prefix}bgp bestpath compare-routerid', ['true', 'false']),
                      ('ignore_as_path_length',                         '{no:no-prefix}bgp bestpath as-path ignore', ['true', 'false']),
                      ('graceful_restart_enable',                       '{no:no-prefix}bgp graceful-restart', ['true', 'false']),
                      ('gr_restart_time',                               '{no:no-prefix}bgp graceful-restart restart-time {}'),
                      ('gr_stale_routes_time',                          '{no:no-prefix}bgp graceful-restart stalepath-time {}'),
                      ('gr_preserve_fw_state',                          '{no:no-prefix}bgp graceful-restart preserve-fw-state', ['true', 'false']),
                      ('log_nbr_state_changes',                         '{no:no-prefix}bgp log-neighbor-changes', ['true', 'false']),
                      ('rr_cluster_id',                                 '{no:no-prefix}bgp cluster-id {}'),
                      ('rr_allow_out_policy',                           '{no:no-prefix}bgp route-reflector allow-outbound-policy', ['true', 'false']),
                      ('disable_ebgp_connected_rt_check',               '{no:no-prefix}bgp disable-ebgp-connected-route-check', ['true', 'false']),
                      ('fast_external_failover',                        '{no:no-prefix}bgp fast-external-failover', ['true', 'false', True]),
                      ('network_import_check',                          '{no:no-prefix}bgp network import-check', ['true', 'false']),
                      ('graceful_shutdown',                             '{no:no-prefix}bgp graceful-shutdown', ['true', 'false']),
                      ('rr_clnt_to_clnt_reflection',                    '{no:no-prefix}bgp client-to-client reflection', ['true', 'false', True]),
                      ('max_dynamic_neighbors',                         '{no:no-prefix}bgp listen limit {}'),
                      ('read_quanta',                                   '{no:no-prefix}read-quanta {}'),
                      ('write_quanta',                                  '{no:no-prefix}write-quanta {}'),
                      ('coalesce_time',                                 '{no:no-prefix}coalesce-time {}'),
                      ('route_map_process_delay',                       '{no:no-prefix}bgp route-map delay-timer {}'),
                      ('deterministic_med',                             '{no:no-prefix}bgp deterministic-med', ['true', 'false']),
                      ('med_confed',                                    '{no:no-prefix}bgp bestpath med confed', ['true', 'false']),
                      ('med_missing_as_worst',                          '{no:no-prefix}bgp bestpath med missing-as-worst', ['true', 'false']),
                      ('compare_confed_as_path',                        '{no:no-prefix}bgp bestpath as-path confed', ['true', 'false']),
                      ('default_ipv4_unicast',                          '{no:no-prefix}bgp default ipv4-unicast', ['true', 'false']),
                      ('default_local_preference',                      '{no:no-prefix}bgp default local-preference {}'),
                      ('default_show_hostname',                         '{no:no-prefix}bgp default show-hostname', ['true', 'false']),
                      ('default_shutdown',                              '{no:no-prefix}bgp default shutdown', ['true', 'false']),
                      ('default_subgroup_pkt_queue_max',                '{no:no-prefix}bgp default subgroup-pkt-queue-max {}'),
                      (['max_med_time', '+max_med_val'],                '{no:no-prefix}bgp max-med on-startup {} {}'),
                      (['max_delay', '+establish_wait'],                '{no:no-prefix}update-delay {} {}'),
                      ('confed_id',                                     '{no:no-prefix}bgp confederation identifier {}'),
                      ('confed_peers',                                  '{no:no-prefix}bgp confederation peers {}', hdl_confed_peers),
                      (['keepalive', 'holdtime'],                       '{no:no-prefix}timers bgp {} {}'),
                      (['max_med_admin', '+max_med_admin_val'],         '{no:no-prefix}bgp max-med administrative {}', ['true', 'false'])
    ]

    global_af_key_map = [(['ebgp_route_distance',
                           'ibgp_route_distance',
                           'local_route_distance'],                     '{no:no-prefix}distance bgp {} {} {}'),
                         ('rd_vpn_export',                              '{no:no-prefix}rd vpn export {}'),
                         ('rt_vpn_export',                              '{no:no-prefix}rt vpn export {}'),
                         ('rt_vpn_import',                              '{no:no-prefix}rt vpn import {}'),
                         ('rt_vpn_both',                                '{no:no-prefix}rt vpn both {}'),
                         ('export_vpn',                                 '{no:no-prefix}export vpn', ['true','false']),
                         ('import_vpn',                                 '{no:no-prefix}import vpn', ['true','false']),
                         ('redistribute_connected',                     '{no:no-prefix}redistribute connected', ['true','false']),
                         ('redistribute_static_rmap',                   '{no:no-prefix}redistribute static route-map {}'),
                         ('rmap_vpn_export',                            '{no:no-prefix}route-map vpn export {}'),
                         ('rmap_vpn_import',                            '{no:no-prefix}route-map vpn import {}'),
                         ('max_ebgp_paths',                             '{no:no-prefix}maximum-paths {}'),
                         (['max_ibgp_paths',
                           '+ibgp_equal_cluster_length'],               '{no:no-prefix}maximum-paths ibgp {} {:match-clust-len}', hdl_ibgp_maxpath),
                         ('route_download_filter',                      '{no:no-prefix}table-map {}'),
                         (['route_flap_dampen',
                           '+route_flap_dampen_half_life',
                           '+route_flap_dampen_reuse_threshold',
                           '+route_flap_dampen_suppress_threshold',
                           '+route_flap_dampen_max_suppress'],          '{no:no-prefix}bgp dampening {} {} {} {}', ['true', 'false']),
                         ('advertise-all-vni',                      '{no:no-prefix}advertise-all-vni', ['true','false']),
                         ('advertise-svi-ip',                       '{no:no-prefix}advertise-svi-ip', ['true','false']),
                         ('advertise-default-gw',                   '{no:no-prefix}advertise-default-gw', ['true','false']),
                         ('advertise-ipv4-unicast',                      '{no:no-prefix}advertise ipv4 unicast', ['true','false']),
                         ('advertise-ipv6-unicast',                      '{no:no-prefix}advertise ipv6 unicast', ['true','false']),
                         ('default-originate-ipv4',                      '{no:no-prefix}default-originate ipv4', ['true','false']),
                         ('default-originate-ipv6',                      '{no:no-prefix}default-originate ipv6', ['true','false']),
                         ('autort',                                      '{no:no-prefix}autort {}', hdl_enum_conversion),
                         ('flooding',                                    '{no:no-prefix}flooding {}'),
                         ('dad-enabled',                                 '{no:no-prefix}dup-addr-detection', ['true','false']),
                         (['dad-max-moves',
                            'dad-time'],                                 '{no:no-prefix}dup-addr-detection max-moves {} time {}'),
                         ('dad-freeze',                                   '{no:no-prefix}dup-addr-detection freeze {}'),
                         ('route-distinguisher',                         '{no:no-prefix}rd {}'),
                         ('import-rts',                                  '{no:no-prefix}route-target import {}', hdl_import_list),
                         ('export-rts',                                  '{no:no-prefix}route-target export {}', hdl_export_list),
                         ('import_vrf',                                 '{no:no-prefix}import vrf {}'),
                         ('import_vrf_route_map',                       '{no:no-prefix}import vrf route-map {}')
    ]

    cmn_key_map = [('asn&peer_type',                        '{no:no-prefix}neighbor {} remote-as {}'),
                   (['local_asn', '+local_as_no_prepend',
                     '+local_as_replace_as'],               '{no:no-prefix}neighbor {} local-as {} {:no-prepend} {:replace-as}'),
                   (['admin_status', '+shutdown_message'],  '{no:no-prefix}neighbor {} shutdown {:shutdown-msg}', hdl_admin_status_shutdown_msg),
                   ('local_addr',                           '{no:no-prefix}neighbor {} update-source {}'),
                   ('name',                                 '{no:no-prefix}neighbor {} description {}'),
                   (['ebgp_multihop', '+ebgp_multihop_ttl'],'{no:no-prefix}neighbor {} ebgp-multihop {}', ['true', 'false']),
                   ('auth_password',                        '{no:no-prefix}neighbor {} password {} encrypted'),
                   (['keepalive', 'holdtime'],              '{no:no-prefix}neighbor {} timers {} {}'),
                   ('conn_retry',                           '{no:no-prefix}neighbor {} timers connect {}'),
                   ('min_adv_interval',                     '{no:no-prefix}neighbor {} advertisement-interval {}'),
                   ('passive_mode',                         '{no:no-prefix}neighbor {} passive', ['true', 'false']),
                   ('capability_ext_nexthop',               '{no:no-prefix}neighbor {} capability extended-nexthop', ['true', 'false']),
                   ('disable_ebgp_connected_route_check',   '{no:no-prefix}neighbor {} disable-connected-check', ['true', 'false']),
                   ('enforce_first_as',                     '{no:no-prefix}neighbor {} enforce-first-as', ['true', 'false']),
                   ('solo_peer',                            '{no:no-prefix}neighbor {} solo', ['true', 'false']),
                   ('ttl_security_hops',                    '{no:no-prefix}neighbor {} ttl-security hops {}'),
                   ('bfd',                                  '{no:no-prefix}neighbor {} bfd', ['true', 'false']),
                   ('bfd_check_ctrl_plane_failure',         '{no:no-prefix}neighbor {} bfd check-control-plane-failure', ['true', 'false']),
                   ('capability_dynamic',                   '{no:no-prefix}neighbor {} capability dynamic', ['true', 'false']),
                   ('dont_negotiate_capability',            '{no:no-prefix}neighbor {} dont-capability-negotiate', ['true', 'false']),
                   ('enforce_multihop',                     '{no:no-prefix}neighbor {} enforce-multihop', ['true', 'false']),
                   ('override_capability',                  '{no:no-prefix}neighbor {} override-capability', ['true', 'false']),
                   ('peer_port',                            '{no:no-prefix}neighbor {} port {}'),
                   ('strict_capability_match',              '{no:no-prefix}neighbor {} strict-capability-match', ['true', 'false'])
    ]

    nbr_key_map = [('peer_group_name',  '{no:no-prefix}neighbor {} peer-group {}')]

    nbr_af_key_map = [(['allow_as_in', '+allow_as_count&allow_as_origin'],  '{no:no-prefix}neighbor {} allowas-in {:allow-as-in}', ['true', 'false']),
                      ('admin_status|ipv4',                                 '{no:no-prefix}neighbor {} activate', hdl_admin_status),
                      ('admin_status|ipv6',                                 '{no:no-prefix}neighbor {} activate', hdl_admin_status),
                      ('admin_status|l2vpn',                                '{no:no-prefix}neighbor {} activate', hdl_admin_status),
                      (['send_default_route', '+default_rmap'],             '{no:no-prefix}neighbor {} default-originate {:default-rmap}', ['true', 'false']),
                      ('default_rmap',                                      '{no:no-prefix}neighbor {} default-originate route-map {}'),
                      (['max_prefix_limit', '++max_prefix_warning_threshold',
                        '+max_prefix_restart_interval&max_prefix_warning_only'], '{no:no-prefix}neighbor {} maximum-prefix {} {} {:restart}'),
                      ('route_map_in',                                      '{no:no-prefix}neighbor {} route-map {} in'),
                      ('route_map_out',                                     '{no:no-prefix}neighbor {} route-map {} out'),
                      ('soft_reconfiguration_in',                           '{no:no-prefix}neighbor {} soft-reconfiguration inbound', ['true', 'false']),
                      ('unsuppress_map_name',                               '{no:no-prefix}neighbor {} unsuppress-map {}'),
                      ('rrclient',                                          '{no:no-prefix}neighbor {} route-reflector-client', ['true', 'false']),
                      ('weight',                                            '{no:no-prefix}neighbor {} weight {}'),
                      ('as_override',                                       '{no:no-prefix}neighbor {} as-override', ['true', 'false']),
                      ('send_community',                                    '{no:no-prefix}neighbor {} send-community {}', hdl_send_com),
                      ('tx_add_paths',                                      '{no:no-prefix}neighbor {} {:tx-add-paths}'),
                      (['++unchanged_as_path',
                        '++unchanged_med', '++unchanged_nexthop'],          '{no:no-prefix}neighbor {} attribute-unchanged {:uchg-as-path} {:uchg-med} {:uchg-nh}', hdl_attr_unchanged),
                      ('filter_list_in',                                    '{no:no-prefix}neighbor {} filter-list {} in'),
                      ('filter_list_out',                                   '{no:no-prefix}neighbor {} filter-list {} out'),
                      ('nhself',                                            '{no:no-prefix}neighbor {} next-hop-self', ['true', 'false']),
                      ('nexthop_self_force',                                '{no:no-prefix}neighbor {} next-hop-self force', ['true', 'false']),
                      ('prefix_list_in',                                    '{no:no-prefix}neighbor {} prefix-list {} in'),
                      ('prefix_list_out',                                   '{no:no-prefix}neighbor {} prefix-list {} out'),
                      (['remove_private_as_enabled',
                        '++remove_private_as_all',
                        '+replace_private_as'],                             '{no:no-prefix}neighbor {} remove-private-AS {:rm-as-all} {:rm-as-repl}', hdl_rm_priv_as, ['true', 'false']),
                      ('cap_orf',                                           '{no:no-prefix}neighbor {} capability orf prefix-list {}', hdl_capa_orf_pfxlist),
                      ('route_server_client',                               '{no:no-prefix}neighbor {} route-server-client', ['true', 'false']),
    ]

    route_map_key_map = [('match_interface',                '{no:no-prefix}match interface {}'),
                         ('match_prefix_set|ipv4',          '{no:no-prefix}match ip address prefix-list {}'),
                         ('match_prefix_set|ipv6',          '{no:no-prefix}match ipv6 address prefix-list {}'),
                         ('match_neighbor',                 '[bgpd]{no:no-prefix}match peer {:peer-ip}'),
                         ('match_tag',                      '{no:no-prefix}match tag {}'),
                         ('match_protocol',                 '[zebra]{no:no-prefix}match source-protocol {:src-proto}'),
                         ('match_next_hop_set|ipv4',        '{no:no-prefix}match ip next-hop prefix-list {}'),
                         ('match_next_hop_set|ipv6',        '{no:no-prefix}match ip next-hop prefix-list {}'), #match ipv6 next-hop prefix-list not suppported by frr
                         ('match_med',                      '{no:no-prefix}match metric {}'),
                         ('match_origin',                   '[bgpd]{no:no-prefix}match origin {:tolower}'),
                         ('match_local_pref',               '[bgpd]{no:no-prefix}match local-preference {}'),
                         ('match_community',                '[bgpd]{no:no-prefix}match community {}'),
                         ('match_ext_community',            '[bgpd]{no:no-prefix}match extcommunity {}'),
                         ('match_as_path',                  '[bgpd]{no:no-prefix}match as-path {}'),
                         ('match_src_vrf',                  '[bgpd]{no:no-prefix}match source-vrf {}'),
                         ('call_route_map',                 '{no:no-prefix}call {:enable-only}'),
                         ('set_origin',                     '[bgpd]{no:no-prefix}set origin {:tolower}'),
                         ('set_local_pref',                 '[bgpd]{no:no-prefix}set local-preference {}'),
                         ('set_next_hop',                   '{no:no-prefix}set ip next-hop {}'),
                         ('set_ipv6_next_hop_global',       '[bgpd]{no:no-prefix}set ipv6 next-hop global {}'),
                         ('set_ipv6_next_hop_prefer_global', '[bgpd]{no:no-prefix}set ipv6 next-hop prefer-global', ['true', 'false']),
                         (['set_metric_action', '+set_metric', '+set_med'], '{}set metric {} ', handle_rmap_set_metric),
                         ('set_med',                        '{no:no-prefix}set metric {}'),
                         (('set_asn', '+set_repeat_asn'),   '[bgpd]{no:no-prefix}set as-path prepend {:repeat}', hdl_set_asn),
                         ('set_asn_list',                   '[bgpd]{no:no-prefix}set as-path prepend {:asn_list}', hdl_set_asn_list),
                         ('set_community_inline',           '[bgpd]{no:no-prefix}set community {}'),
                         ('set_community_ref',              '[bgpd]{no:no-prefix}set community {:com-ref}'),
                         ('set_ext_community_inline',       '[bgpd]{no:no-prefix}set extcommunity {:ext-com-list}', hdl_set_extcomm, True),
                         ('set_ext_community_ref',          '[bgpd]{no:no-prefix}set extcommunity {:ext-com-ref}', hdl_set_extcomm, False)
    ]

    bfd_peer_shop_key_map = [('enabled',                        '{no:no-prefix}shutdown', ['false', 'true']),
                         ('desired-minimum-tx-interval',        '{no:no-prefix}transmit-interval {}'),
                         ('required-minimum-receive',           '{no:no-prefix}receive-interval {}'),
                         ('desired-minimum-echo-receive',       '{no:no-prefix}echo-interval {}'),
                         ('detection-multiplier',               '{no:no-prefix}detect-multiplier {}'),
                         ('echo-active',                        '{no:no-prefix}echo-mode', ['true', 'false'])
    ]

    bfd_peer_mhop_key_map = [('enabled',                        '{no:no-prefix}shutdown', ['false', 'true']),
                         ('desired-minimum-tx-interval',        '{no:no-prefix}transmit-interval {}'),
                         ('required-minimum-receive',           '{no:no-prefix}receive-interval {}'),
                         ('detection-multiplier',               '{no:no-prefix}detect-multiplier {}'),
    ]

    listen_prefix_key_map = [('peer_group', '{no:no-prefix}bgp listen range {} peer-group {}')]

    community_set_key_map = [(('set_type', 'match_action', 'community_member'), '{no:no-prefix}bgp community-list {}', hdl_com_set, False)]
    extcommunity_set_key_map = [(('set_type', 'match_action', 'community_member'), '{no:no-prefix}bgp extcommunity-list {}', hdl_com_set, True)]

    aspath_set_key_map = [('as_path_set_member', '{no:no-prefix}bgp as-path access-list {}', hdl_aspath_set)]

    route_redist_key_map = [(['protocol', '++metric', '+route_map'],
                             '{no:no-prefix}redistribute {} {:redist-metric} {:redist-route-map}', hdl_route_redist_set)]

    af_aggregate_key_map = [(['ip_prefix', '++as_set', '++summary_only', '+policy'],
                             '{no:no-prefix}aggregate-address {2} {3:aggr-as-set} {4:aggr-summary-only} {5:aggr-policy}', hdl_af_aggregate)]

    af_network_key_map = [(['ip_prefix', '++policy', '+backdoor'], '{no:no-prefix}network {2} {3:network-policy} {4:network-backdoor}')]

    global_evpn_vni_key_map = [('advertise-default-gw',                   '{no:no-prefix}advertise-default-gw', ['true','false']),
                               ('route-distinguisher',                         '{no:no-prefix}rd {}'),
                               ('import-rts',                                  '{no:no-prefix}route-target import {}', hdl_import_list),
                               ('export-rts',                                  '{no:no-prefix}route-target export {}', hdl_export_list)]

    ospfv2_global_key_map = [('enable',                        '{no:no-prefix}'),
                             ('auto-cost-reference-bandwidth', '{no:no-prefix}auto-cost reference-bandwidth {}'),
                             ('ospf-rfc1583-compatible',       '{no:no-prefix}compatible rfc1583', ['true', 'false']),
                             ('max-metric-administrative',     '{no:no-prefix}max-metric router-lsa administrative', ['true', 'false']),
                             ('max-metric-on-shutdown',        '{no:no-prefix}max-metric router-lsa on-shutdown {}'),
                             ('max-metric-on-startup',         '{no:no-prefix}max-metric router-lsa on-startup {}'),
                             ('router-id',                     '{no:no-prefix}ospf router-id {}'),
                             ('abr-type',                      '{}ospf abr-type {:abrtype}', handle_ospf_abrtype),
                             ('write-multiplier',              '{no:no-prefix}write-multiplier {}'),
                             ('passive-interface-default',     '{no:no-prefix}passive-interface default', ['true', 'false']),
                             ('lsa-refresh-timer',             '{no:no-prefix}refresh timer {}'),
                             ('lsa-min-arrival-timer',         '{no:no-prefix}timers lsa min-arrival {}'),
                             ('lsa-min-interval-timer',        '{no:no-prefix}timers throttle lsa all {}'),
                             (['spf-initial-delay', 'spf-maximum-delay', 'spf-throttle-delay'], '{no:no-prefix}timers throttle spf {} {} {}'),
                             ('log-adjacency-changes',         '{}log-adjacency-changes {}', hdl_ospf_log),
                             ('default-metric',                '{no:no-prefix}default-metric {}'),
                             ('distance-all',                  '{no:no-prefix}distance {}'),
                             ('distance-external',             '{no:no-prefix}distance ospf external {}'),
                             ('distance-inter-area',           '{no:no-prefix}distance ospf inter-area {}'),
                             ('distance-intra-area',           '{no:no-prefix}distance ospf intra-area {}')]

    ospfv2_area_key_map = [('stub',                     '{no:no-prefix}area {} stub'),
                           ('stub-no-summary',          '{no:no-prefix}area {} stub no-summary'),
                           ('import-list',               '{no:no-prefix}area {} import-list {}'),
                           ('export-list',               '{no:no-prefix}area {} export-list {}'),
                           ('filter-list-in',            '{no:no-prefix}area {} filter-list prefix {} in'),
                           ('filter-list-out',           '{no:no-prefix}area {} filter-list prefix {} out'),
                           ('authentication',            '{}area {} authentication {}', handle_ospf_area_auth),
                           ('stub-default-cost',         '{no:no-prefix}area {} default-cost {}'),
                           ('shortcut',                  '{}area {} shortcut {}', handle_ospf_area_shortcut)]

    ospfv2_area_vlink_key_map = [('enable',                  '{no:no-prefix}area {} virtual-link {}'),
                                 ('dead-interval',           '{no:no-prefix}area {} virtual-link {} dead-interval {}'),
                                 ('hello-interval',          '{no:no-prefix}area {} virtual-link {} hello-interval {}'),
                                 ('retransmission-interval', '{no:no-prefix}area {} virtual-link {} retransmit-interval {}'),
                                 ('transmit-delay',          '{no:no-prefix}area {} virtual-link {} transmit-delay {}'),
                                 ('authentication-type',     '{}area {} virtual-link {} authentication {}', handle_ospf_area_vlink_auth),
                                 ('authentication-key',      '{no:no-prefix}area {} virtual-link {} authentication-key {}'),
                                 (['authentication-key-id', 'authentication-md5-key'],      '{no:no-prefix}area {} virtual-link {} authentication message-digest message-digest-key {} md5 {}')]

    ospfv2_area_range_key_map =[('advertise',                '{} area {} range {} {}', handle_ospf_area_range_advt),
                                ('metric',                   '{no:no-prefix} area {} range {} cost {}'),
                                ('substitue-prefix',         '{no:no-prefix} area {} range {} substitute {}')]

    ospfv2_distribution_key_map = [('route-map|BGP|IMPORT', '{no:no-prefix}distribute-list {} out bgp'),
                                   ('route-map|STATIC|IMPORT', '{no:no-prefix}distribute-list {} out static')]

    ospfv2_interface_key_map = [
                             ('area-id', '{}ip ospf area {} {}', handle_ospf_if_common),
                             ('authentication-type', '{}ip ospf authentication {} {}', handle_ospf_if_authtype),
                             ('authentication-key', '{}ip ospf authentication-key {} {}', handle_ospf_if_common),
                             ('bfd-enable', '{no:no-prefix}ip ospf bfd ', ['true', 'false']),
                             ('metric', '{}ip ospf cost {} {}', handle_ospf_if_common),
                             ('dead-interval', '{}ip ospf dead-interval {} {}', handle_ospf_if_common),
                             ('hello-multiplier', '{}ip ospf dead-interval minimal hello-multiplier {} {}', handle_ospf_if_common),
                             ('hello-interval', '{}ip ospf hello-interval {} {}', handle_ospf_if_common),
                             (['authentication-key-id', '+authentication-md5-key'], '{}ip ospf message-digest-key {} md5 {} {}', handle_ospf_if_md5key),
                             ('mtu-ignore', '{}ip ospf mtu-ignore {}', handle_ospf_if_mtu_ignore),
                             ('network-type', '{}ip ospf network {}', handle_ospf_if_nwtype),
                             ('priority', '{}ip ospf priority {} {}', handle_ospf_if_common),
                             ('retransmission-interval', '{}ip ospf retransmit-interval {} {}', handle_ospf_if_common),
                             ('transmit-delay', '{}ip ospf transmit-delay {} {}', handle_ospf_if_common),
                           ]
    static_route_map = [(['ip_prefix|ipv4', '++blackhole', '++nexthop', '++ifname', '++track', '++tag', '++distance', '++nexthop-vrf'],
                         '{no:no-prefix}ip route {} {:blackhole} {} {} {:track} {:nh-tag} {} {:nh-vrf}', hdl_static_route, socket.AF_INET),
                        (['ip_prefix|ipv6', '++blackhole', '++nexthop', '++ifname', '++track', '++tag', '++distance', '++nexthop-vrf'],
                         '{no:no-prefix}ipv6 route {} {:blackhole} {} {} {:track} {:nh-tag} {} {:nh-vrf} ', hdl_static_route, socket.AF_INET6)]
    pim_interface_key_map = [('mode', '{no:no-prefix}ip pim', ['sm','']),
                             ('dr-priority', '{no:no-prefix}ip pim drpriority {}'),
                             ('hello-interval', '{no:no-prefix}ip pim hello {:pim_hello_parms}',
                              hdl_set_pim_hello_parms),
                             ('bfd-enabled', '{no:no-prefix}ip pim bfd', ['true', 'false']),
            ]
    pim_global_key_map =    [('join-prune-interval', '{no:no-prefix}ip pim join-prune-interval {}'),
                             ('keep-alive-timer', '{no:no-prefix}ip pim keep-alive-timer {}'),
                             ('ssm-ranges', '{no:no-prefix}ip pim ssm prefix-list {}'),
                             ('ecmp-enabled', '{no:no-prefix}ip pim ecmp', ['true', 'false']),
                             ('ecmp-rebalance-enabled', '{no:no-prefix}ip pim ecmp rebalance',['true', 'false']),
            ]

    igmp_mcast_grp_key_map =[('enable', '{no:no-prefix}ip igmp join {} {}'),
                           ]

    igmp_interface_config_key_map = [
                             ('enabled', 'ip igmp {}', handle_igmp_if_enable),
                             ('version', '{no:no-prefix}ip igmp version {}'),
                             ('query-interval', 'ip igmp query-interval {}', handle_igmp_if_common),
                             ('query-max-response-time', 'ip igmp query-max-response-time {}', handle_igmp_if_common),
                             ('last-member-query-count', 'ip igmp last-member-query-count {}', handle_igmp_if_common),
                             ('last-member-query-interval', 'ip igmp last-member-query-interval {}', handle_igmp_if_common),
                           ]
    ip_sla_key_map = [
                             ('sla_id', '{no:no-prefix}ip sla {}'),
                             ('frequency', 'frequency {}', handle_ip_sla_common),
                             ('threshold', 'threshold {}', handle_ip_sla_common),
                             ('timeout', 'timeout {}', handle_ip_sla_common),
                             ('tcp_source_port', 'source-port {}', handle_ip_sla_common),
                             ('tcp_source_ip', 'source-address {}', handle_ip_sla_common),
                             ('tcp_dst_ip', 'tcp-connect {} port {}', handle_ip_sla_tcp_connect),
                             ('tcp_vrf', 'source-vrf {}', handle_ip_sla_common),
                             ('tcp_dst_port', 'tcp-connect {} port {}', handle_ip_sla_tcp_connect),
                             ('tcp_source_interface', 'source-interface {}', handle_ip_sla_common),
                             ('tcp_ttl', 'ttl {}', handle_ip_sla_common),
                             ('tcp_tos', 'tos {}', handle_ip_sla_common),
                             ('icmp_source_interface', 'source-interface {}', handle_ip_sla_common),
                             ('icmp_source_ip', 'source-address {}', handle_ip_sla_common),
                             ('icmp_dst_ip', 'icmp-echo {}', handle_ip_sla_icmp_echo),
                             ('icmp_vrf', 'source-vrf {}', handle_ip_sla_common),
                             ('icmp_size', 'request-data-size {}', handle_ip_sla_common),
                             ('icmo_ttl', 'ttl {}', handle_ip_sla_common),
                             ('icmp_tos', 'tos {}', handle_ip_sla_common),
                           ]


    tbl_to_key_map = {'BGP_GLOBALS':                    global_key_map,
                      'BGP_GLOBALS_AF':                 global_af_key_map,
                      'BGP_GLOBALS_LISTEN_PREFIX':      listen_prefix_key_map,
                      'BGP_NEIGHBOR':                   cmn_key_map[0:2] + nbr_key_map + cmn_key_map[2:],
                      'BGP_PEER_GROUP':                 cmn_key_map,
                      'BGP_NEIGHBOR_AF':                nbr_af_key_map,
                      'BGP_PEER_GROUP_AF':              nbr_af_key_map,
                      'ROUTE_MAP':                      route_map_key_map,
                      'COMMUNITY_SET':                  community_set_key_map,
                      'EXTENDED_COMMUNITY_SET':         extcommunity_set_key_map,
                      'AS_PATH_SET':                    aspath_set_key_map,
                      'ROUTE_REDISTRIBUTE':             route_redist_key_map,
                      'BGP_GLOBALS_AF_AGGREGATE_ADDR':  af_aggregate_key_map,
                      'BGP_GLOBALS_AF_NETWORK':         af_network_key_map,
                      'BGP_GLOBALS_EVPN_VNI':           global_evpn_vni_key_map,
                      'BFD_PEER_SINGLE_HOP':            bfd_peer_shop_key_map,
                      'BFD_PEER_MULTI_HOP':             bfd_peer_mhop_key_map,
                      'IP_SLA':                         ip_sla_key_map,
                      'OSPFV2_ROUTER':                  ospfv2_global_key_map,
                      'OSPFV2_ROUTER_AREA':             ospfv2_area_key_map,
                      'OSPFV2_ROUTER_AREA_VIRTUAL_LINK':ospfv2_area_vlink_key_map,
                      'OSPFV2_ROUTER_AREA_POLICY_ADDRESS_RANGE':ospfv2_area_range_key_map,
                      'OSPFV2_INTERFACE':               ospfv2_interface_key_map,
                      'STATIC_ROUTE':                   static_route_map,
                      'PIM_GLOBALS':                    pim_global_key_map,
                      'PIM_INTERFACE':                  pim_interface_key_map,
                      'IGMP_INTERFACE':                 igmp_mcast_grp_key_map,
                      'IGMP_INTERFACE_QUERY':           igmp_interface_config_key_map,
    }

    vrf_tables = {'BGP_GLOBALS', 'BGP_GLOBALS_AF',
                  'BGP_NEIGHBOR', 'BGP_PEER_GROUP', 'BGP_NEIGHBOR_AF', 'BGP_PEER_GROUP_AF',
                  'BGP_GLOBALS_LISTEN_PREFIX', 'ROUTE_REDISTRIBUTE',
                  'BGP_GLOBALS_AF_AGGREGATE_ADDR', 'BGP_GLOBALS_AF_NETWORK',
                  'BGP_GLOBALS_EVPN_RT', 'BGP_GLOBALS_EVPN_VNI', 'BGP_GLOBALS_EVPN_VNI_RT'}

    @staticmethod
    def __peer_is_ip(peer):
        try:
            socket.inet_pton(socket.AF_INET, peer)
            return True
        except socket.error:
            pass
        try:
            socket.inet_pton(socket.AF_INET6, peer)
            return True
        except socket.error:
            pass
        return False

    def __init__(self):
        self.config_db = ExtConfigDBConnector({'STATIC_ROUTE': {'nexthop', 'ifname', 'distance', 'nexthop-vrf', 'blackhole', 'track'}})
        try:
            self.config_db.connect()
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, '[bgp cfgd] Failed connecting to config DB with exception:' + str(e))
        db_entry = self.config_db.get_entry('DEVICE_METADATA', 'localhost')
        if 'bgp_asn' in db_entry:
            self.metadata_asn = db_entry['bgp_asn']
        else:
            self.metadata_asn = None
        if 'docker_routing_config_mode' in db_entry:
            self.config_mode = db_entry['docker_routing_config_mode']
        else:
            self.config_mode = "separated"
        # VRF ==> local_as
        self.bgp_asn = {}
        # VRF ==> confederation peer list
        self.bgp_confed_peers = {}
        glb_table = self.config_db.get_table('BGP_GLOBALS')
        for vrf, entry in glb_table.items():
            if 'local_asn' in entry:
                self.bgp_asn[vrf] = entry['local_asn']
                syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: VRF %s Local_ASN %s' % (vrf, self.bgp_asn[vrf]))
            if 'confed_peers' in entry:
                self.bgp_confed_peers[vrf] = set(entry['confed_peers'])
        # VRF ==> grp_name ==> peer_group
        self.bgp_peer_group = {}
        # VRF ==> set of interface neighbor
        self.bgp_intf_nbr = {}
        nbr_table = self.config_db.get_table('BGP_NEIGHBOR')
        pg_table = self.config_db.get_table('BGP_PEER_GROUP')
        for key, entry in pg_table.items():
            vrf, pg = key
            self.bgp_peer_group.setdefault(vrf, {})[pg] = BGPPeerGroup(vrf)
            syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: VRF %s Peer_Group %s' % (vrf, pg))
        for key, entry in nbr_table.items():
            if len(key) != 2:
                continue
            vrf, peer = key
            if 'peer_group_name' in entry:
                pg_name = entry['peer_group_name']
                if vrf in self.bgp_peer_group and pg_name in self.bgp_peer_group[vrf]:
                    self.bgp_peer_group[vrf][pg_name].ref_nbrs.add(peer)
                    syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: VRF %s Neighbor %s Peer_Group %s' %
                            (vrf, peer, pg_name))
            if not self.__peer_is_ip(peer):
                self.bgp_intf_nbr.setdefault(vrf, set()).add(peer)
        # map_name ==> seq_no ==> operation
        self.route_map = {}
        rtmap_table = self.config_db.get_table('ROUTE_MAP')
        for key, entry in rtmap_table.items():
            rtmap_name, seq_no = key
            syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: Route_Map %s Seq_NO %s' % (rtmap_name, seq_no))
            if 'route_operation' in entry:
                self.route_map.setdefault(rtmap_name, {})[seq_no] = entry['route_operation']

        self.comm_set_list = {}
        comm_table = self.config_db.get_table('COMMUNITY_SET')
        for key, entry in comm_table.items():
            syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: Community %s' % key)
            self.comm_set_list[key] = CommunityList(key, False)
            for k, v in entry.items():
                self.comm_set_list[key].db_data_to_attr(k, v)
        self.extcomm_set_list = {}
        extcomm_table = self.config_db.get_table('EXTENDED_COMMUNITY_SET')
        for key, entry in extcomm_table.items():
            syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: Extended_Community %s' % key)
            self.extcomm_set_list[key] = CommunityList(key, True)
            for k, v in entry.items():
                self.extcomm_set_list[key].db_data_to_attr(k, v)
        self.prefix_set_list = {}
        pfx_set_table = self.config_db.get_table('PREFIX_SET')
        for key, entry in pfx_set_table.items():
            if 'mode' in entry:
                syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: Prefix_Set %s mode %s' % (key, entry['mode']))
                self.prefix_set_list[key] = MatchPrefixList(entry['mode'].lower())
        pfx_table = self.config_db.get_table('PREFIX')
        for key, entry in pfx_table.items():
            pfx_set_name, ip_pfx, len_range = key
            syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: Prefix %s range %s of set %s' % (ip_pfx, len_range, pfx_set_name))
            if len_range == 'exact':
                len_range = None
            if pfx_set_name in self.prefix_set_list and 'action' in entry:
                try:
                    self.prefix_set_list[pfx_set_name].add_prefix(ip_pfx, len_range, entry['action'])
                except ValueError:
                    pass
        self.as_path_set_list = {}
        aspath_table = self.config_db.get_table('AS_PATH_SET')
        for key, entry in aspath_table.items():
            if 'as_path_set_member' in entry:
                syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: AS_Path_Set %s member %s' % (key, entry['as_path_set_member']))
                self.as_path_set_list[key] = entry['as_path_set_member'][:]
        self.tag_set_list = {}

        self.af_aggr_list = {}
        af_aggr_table = self.config_db.get_table('BGP_GLOBALS_AF_AGGREGATE_ADDR')
        for key, entry in af_aggr_table.items():
            vrf, af_type, ip_pfx = key
            af, _ = af_type.lower().split('_')
            norm_ip_pfx = MatchPrefix.normalize_ip_prefix((socket.AF_INET if af == 'ipv4' else socket.AF_INET6), ip_pfx)
            if norm_ip_pfx is not None:
                syslog.syslog(syslog.LOG_DEBUG, 'Init Config DB Data: AF Aggregate Prefix %s of vrf %s AF %s' % (norm_ip_pfx, vrf, af))
                aggr_obj = AggregateAddr()
                for k, v in entry.items():
                    if v == 'true':
                        setattr(aggr_obj, k, True)
                self.af_aggr_list.setdefault(vrf, {})[norm_ip_pfx] = aggr_obj

        self.vrf_vni_map = {}
        vrf_table = self.config_db.get_table('VRF')
        for key, entry in vrf_table.items():
            if 'vni' in entry:
                self.vrf_vni_map[key] = entry['vni']

        # VRF ==> ip_prefix ==> nexthop list
        self.static_route_list = {}
        sroute_table = self.config_db.get_table('STATIC_ROUTE')
        get_list = lambda v: v.split(',') if v is not None else None
        for key, entry in sroute_table.items():
            if type(key) is tuple and len(key) == 2:
                vrf, ip_prefix = key
            else:
                vrf = self.DEFAULT_VRF
                ip_prefix = key
            af, ip_prefix = IpNextHopSet.get_af_norm_prefix(ip_prefix)
            nh_attr = lambda k: get_list(entry.get(k, None))
            self.static_route_list.setdefault(vrf, {})[ip_prefix] = IpNextHopSet(af,
                                        nh_attr('blackhole'), nh_attr('nexthop'),nh_attr('track'),
                                        nh_attr('ifname'), nh_attr('tag'), nh_attr('distance'),
                                        nh_attr('nexthop-vrf'))

        self.table_handler_list = [
            ('VRF', self.vrf_handler),
            ('DEVICE_METADATA', self.metadata_handler),
            ('BGP_GLOBALS', self.bgp_global_handler),
            ('BGP_GLOBALS_AF', self.bgp_af_handler),
            ('PREFIX_SET', self.bgp_table_handler_common),
            ('PREFIX', self.bgp_table_handler_common),
            ('COMMUNITY_SET', self.comm_set_handler),
            ('EXTENDED_COMMUNITY_SET', self.comm_set_handler),
            ('ROUTE_MAP', self.bgp_table_handler_common),
            ('BGP_PEER_GROUP', self.bgp_neighbor_handler),
            ('BGP_NEIGHBOR', self.bgp_neighbor_handler),
            ('BGP_PEER_GROUP_AF', self.bgp_table_handler_common),
            ('BGP_NEIGHBOR_AF', self.bgp_table_handler_common),
            ('BGP_GLOBALS_LISTEN_PREFIX', self.bgp_table_handler_common),
            ('BGP_GLOBALS_EVPN_VNI', self.bgp_table_handler_common),
            ('BGP_GLOBALS_EVPN_RT', self.bgp_table_handler_common),
            ('BGP_GLOBALS_EVPN_VNI_RT', self.bgp_table_handler_common),
            ('BFD_PEER', self.bfd_handler),
            ('NEIGHBOR_SET', self.bgp_table_handler_common),
            ('NEXTHOP_SET', self.bgp_table_handler_common),
            ('TAG_SET', self.bgp_table_handler_common),
            ('AS_PATH_SET', self.bgp_table_handler_common),
            ('ROUTE_REDISTRIBUTE', self.bgp_table_handler_common),
            ('BGP_GLOBALS_AF_AGGREGATE_ADDR', self.bgp_table_handler_common),
            ('BGP_GLOBALS_AF_NETWORK', self.bgp_table_handler_common),
            ('BFD_PEER_SINGLE_HOP', self.bgp_table_handler_common),
            ('BFD_PEER_MULTI_HOP', self.bgp_table_handler_common),
            ('IP_SLA', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER_AREA', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER_AREA_VIRTUAL_LINK', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER_AREA_NETWORK', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER_AREA_POLICY_ADDRESS_RANGE', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER_DISTRIBUTE_ROUTE', self.bgp_table_handler_common),
            ('OSPFV2_INTERFACE', self.bgp_table_handler_common),
            ('OSPFV2_ROUTER_PASSIVE_INTERFACE', self.bgp_table_handler_common),
            ('STATIC_ROUTE', self.bgp_table_handler_common),
            ('PIM_GLOBALS', self.bgp_table_handler_common),
            ('PIM_INTERFACE', self.bgp_table_handler_common),
            ('IGMP_INTERFACE', self.bgp_table_handler_common),
            ('IGMP_INTERFACE_QUERY', self.bgp_table_handler_common),
            ('SRV6_MY_LOCATORS', self.bgp_table_handler_common),
            ('SRV6_MY_SOURCE', self.bgp_table_handler_common),
            ('SRV6_MY_SIDS', self.bgp_table_handler_common),
        ]
        self.bgp_message = queue.Queue(0)
        self.table_data_cache = self.config_db.get_table_data([tbl for tbl, _ in self.table_handler_list])
        syslog.syslog(syslog.LOG_DEBUG, 'Init Cached DB data')
        for key, entry in self.table_data_cache.items():
            syslog.syslog(syslog.LOG_DEBUG, '  %-20s : %s' % (key, entry))
        if self.config_mode == "unified":
            for table, _ in self.table_handler_list:
                table_list = self.config_db.get_table(table)
                for key, data in table_list.items():
                    syslog.syslog(syslog.LOG_DEBUG, 'config replay for table {} key {}'.format(table, key))
                    upd_data = {}
                    for upd_key, upd_val in data.items():
                        upd_data[upd_key] = CachedDataWithOp(upd_val, CachedDataWithOp.OP_ADD)
                    self.bgp_message.put((self.config_db.serialize_key(key), False, table, upd_data))
                    upd_data_list = []
                    self.__update_bgp(upd_data_list)
                    for table1, key1, data1 in upd_data_list:
                        table_key = ExtConfigDBConnector.get_table_key(table1, key1)
                        self.__update_cache_data(table_key, data1)

    def subscribe_all(self):
        for table, hdlr in self.table_handler_list:
            self.config_db.subscribe(table, hdlr)

    @staticmethod
    def __run_command(table, command, daemons = None):
        return g_run_command(table, command, True, daemons)

    def metadata_handler(self, table, key, data):
        if key != 'localhost':
            syslog.syslog(syslog.LOG_DEBUG, 'not localhost data update')
            return
        if data is None or 'bgp_asn' not in data:
            self.metadata_asn = None
        else:
            self.metadata_asn = data['bgp_asn']

    def bfd_handler(self, table, key, data):
        syslog.syslog(syslog.LOG_INFO, '[bgp cfgd](bfd) value for {} changed to {}'.format(key, data))
        #get frr bfd session key
        key_params = key.split('|')
        cmd = 'peer {}'.format(key_params[0])
        if len(key_params) == 4 and key_params[3] == 'multihop':
            cmd = cmd + ' multihop '
        if key_params[1] != 'null':
            cmd = cmd + ' local-address ' + key_params[1]
        if key_params[2] != 'null':
            cmd = cmd + ' interface ' + key_params[2]
        if not data:
            #BFD peer is deleted
            command = "vtysh -c 'configure terminal' -c 'bfd' -c 'no {}'".format(cmd)
            self.__run_command(table, command)
        else:
            #create/update case
            command = "vtysh -c 'configure terminal' -c 'bfd' -c '{}'".format(cmd)
            for param in data:
                if param == 'transmit_interval':
                    command = command + " -c 'transmit-interval {}'".format(data[param])
                elif param == 'receive_interval':
                    command = command + " -c 'receive-interval {}'".format(data[param])
                elif param == 'multiplier':
                    command = command + " -c 'detect-multiplier {}'".format(data[param])
                elif param == 'echo_mode' and data[param] == 'true':
                    command = command + " -c 'echo-mode'"
                elif param == 'echo_interval':
                    command = command + " -c 'echo-interval {}'".format(data[param])
                elif param == 'label':
                    command = command + " -c 'label {}'".format(data[param])
                elif param == 'admin_status' and data[param] == 'up':
                    command = command + " -c 'no shutdown'"
                elif param == 'admin_status' and data[param] == 'down':
                    command = command + " -c 'shutdown'"
            self.__run_command(table, command)

    def vrf_handler(self, table, key, data):
        syslog.syslog(syslog.LOG_INFO, '[bgp cfgd](vrf) value for {} changed to {}'.format(key, data))
        #get vrf key
        key_params = key.split('|')
        cmd = 'vrf {}'.format(key_params[0])
        if not data:
            #VRF is deleted
            command = "vtysh -c 'configure terminal' -c '{}'".format(cmd)
            if key_params[0] in self.vrf_vni_map:
                command = command + " -c 'no vni {}'".format(self.vrf_vni_map[key_params[0]])
                del self.vrf_vni_map[key_params[0]]
                self.__run_command(table, command)
        else:
            #create/update case
            command = "vtysh -c 'configure terminal' -c '{}'".format(cmd)
            positive_execute = False
            for param in data:
                if param == 'vni':
                    if data[param] != '0':
                        command = command + " -c 'vni {}'".format(data[param])
                        self.vrf_vni_map[key_params[0]] = data[param]
                        positive_execute = True
                    elif key_params[0] in self.vrf_vni_map:
                        command = command + " -c 'no vni {}'".format(self.vrf_vni_map[key_params[0]])
                        del self.vrf_vni_map[key_params[0]]
                        positive_execute = True
            if positive_execute == True:
                self.__run_command(table, command)

    def __get_vrf_asn(self, vrf):
        if vrf in self.bgp_asn:
            return self.bgp_asn[vrf]
        if vrf == self.DEFAULT_VRF and self.metadata_asn is not None:
            return self.metadata_asn
        return None

    def __delete_vrf_asn(self, vrf, table, data):
        if vrf != self.DEFAULT_VRF and vrf not in self.bgp_asn:
            syslog.syslog(syslog.LOG_ERR, 'non-default VRF {} was not configured'.format(vrf))
            return False
        local_asn = self.__get_vrf_asn(vrf)
        if local_asn is None:
            syslog.syslog(syslog.LOG_ERR, 'failed to get local ASN of VRF {} for delete'.format(vrf))
            return False
        command = "vtysh -c 'configure terminal' -c 'no router bgp {}".format(local_asn)
        if vrf != self.DEFAULT_VRF:
            command += " vrf {}'".format(vrf)
        else:
            command += "'"
        if not self.__run_command(table, command):
            syslog.syslog(syslog.LOG_ERR, 'failed to delete local_asn for VRF %s' % vrf)
            return False
        if vrf in self.bgp_asn:
            del(self.bgp_asn[vrf])
        for dkey, dval in data.items():
            # force delete all VRF instance attributes in cache
            dval.status = CachedDataWithOp.STAT_SUCC
            dval.op = CachedDataWithOp.OP_DELETE
        return True

    def __cleanup_nbr_cache(self, vrf, nbr):
        nbr_key = ExtConfigDBConnector.get_table_key('BGP_NEIGHBOR',
                                            self.config_db.serialize_key((vrf, nbr)))
        self.table_data_cache.pop(nbr_key, None)
        for af in ['ipv4', 'ipv6']:
            nbr_af_key = ExtConfigDBConnector.get_table_key('BGP_NEIGHBOR_AF',
                                                self.config_db.serialize_key((vrf, nbr, af + '_unicast')))
            self.table_data_cache.pop(nbr_af_key, None)

    def __delete_vrf_neighbor(self, vrf, peer, data, is_peer_grp):
        if is_peer_grp:
            if vrf in self.bgp_peer_group and peer in self.bgp_peer_group[vrf]:
                del(self.bgp_peer_group[vrf][peer])
        else:
            if vrf in self.bgp_peer_group:
                for _, peer_grp in self.bgp_peer_group[vrf].items():
                    if peer in peer_grp.ref_nbrs:
                        peer_grp.ref_nbrs.remove(peer)
            if not self.__peer_is_ip(peer) and vrf in self.bgp_intf_nbr and peer in self.bgp_intf_nbr[vrf]:
                self.bgp_intf_nbr[vrf].remove(peer)
        self.__cleanup_nbr_cache(vrf, peer)
        for dkey, dval in data.items():
            # bypass cache update because cache entry was removed
            dval.status = CachedDataWithOp.STAT_SUCC
            dval.op = CachedDataWithOp.OP_NONE

    def __delete_pg_neighbors(self, vrf, pg_name):
        syslog.syslog(syslog.LOG_DEBUG,
                'delete all associated neighbors of peer group %s of vrf %s from cache' % (pg_name, vrf))
        if vrf in self.bgp_peer_group and pg_name in self.bgp_peer_group[vrf]:
            peer_grp = self.bgp_peer_group[vrf][pg_name]
            for nbr in peer_grp.ref_nbrs:
                if vrf in self.bgp_intf_nbr and nbr in self.bgp_intf_nbr[vrf]:
                    self.bgp_intf_nbr[vrf].remove(nbr)
                self.__cleanup_nbr_cache(vrf, nbr)
            peer_grp.ref_nbrs.clear()

    def __delete_route_map(self, map_name, seq_no, data):
        if map_name in self.route_map and seq_no in self.route_map[map_name]:
            del(self.route_map[map_name][seq_no])
        for dkey, dval in data.items():
            # force delete all neighbor attributes in cache
            dval.status = CachedDataWithOp.STAT_SUCC
            dval.op = CachedDataWithOp.OP_DELETE

    @staticmethod
    def __vrf_based_table(table_name):
        return table_name in BGPConfigDaemon.vrf_tables

    @staticmethod
    def get_prefix_set_name(orig_name, table_name):
        new_name = orig_name
        if table_name == 'NEIGHBOR_SET':
            new_name += '_neighbor'
        elif table_name == 'NEXTHOP_SET':
            new_name += '_nexthop'
        return new_name

    def __apply_dep_vrf_table(self, vrf, table_name, *table_key, **extra_args):
        if len(table_key) > 0:
            new_key = (vrf,) + table_key
            entry_list = {new_key: self.config_db.get_entry(table_name, new_key)}
        else:
            entry_list = self.config_db.get_table(table_name)
        match_fn = extra_args.get('match', None)
        for key, data in entry_list.items():
            if data is None or len(key) == 0 or key[0] != vrf:
                continue
            if match_fn is not None and not match_fn(data):
                continue
            syslog.syslog(syslog.LOG_DEBUG, 'attr re-apply for vrf {} table {} key {} data {}'.format(vrf, table_name, key, data))
            upd_data = {}
            for upd_key, upd_val in data.items():
                upd_data[upd_key] = CachedDataWithOp(upd_val, CachedDataWithOp.OP_ADD)
            self.bgp_message.put((self.config_db.serialize_key(key), False, table_name, upd_data))

    @staticmethod
    def __nbr_impl_action(data, peer, is_pg):
        if is_pg:
            chk_attrs = ['asn']
        elif BGPConfigDaemon.__peer_is_ip(peer):
            chk_attrs = ['asn', 'peer_group_name']
        else:
            chk_attrs = ['peer_group_name']
        for attr in chk_attrs:
            val = data.get(attr, None)
            if val is not None:
                if val.op == CachedDataWithOp.OP_ADD:
                    return 'apply'
                elif val.op == CachedDataWithOp.OP_DELETE:
                    return 'delete'
        return None

    def __apply_config_op_success(self, data, cfg_data={}):
        op_list = [ CachedDataWithOp.OP_NONE, CachedDataWithOp.OP_ADD,
                    CachedDataWithOp.OP_DELETE, CachedDataWithOp.OP_UPDATE ]
        if len(cfg_data) == 0:
            for dkey, dval in data.items():
                dval.status = CachedDataWithOp.STAT_SUCC
        else :
            for dkey, dval in data.items():
                if dkey in cfg_data.keys():
                    if cfg_data[dkey] not in op_list :
                        syslog.syslog(syslog.LOG_INFO, 'Invalid op {} received'.format(cfg_data[dkey]))
                        continue
                    dval.status = CachedDataWithOp.STAT_SUCC
                    dval.op = cfg_data[dkey]
        syslog.syslog(syslog.LOG_INFO, 'apply_config_op_success on {}.'.format(data))

    def __apply_config_delete_success(self, data):
        for dkey, dval in data.items():
            dval.status = CachedDataWithOp.STAT_SUCC
            dval.op = CachedDataWithOp.OP_DELETE

    def __ospf_delete (self, data):
        for dkey, dval in data.items():
            # force delete all peer attributes in cache
            dval.status = CachedDataWithOp.STAT_SUCC
            dval.op = CachedDataWithOp.OP_DELETE

    def __ospf_apply_config(self, data, rmapoper, metricoper, metrictypeoper, alwaysoper, acclistoper):
        for dkey, dval in data.items():

            if 'route-map' == dkey:
                dval.status = CachedDataWithOp.STAT_SUCC
                dval.op = rmapoper

            if 'metric' == dkey:
                dval.status = CachedDataWithOp.STAT_SUCC
                dval.op = metricoper

            if 'metric-type' == dkey:
                dval.status = CachedDataWithOp.STAT_SUCC
                dval.op = metrictypeoper

            if 'always' == dkey:
                dval.status = CachedDataWithOp.STAT_SUCC
                dval.op = alwaysoper

            if 'access-list' == dkey:
                dval.status = CachedDataWithOp.STAT_SUCC
                dval.op = acclistoper

    def __delete_bfd_peer(self, data):
        for dkey, dval in data.items():
            # force delete all peer attributes in cache
            dval.status = CachedDataWithOp.STAT_SUCC
            dval.op = CachedDataWithOp.OP_DELETE

    def __bfd_handle_delete (self, data):
        cmd_suffix = ""
        if 'desired-minimum-tx-interval' in data:
            dval = data['desired-minimum-tx-interval']
            cmd_suffix = "transmit-interval 300"
        elif 'required-minimum-receive' in data:
            dval = data['required-minimum-receive']
            cmd_suffix = "receive-interval 300"
        elif 'detection-multiplier' in data:
            dval = data['detection-multiplier']
            cmd_suffix = "detect-multiplier 3"
        elif 'desired-minimum-echo-receive' in data:
            dval = data['desired-minimum-echo-receive']
            cmd_suffix = "echo-interval 300"
        if cmd_suffix != "":
            return cmd_suffix, dval.op

        return cmd_suffix, None

    def __update_bgp(self, data_list):
        while not self.bgp_message.empty():
            key, del_table, table, data = self.bgp_message.get()
            if table == 'STATIC_ROUTE' and len(key.split('|')) == 1:
                key = self.DEFAULT_VRF + '|' + key
            key_list = key.split('|', 1)
            if table == 'BGP_NEIGHBOR' and len(key_list) == 1:
                # bypass non-compatible neighbor table
                continue
            data_list.append((table, key, data))
            if len(key_list) > 1:
                key = key_list[1]
            else:
                key = None
            prefix = key_list[0]
            syslog.syslog(syslog.LOG_INFO, 'value for table {} prefix {} key {} changed to {}'.format(table, prefix, key, data))
            if self.__vrf_based_table(table):
                vrf = prefix
                local_asn = self.__get_vrf_asn(vrf)
                if local_asn is None and (table != 'BGP_GLOBALS' or 'local_asn' not in data):
                    syslog.syslog(syslog.LOG_DEBUG, 'ignore table {} update because local_asn for VRF {} was not configured'.\
                            format(table, vrf))
                    continue
            if table in self.tbl_to_key_map:
                tbl_key = None
                if table == 'BGP_NEIGHBOR_AF' or table == 'BGP_PEER_GROUP_AF' and key is not None:
                    _, af_ip_type = key.split('|')
                    tbl_key, _ = af_ip_type.lower().split('_')
                    tbl_key = {'admin_status': tbl_key}
                elif table == 'ROUTE_MAP':
                    tbl_key = {}
                    for attr_name, table_name in {'match_prefix_set': 'PREFIX', 'match_next_hop_set': 'PREFIX'}.items():
                        if attr_name in data:
                            pfx_set_name = self.get_prefix_set_name(data[attr_name].data, table_name)
                            if pfx_set_name in self.prefix_set_list:
                                af_mode = self.prefix_set_list[pfx_set_name].af
                                tbl_key[attr_name] = 'ipv4' if af_mode == socket.AF_INET else 'ipv6'
                elif table == 'STATIC_ROUTE':
                    af_id, new_key = IpNextHopSet.get_af_norm_prefix(key)
                    if new_key is not None:
                        key = new_key
                        tbl_key = {'ip_prefix': ('ipv4' if af_id == socket.AF_INET else 'ipv6')}
                key_map = BGPKeyMapList(self.tbl_to_key_map[table], table, tbl_key)
            else:
                key_map = None
            if table == 'BGP_GLOBALS':
                if not del_table:
                    if 'local_asn' in data:
                        dval = data['local_asn']
                        if dval.op == CachedDataWithOp.OP_DELETE:
                            # delete local_asn will delete whole VRF instance
                            self.__delete_vrf_asn(vrf, table, data)
                            continue
                        prog_asn = True
                        if dval.op == CachedDataWithOp.OP_UPDATE:
                            syslog.syslog(syslog.LOG_ERR, 'local_asn could not be modified')
                            prog_asn = False
                        if dval.op == CachedDataWithOp.OP_NONE:
                            prog_asn = False
                        if prog_asn:
                            command = "vtysh -c 'configure terminal' -c 'router bgp {} vrf {}' -c 'no bgp default ipv4-unicast'".format(dval.data, vrf)
                            if self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_DEBUG, 'set local_asn %s to VRF %s, re-apply all VRF related tables' % (dval.data, vrf))
                                self.bgp_asn[vrf] = dval.data
                                self.__apply_dep_vrf_table(vrf, 'ROUTE_REDISTRIBUTE')
                                dval.status = CachedDataWithOp.STAT_SUCC
                            else:
                                syslog.syslog(syslog.LOG_ERR, 'failed to set local_asn %s to VRF %s' % (dval.data, vrf))
                        else:
                            dval.status = CachedDataWithOp.STAT_SUCC
                    if 'confed_peers' in data:
                        self.upd_confed_peers = copy.copy(self.bgp_confed_peers.get(vrf, set()))
                    local_asn = self.__get_vrf_asn(vrf)
                    if local_asn is None:
                        syslog.syslog(syslog.LOG_ERR, 'local ASN for VRF %s was not configured' % vrf)
                        continue
                    cmd_prefix = ['configure terminal', 'router bgp {} vrf {}'.format(local_asn, vrf)]
                    if 'srv6_locator' in data:
                        cmd =  "vtysh -c 'configure terminal' "
                        cmd += " -c 'router bgp {} vrf {}' ".format(local_asn, vrf)
                        cmd += " -c 'segment-routing srv6' "
                        cmd += " -c 'locator {}' ".format(data['srv6_locator'].data)
                        if not self.__run_command(table, cmd):
                            syslog.syslog(syslog.LOG_ERR, 'failed running SRV6 POLICY config command')
                            continue
                    if not key_map.run_command(self, table, data, cmd_prefix):
                        syslog.syslog(syslog.LOG_ERR, 'failed running BGP global config command')
                        continue
                    if 'confed_peers' in data:
                        self.bgp_confed_peers[vrf] = copy.copy(self.upd_confed_peers)
                else:
                    self.__delete_vrf_asn(vrf, table, data)
            elif table == 'SRV6_MY_LOCATORS':
                if key is None:
                    syslog.syslog(syslog.LOG_ERR, 'invalid key for SRV6_MY_LOCATORS table')
                    continue
                if not del_table:
                    key = prefix
                    prefix = data['prefix']
                    cmd =  "vtysh -c 'configure terminal' -c 'segment-routing' -c 'srv6' -c 'locators' "
                    cmd += " -c 'locator {}' ".format(key)
                    cmd += " -c 'prefix {} block-len {} node-len {} func-bits {}' ".format(prefix.data, data['block_len'].data, data['node_len'].data, data['func_len'].data)
                    if not self.__run_command(table, cmd):
                        syslog.syslog(syslog.LOG_ERR, 'failed running SRV6 POLICY config command')
                        continue
            elif table == 'SRV6_MY_SOURCE':
                source = data['source-address']
                cmd =  "vtysh -c 'configure terminal' -c 'segment-routing' -c 'srv6' -c 'encapsulation' "
                cmd += " -c 'source-address {}' ".format(source.data)
                if not self.__run_command(table, cmd):
                    syslog.syslog(syslog.LOG_ERR, 'failed running SRV6 encap config command {}'.format(cmd))
                    continue
            elif table == 'SRV6_MY_SIDS':
                if key is None:
                    syslog.syslog(syslog.LOG_ERR, 'invalid key for SRV6_MY_SIDS table')
                    continue
                if not del_table:
                    cmd = "vtysh -c 'configure terminal' -c 'segment-routing' -c 'srv6' "
                    cmd +="-c 'static-sids' "
                    uDTAction = ["uDT46", "uDT4", "uDT6"]
                    if data['action'].data in uDTAction:
                        cmd +="-c 'sid {} locator {} behavior {} vrf {}' ".format(key, prefix, data['action'].data, data['decap_vrf'].data)
                    elif data['action'].data == 'uN':
                        cmd +="-c 'sid {} locator {} behavior {} ' ".format(key, prefix, data['action'].data)
                    else:
                        syslog.syslog(syslog.LOG_ERR, 'failed running SRV6 POLICY config command, not support action %s'.format(data['action'].data))
                        continue
                    if not self.__run_command(table, cmd):
                        syslog.syslog(syslog.LOG_ERR, 'failed running SRV6 SRV6_MY_SIDS config command')
                        continue

            elif table == 'BGP_GLOBALS_AF':
                af, ip_type = key.lower().split('_')
                #this is to temporarily make table cache key accessible to key_map handler function
                self.tmp_cache_key = 'BGP_GLOBALS_AF&&{}|{}'.format(vrf, key.lower())
                syslog.syslog(syslog.LOG_INFO, 'Set address family global to {} {} cache-key to {}'.format(af, ip_type, self.tmp_cache_key))
                cmd_prefix = ['configure terminal',
                              'router bgp {} vrf {}'.format(local_asn, vrf),
                              'address-family {} {}'.format(af, ip_type)]
                if not key_map.run_command(self, table, data, cmd_prefix):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP global AF config command')
                    continue
                self.tmp_cache_key = ''
            elif table == 'BGP_GLOBALS_LISTEN_PREFIX':
                syslog.syslog(syslog.LOG_INFO, 'Set BGP listen prefix {}'.format(key))
                cmd_prefix = ['configure terminal',
                              'router bgp {} vrf {}'.format(local_asn, vrf)]
                if not key_map.run_command(self, table, data, cmd_prefix, key):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP global listen prefix config command')
                    continue
            elif table == 'BGP_NEIGHBOR' or table == 'BGP_PEER_GROUP':
                is_peer_group = table == 'BGP_PEER_GROUP'
                if not del_table:
                    if is_peer_group:
                        # if peer group is not created, create it before setting other attributes
                        if key not in self.bgp_peer_group.setdefault(vrf, {}):
                            command = "vtysh -c 'configure terminal' -c 'router bgp {} vrf {}' ".format(local_asn, vrf)
                            command += "-c 'neighbor {} peer-group'".format(key)
                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to create peer-group %s for VRF %s' % (key, vrf))
                                continue
                            self.bgp_peer_group[vrf][key] = BGPPeerGroup(vrf)
                    elif not self.__peer_is_ip(key):
                        if key not in self.bgp_intf_nbr.setdefault(vrf, set()):
                            command = "vtysh -c 'configure terminal' -c 'router bgp {} vrf {}' ".format(local_asn, vrf)
                            command += "-c 'neighbor {} interface'".format(key)
                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to create neighbor of interface %s for VRF %s' % (key, vrf))
                                continue
                            self.bgp_intf_nbr[vrf].add(key)
                    bfd_val = data.get('bfd', None)
                    if (bfd_val is not None and (bfd_val.op == CachedDataWithOp.OP_ADD or bfd_val.op == CachedDataWithOp.OP_UPDATE) and
                        bfd_val.data == 'true'):
                        cp_chk_val = data.get('bfd_check_ctrl_plane_failure', None)
                        if cp_chk_val is not None and cp_chk_val.op == CachedDataWithOp.OP_NONE and cp_chk_val.data == 'true':
                            cp_chk_val.op = CachedDataWithOp.OP_ADD
                    cmd_prefix = ['configure terminal', 'router bgp {} vrf {}'.format(local_asn, vrf)]
                    if not key_map.run_command(self, table, data, cmd_prefix, key):
                        syslog.syslog(syslog.LOG_ERR, 'failed running BGP neighbor config command')
                        continue
                    if ('peer_group_name' in data and
                        (data['peer_group_name'].op == CachedDataWithOp.OP_ADD or
                         data['peer_group_name'].op == CachedDataWithOp.OP_DELETE)):
                        dval = data['peer_group_name']
                        if vrf not in self.bgp_peer_group or dval.data not in self.bgp_peer_group[vrf]:
                            # should not happen because vtysh command will fail if peer_group not exists
                            syslog.syslog(syslog.LOG_ERR, 'invalid peer-group %s was referenced' % dval.data)
                            continue
                        peer_grp = self.bgp_peer_group[vrf][dval.data]
                        if dval.op == CachedDataWithOp.OP_ADD:
                            peer_grp.ref_nbrs.add(key)
                        else:
                            peer_grp.ref_nbrs.discard(key)
                    nbr_action = self.__nbr_impl_action(data, key, is_peer_group)
                    if nbr_action == 'delete':
                        if not is_peer_group and self.__peer_is_ip(key):
                            # delete asn or peer_group will delete all neighbor
                            self.__delete_vrf_neighbor(vrf, key, data, False)
                        elif is_peer_group:
                            # clear associated neighbor list in cache
                            self.__delete_pg_neighbors(vrf, key)
                    elif nbr_action == 'apply':
                        if is_peer_group:
                            syslog.syslog(syslog.LOG_DEBUG, 'apply attributes to FRR for vrf %s peer_group %s' % (vrf, key))
                            match_pg = lambda data: data.get('peer_group', None) == key
                            self.__apply_dep_vrf_table(vrf, 'BGP_GLOBALS_LISTEN_PREFIX', match = match_pg)
                            match_nbr = lambda data: data.get('peer_group_name', None) == key
                            self.__apply_dep_vrf_table(vrf, 'BGP_NEIGHBOR', match = match_nbr)
                        else:
                            for af in ['ipv4_unicast', 'ipv6_unicast']:
                                syslog.syslog(syslog.LOG_DEBUG, 'apply attributes to FRR for vrf %s neighbor %s af %s' % (vrf, key, af))
                                self.__apply_dep_vrf_table(vrf, 'BGP_NEIGHBOR_AF', key, af)
                else:
                    # Neighbor is deleted
                    if is_peer_group:
                        # clear associated neighbor list in cache
                        self.__delete_pg_neighbors(vrf, key)
                    command = "vtysh -c 'configure terminal' -c 'router bgp {} vrf {}' -c 'no neighbor {}'".\
                        format(local_asn, vrf, key)
                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete VRF %s bgp neigbor %s' % (vrf, key))
                    self.__delete_vrf_neighbor(vrf, key, data, is_peer_group)
            elif table == 'BGP_NEIGHBOR_AF' or table == 'BGP_PEER_GROUP_AF':
                nbr, af_type = key.split('|')
                af, ip_type = af_type.lower().split('_')
                syslog.syslog(syslog.LOG_INFO, 'Set address family for neighbor {} to {} {}'.format(nbr, af, ip_type))
                cmd_prefix = ['configure terminal',
                              'router bgp {} vrf {}'.format(local_asn, vrf),
                              'address-family {} {}'.format(af, ip_type)]
                if not key_map.run_command(self, table, data, cmd_prefix, nbr):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP neighbor AF config command')
                    continue
            elif table == 'COMMUNITY_SET' or table == 'EXTENDED_COMMUNITY_SET':
                comm_set_name = prefix
                syslog.syslog(syslog.LOG_INFO, 'Set community set {} for table {}'.format(comm_set_name, table))
                cmd_prefix = ['configure terminal']
                if not key_map.run_command(self, table, data, cmd_prefix, comm_set_name):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP community config command')
                    continue
                extended = (table != 'COMMUNITY_SET')
                comm_set = (self.comm_set_list if not extended else self.extcomm_set_list).setdefault(comm_set_name,
                    CommunityList(comm_set_name, extended))
                if del_table:
                    del((self.comm_set_list if not extended else self.extcomm_set_list)[comm_set_name])
                else:
                    for dkey, dval in data.items():
                        if dval.op == CachedDataWithOp.OP_DELETE:
                            upd_val = None
                        else:
                            upd_val = dval.data
                        comm_set.db_data_to_attr(dkey, upd_val)
            elif table == 'PREFIX_SET':
                pfx_set_name = prefix
                if not del_table:
                    if pfx_set_name in self.prefix_set_list:
                        syslog.syslog(syslog.LOG_DEBUG, 'prefix-set %s exists with af %d' %
                                (pfx_set_name, self.prefix_set_list[pfx_set_name].af))
                        continue
                    if 'mode' not in data:
                        syslog.syslog(syslog.LOG_ERR, 'no mode given for prefix-set %s' % pfx_set_name)
                        continue
                    set_mode = data['mode'].data.lower()
                    self.prefix_set_list[pfx_set_name] = MatchPrefixList(set_mode)
                else:
                    if pfx_set_name in self.prefix_set_list:
                        del(self.prefix_set_list[pfx_set_name])
                for _, dval in data.items():
                    dval.status = CachedDataWithOp.STAT_SUCC
            elif table == 'PREFIX' or table == 'NEIGHBOR_SET' or table == 'NEXTHOP_SET':
                pfx_set_name = self.get_prefix_set_name(prefix, table)
                if table == 'PREFIX':
                    if pfx_set_name not in self.prefix_set_list:
                        syslog.syslog(syslog.LOG_ERR, 'could not find prefix-set %s from cache' % pfx_set_name)
                        continue
                    ip_pfx, len_range = key.split('|')
                    if len_range == 'exact':
                        len_range = None
                    pfx_action = data.get('action', None)
                    if pfx_action is None or pfx_action.op == CachedDataWithOp.OP_NONE:
                        continue
                    af = self.prefix_set_list[pfx_set_name].af
                    if af == socket.AF_INET:
                        # use table daemons setting
                        daemons = None
                    else:
                        daemons = ['bgpd', 'zebra']
                    if pfx_action.op == CachedDataWithOp.OP_DELETE or pfx_action.op == CachedDataWithOp.OP_UPDATE:
                        del_pfx, pfx_idx = self.prefix_set_list[pfx_set_name].get_prefix(ip_pfx, len_range)
                        if del_pfx is None:
                            syslog.syslog(syslog.LOG_ERR, 'prefix of {} with range {} not found from prefix-set {}'.\
                                            format(ip_pfx, len_range, pfx_set_name))
                            continue
                        command = "vtysh -c 'configure terminal' -c 'no {} prefix-list {} {}'".\
                                    format(('ip' if af == socket.AF_INET else 'ipv6'), pfx_set_name, str(del_pfx))
                        if not self.__run_command(table, command, daemons):
                            syslog.syslog(syslog.LOG_ERR, 'failed to delete prefix %s with range %s from set %s' %
                                          (ip_pfx, len_range, pfx_set_name))
                            continue
                        del(self.prefix_set_list[pfx_set_name][pfx_idx])
                    if pfx_action.op == CachedDataWithOp.OP_ADD or pfx_action.op == CachedDataWithOp.OP_UPDATE:
                        try:
                            add_pfx = self.prefix_set_list[pfx_set_name].add_prefix(ip_pfx, len_range, pfx_action.data)
                        except ValueError:
                            syslog.syslog(syslog.LOG_ERR, 'failed to update prefix-set %s in cache with prefix %s range %s' %
                                    (pfx_set_name, ip_pfx, len_range))
                            continue
                        command = "vtysh -c 'configure terminal' -c '{} prefix-list {} {}'".\
                                    format(('ip' if af == socket.AF_INET else 'ipv6'), pfx_set_name, str(add_pfx))
                        if not self.__run_command(table, command, daemons):
                            syslog.syslog(syslog.LOG_ERR, 'failed to add prefix %s with range %s to set %s' %
                                          (ip_pfx, len_range, pfx_set_name))
                            # revert cached update on failure
                            del_pfx, pfx_idx = self.prefix_set_list[pfx_set_name].get_prefix(ip_pfx, len_range)
                            if del_pfx is not None:
                                del(self.prefix_set_list[pfx_set_name][pfx_idx])
                            continue
                else:
                    if 'address' not in data or data['address'].op == CachedDataWithOp.OP_NONE:
                        continue
                    ip_addr_list = data['address'].data
                    if pfx_set_name in self.prefix_set_list:
                        af = self.prefix_set_list[pfx_set_name].af
                        command = "vtysh -c 'configure terminal' -c 'no {} prefix-list {}'".\
                                   format(('ip' if af == socket.AF_INET else 'ipv6'), pfx_set_name)
                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to delete existing prefix-set {}'.format(pfx_set_name))
                            continue
                        del(self.prefix_set_list[pfx_set_name])
                    if not del_table:
                        prefix_set = MatchPrefixList()
                        for ip_addr in ip_addr_list:
                            try:
                                prefix_set.add_prefix(ip_addr)
                            except ValueError:
                                continue
                        for prefix in prefix_set:
                            command = "vtysh -c 'configure terminal' -c '{} prefix-list {} {}'".\
                                       format(('ip' if prefix_set.af == socket.AF_INET else 'ipv6'), pfx_set_name, str(prefix))
                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to delete existing prefix-set {}'.format(pfx_set_name))
                                continue
                        self.prefix_set_list[pfx_set_name] = prefix_set
                for _, dval in data.items():
                    dval.status = CachedDataWithOp.STAT_SUCC
            elif table == 'AS_PATH_SET':
                as_set_name = prefix
                syslog.syslog(syslog.LOG_INFO, 'Set AS path set {} for table {}'.format(as_set_name, table))
                cmd_prefix = ['configure terminal']
                if not key_map.run_command(self, table, data, cmd_prefix, as_set_name):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP AS path set config command')
                    continue
                as_set_data = data.get('as_path_set_member', None)
                if as_set_data is not None and (as_set_data.op == CachedDataWithOp.OP_DELETE or len(as_set_data.data) == 0):
                    del_table = True
                if del_table:
                    self.as_path_set_list.pop(as_set_name, None)
                elif as_set_data is not None:
                    self.as_path_set_list[as_set_name] = as_set_data.data[:]
            elif table == 'TAG_SET':
                tag_set_name = prefix
                if not del_table and 'tag_value' not in data:
                    continue
                tag_set_data = data.get('tag_value', None)
                if tag_set_data is not None and (tag_set_data.op == CachedDataWithOp.OP_DELETE or len(tag_set_data.data) == 0):
                    del_table = True
                if not del_table:
                    self.tag_set_list[tag_set_name] = set(tag_set_data.data)
                else:
                    self.tag_set_list.pop(tag_set_name, None)
                for _, dval in data.items():
                    dval.status = CachedDataWithOp.STAT_SUCC
            elif table == 'BGP_GLOBALS_EVPN_VNI':
                af_type, vni = key.split('|')
                af, ip_type = af_type.lower().split('_')
                #this is to temporarily make table cache key accessible to key_map handler function
                self.tmp_cache_key = 'BGP_GLOBALS_EVPN_VNI&&{}|{}|{}'.format(vrf, af_type, vni)
                syslog.syslog(syslog.LOG_INFO, 'Set address family for VNI {} to {} {} cache-key to {}'.format(vni, af, ip_type, self.tmp_cache_key))
                cmd_prefix = ['configure terminal',
                              'router bgp {} vrf {}'.format(local_asn, vrf),
                              'address-family {} {}'.format(af, ip_type),
                              'vni {}'.format(vni)]
                if not key_map.run_command(self, table, data, cmd_prefix):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP L2VPN_EVPN VNI config command')
                    continue
                self.tmp_cache_key = ''
                if del_table:
                    cmd = "vtysh -c 'configure terminal'"
                    cmd += " -c 'router bgp {} vrf {}'".format(local_asn, vrf)
                    cmd += " -c 'address-family {} {}'".format(af, ip_type)
                    cmd += " -c 'no vni {}'".format(vni)
                    if not self.__run_command(table, cmd):
                        syslog.syslog(syslog.LOG_ERR, 'failed running BGP L2VPN_EVPN VNI unconfig command')
                        continue
                else:
                    if not data:
                        cmd = "vtysh -c 'configure terminal'"
                        cmd += " -c 'router bgp {} vrf {}'".format(local_asn, vrf)
                        cmd += " -c 'address-family {} {}'".format(af, ip_type)
                        cmd += " -c 'vni {}'".format(vni)
                        if not self.__run_command(table, cmd):
                            syslog.syslog(syslog.LOG_ERR, 'failed running BGP L2VPN_EVPN VNI config command')
                            continue
            elif table == 'BGP_GLOBALS_EVPN_RT':
                af_type, rt = key.split('|')
                af, ip_type = af_type.lower().split('_')
                nostr = "no " if del_table else ""
                syslog.syslog(syslog.LOG_INFO, 'Set address family for RT {} to {} {}'.format(rt, af, ip_type))
                cmd = "vtysh -c 'configure terminal'"
                cmd += " -c 'router bgp {} vrf {}'".format(local_asn, vrf)
                cmd += " -c 'address-family {} {}'".format(af, ip_type)
                cmd += " -c '{}route-target {} {}'".format(nostr,data['route-target-type'].data, rt)
                cache_tbl_key = 'BGP_GLOBALS_EVPN_RT&&{}|L2VPN_EVPN|{}'.format(vrf, rt)
                if not del_table and cache_tbl_key in self.table_data_cache.keys():
                    new_rttype = data['route-target-type'].data
                    cache_tbl_data = self.table_data_cache[cache_tbl_key]
                    if 'route-target-type' in cache_tbl_data:
                        old_rttype = cache_tbl_data['route-target-type']
                        if new_rttype == "export":
                            if old_rttype == "import" or old_rttype == "both":
                                cmd += " -c 'no route-target import {}'".format(rt)
                        if new_rttype == "import":
                            if old_rttype == "export" or old_rttype == "both":
                                cmd += " -c 'no route-target export {}'".format(rt)
                if not self.__run_command(table, cmd):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP L2VPN_EVPN RT config command')
                    continue
                else:
                    data['route-target-type'].status = CachedDataWithOp.STAT_SUCC
            elif table == 'BGP_GLOBALS_EVPN_VNI_RT':
                af_type, vni, rt = key.split('|')
                af, ip_type = af_type.lower().split('_')
                nostr = "no " if del_table else ""
                syslog.syslog(syslog.LOG_INFO, 'Set address family for VNI {} RT {} to {} {}'.format(vni, rt, af, ip_type))
                cmd = "vtysh -c 'configure terminal'"
                cmd += " -c 'router bgp {} vrf {}'".format(local_asn, vrf)
                cmd += " -c 'address-family {} {}'".format(af, ip_type)
                cmd += " -c 'vni {}'".format(vni)
                cmd += " -c '{}route-target {} {}'".format(nostr,data['route-target-type'].data, rt)
                cache_tbl_key = 'BGP_GLOBALS_EVPN_VNI_RT&&{}|L2VPN_EVPN|{}|{}'.format(vrf, vni, rt)
                if not del_table and cache_tbl_key in self.table_data_cache.keys():
                    new_rttype = data['route-target-type'].data
                    cache_tbl_data = self.table_data_cache[cache_tbl_key]
                    if 'route-target-type' in cache_tbl_data:
                        old_rttype = cache_tbl_data['route-target-type']
                        if new_rttype == "export":
                            if old_rttype == "import" or old_rttype == "both":
                                cmd += " -c 'no route-target import {}'".format(rt)
                        if new_rttype == "import":
                            if old_rttype == "export" or old_rttype == "both":
                                cmd += " -c 'no route-target export {}'".format(rt)
                if not self.__run_command(table, cmd):
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP L2VPN_EVPN VNI RT config command')
                    continue
                else:
                    data['route-target-type'].status = CachedDataWithOp.STAT_SUCC
            elif table == 'ROUTE_MAP':
                map_name = prefix
                seq_no = key
                if not del_table:
                    if 'route_operation' in data:
                        dval = data['route_operation']
                        if dval.op != CachedDataWithOp.OP_NONE:
                            enable = (dval.op != CachedDataWithOp.OP_DELETE)
                            no_arg = CommandArgument(self, enable)
                            command = "vtysh -c 'configure terminal' -c '{:no-prefix}route-map {} {} {}'".\
                                       format(no_arg, map_name, dval.data, seq_no)
                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to configure route-map {} seq {}'.format(map_name, seq_no))
                                continue
                            if dval.op == CachedDataWithOp.OP_DELETE:
                                self.__delete_route_map(map_name, seq_no, data)
                                continue
                            self.route_map.setdefault(map_name, {})[seq_no] = dval.data
                            for k, v in data.items():
                                if v.op == CachedDataWithOp.OP_NONE:
                                    v.op = CachedDataWithOp.OP_UPDATE
                        dval.status = CachedDataWithOp.STAT_SUCC
                    if map_name not in self.route_map or seq_no not in self.route_map[map_name]:
                        syslog.syslog(syslog.LOG_ERR, 'route-map {} seq {} not found for update'.format(map_name, seq_no))
                        continue
                    cmd_prefix = ['configure terminal',
                                  'route-map {} {} {}'.format(map_name, self.route_map[map_name][seq_no], seq_no)]
                    if not key_map.run_command(self, table, data, cmd_prefix):
                        syslog.syslog(syslog.LOG_ERR, 'failed running route-map config command')
                        continue
                else:
                    if map_name not in self.route_map or seq_no not in self.route_map[map_name]:
                        syslog.syslog(syslog.LOG_ERR, 'route-map {} seq {} not found for delete'.format(map_name, seq_no))
                        continue
                    command = "vtysh -c 'configure terminal' -c 'no route-map {} {} {}'".\
                               format(map_name, self.route_map[map_name][seq_no], seq_no)
                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed running route-map delete command')
                        continue
                    self.__delete_route_map(map_name, seq_no, data)
            elif table == 'ROUTE_REDISTRIBUTE':
                src_proto, dst_proto, af = key.split('|')
                if af == 'ipv6' and src_proto == 'ospf3':
                    src_proto = 'ospf6'
                ip_type = 'unicast'
                syslog.syslog(syslog.LOG_INFO, 'Set route distribute for src_proto {} dst_proto {} {}'.\
                                format(src_proto, dst_proto, af, ip_type))
                if dst_proto != 'bgp':
                    syslog.syslog(syslog.LOG_ERR, 'only bgp could be used as dst protocol, but {} was given'.format(dst_proto))
                    continue
                op = CachedDataWithOp.OP_DELETE if del_table else CachedDataWithOp.OP_UPDATE
                data['protocol'] = CachedDataWithOp(src_proto, op)
                cmd_prefix = ['configure terminal',
                              'router bgp {} vrf {}'.format(local_asn, vrf),
                              'address-family {} {}'.format(af, ip_type)]
                ret_val = key_map.run_command(self, table, data, cmd_prefix)
                del(data['protocol'])
                if not ret_val:
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP route redistribute config command')
                    continue
            elif table == 'BGP_GLOBALS_AF_AGGREGATE_ADDR' or table == 'BGP_GLOBALS_AF_NETWORK':
                af_type, ip_prefix = key.split('|')
                af, ip_type = af_type.lower().split('_')
                norm_ip_prefix = MatchPrefix.normalize_ip_prefix((socket.AF_INET if af == 'ipv4' else socket.AF_INET6), ip_prefix)
                if norm_ip_prefix is None:
                    syslog.syslog(syslog.LOG_ERR, 'invalid IP prefix format %s for af %s' % (ip_prefix, af))
                    continue
                syslog.syslog(syslog.LOG_INFO, 'Set address family for IP prefix {} to {} {}'.format(norm_ip_prefix, af, ip_type))
                op = CachedDataWithOp.OP_DELETE if del_table else CachedDataWithOp.OP_UPDATE
                data['ip_prefix'] = CachedDataWithOp(norm_ip_prefix, op)
                cmd_prefix = ['configure terminal',
                              'router bgp {} vrf {}'.format(local_asn, vrf),
                              'address-family {} {}'.format(af, ip_type)]
                ret_val = key_map.run_command(self, table, data, cmd_prefix, vrf, af)
                del(data['ip_prefix'])
                if not ret_val:
                    syslog.syslog(syslog.LOG_ERR, 'failed running BGP IP prefix AF config command')
                    continue
                if table == 'BGP_GLOBALS_AF_AGGREGATE_ADDR':
                    if not del_table:
                        aggr_obj = AggregateAddr()
                        for attr in ['as_set', 'summary_only']:
                            if attr in data and data[attr].op != CachedDataWithOp.OP_DELETE and data[attr].data == 'true':
                                setattr(aggr_obj, attr, True)
                        self.af_aggr_list.setdefault(vrf, {})[norm_ip_prefix] = aggr_obj
                    else:
                        if vrf in self.af_aggr_list:
                            self.af_aggr_list[vrf].pop(norm_ip_prefix, None)

            elif table == 'BFD_PEER_SINGLE_HOP':
                key = prefix + '|' + key
                remoteaddr, interface, vrf, localaddr = key.split('|')
                if not del_table:
                    if not 'null' in localaddr:
                        syslog.syslog(syslog.LOG_INFO, 'Set BFD single hop peer {} {} {} {}'.format(remoteaddr, vrf, interface, localaddr))

                        suffix_cmd, oper = self.__bfd_handle_delete (data)
                        if suffix_cmd and oper == CachedDataWithOp.OP_DELETE:
                            command = "vtysh -c 'configure terminal' -c 'bfd' -c 'peer {} local-address {} vrf {} interface {}' -c '{}'".\
                            format(remoteaddr, localaddr, vrf, interface, suffix_cmd)

                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to delete single-hop peer {}'.format(key))
                                continue
                        else:
                            cmd_prefix = ['configure terminal',
                                          'bfd',
                                          'peer {} local-address {} vrf {} interface {}'.format(remoteaddr, localaddr, vrf, interface)]
                            if not key_map.run_command(self, table, data, cmd_prefix):
                                syslog.syslog(syslog.LOG_ERR, 'failed running BFD single-hop config command')
                                continue

                    else:
                        syslog.syslog(syslog.LOG_INFO, 'Set BFD single hop peer {} {} {}'.format(remoteaddr, vrf, interface))

                        suffix_cmd, oper = self.__bfd_handle_delete (data)

                        if suffix_cmd and oper == CachedDataWithOp.OP_DELETE:
                            command = "vtysh -c 'configure terminal' -c 'bfd' -c 'peer {} vrf {} interface {}' -c '{}'".\
                            format(remoteaddr, vrf, interface, suffix_cmd)

                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to delete single-hop peer {}'.format(key))
                                continue
                        else:
                            syslog.syslog(syslog.LOG_INFO, 'Set BFD single hop peer {} {} {}'.format(remoteaddr, vrf, interface))
                            cmd_prefix = ['configure terminal',
                                          'bfd',
                                          'peer {} vrf {} interface {}'.format(remoteaddr, vrf, interface)]
                            if not key_map.run_command(self, table, data, cmd_prefix):
                                syslog.syslog(syslog.LOG_ERR, 'failed running BFD single-hop config command')
                                continue
                else:
                    if 'local-address' in data:
                        dval = data['local-address']
                        localaddr = dval.data
                        syslog.syslog(syslog.LOG_INFO, 'Delete BFD single hop to {} {} {}'.format(remoteaddr, vrf, interface, localaddr))
                        command = "vtysh -c 'configure terminal' -c 'bfd' -c 'no peer {} local-address {} vrf {} interface {}'".\
                            format(remoteaddr, localaddr, vrf, interface)
                    else:
                        syslog.syslog(syslog.LOG_INFO, 'Delete BFD single hop to {} {} {}'.format(remoteaddr, vrf, interface))
                        command = "vtysh -c 'configure terminal' -c 'bfd' -c 'no peer {} vrf {} interface {}'".\
                            format(remoteaddr, vrf, interface)
                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete single-hop peer {}'.format(key))
                        continue
                    self.__delete_bfd_peer(data)
            elif table == 'BFD_PEER_MULTI_HOP':
                key = prefix + '|' + key
                remoteaddr, interface, vrf, localaddr = key.split('|')
                if not del_table:
                    syslog.syslog(syslog.LOG_INFO, 'Set BFD multi hop to {} {} {} {}'.format(remoteaddr, interface, vrf, localaddr))
                    suffix_cmd, oper = self.__bfd_handle_delete (data)
                    if suffix_cmd and oper == CachedDataWithOp.OP_DELETE:
                        if not 'null' in interface:
                            command = "vtysh -c 'configure terminal' -c 'bfd' -c 'peer {} local-address {} vrf {} interface {}' -c '{}'".\
                            format(remoteaddr, localaddr, vrf, interface, suffix_cmd)
                        else:
                            command = "vtysh -c 'configure terminal' -c 'bfd' -c 'peer {} local-address {} vrf {}' -c '{}'".\
                            format(remoteaddr, localaddr, vrf, suffix_cmd)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to delete single-hop peer {}'.format(key))
                            continue
                    else:
                        if not 'null' in interface:
                            cmd_prefix = ['configure terminal',
                                          'bfd',
                                                'peer {} vrf {} multihop local-address {} interface {}'.format(remoteaddr, vrf, localaddr, interface)]
                        else:
                            cmd_prefix = ['configure terminal',
                                          'bfd',
                                          'peer {} vrf {} multihop local-address {}'.format(remoteaddr, vrf, localaddr)]

                        if not key_map.run_command(self, table, data, cmd_prefix):
                            syslog.syslog(syslog.LOG_ERR, 'failed running BFD multi-hop config command')
                            continue
                else:
                    syslog.syslog(syslog.LOG_INFO, 'Delete BFD multi hop to {} {} {} {}'.format(remoteaddr, vrf, localaddr, interface))
                    if not 'null' in interface:
                        command = "vtysh -c 'configure terminal' -c 'bfd' -c 'no peer {} vrf {} multihop local-address {} interface {}'".\
                        format(remoteaddr, vrf, localaddr, interface)
                    else:
                        command = "vtysh -c 'configure terminal' -c 'bfd' -c 'no peer {} vrf {} multihop local-address {}'".\
                        format(remoteaddr, vrf, localaddr)

                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete multihop peer {}'.format(key))
                        continue
                    self.__delete_bfd_peer(data)
            elif table == 'IP_SLA':
                sla_id = prefix
                icmp_config = False
                tcp_config = False
                cmd_prefix = ['configure terminal']
                ipsla_table = self.config_db.get_table('IP_SLA')
                syslog.syslog(syslog.LOG_INFO, 'Config ip sla data {}'.format(data))
                found_in_configdb = False
                for key, entry in ipsla_table.items():
                    ipsla_id = key
                    if sla_id == ipsla_id:
                        found_in_configdb = True
                        break
                syslog.syslog(syslog.LOG_INFO, 'Config ip sla found_in_configdb {}'.format(found_in_configdb))
                if 'icmp_source_interface' in data or 'icmp_source_ip' in data or  'icmp_size' in data or 'icmp_dst_ip' in data or 'icmp_vrf' in data or 'icmp_ttl' in data or 'icmp_tos' in data:
                    cmd_prefix = ['configure terminal','ip sla {}'.format(sla_id)]
                    icmp_config = True
                    for key, entry in ipsla_table.items():
                        ipsla_id = key
                        if sla_id == ipsla_id:
                            if 'icmp_dst_ip' in entry:
                                icmp_cmd = ("icmp", "echo")
                                icmp_cmd_str = "-".join(icmp_cmd)
                                icmp_cmd_mode = icmp_cmd_str + " " + entry['icmp_dst_ip']
                                syslog.syslog(syslog.LOG_INFO, 'Data: icmp_cmd_str %s icmp_cmd_mode %s' % (icmp_cmd_str, icmp_cmd_mode))

                                cmd_prefix = ['configure terminal','ip sla {}'.format(sla_id), icmp_cmd_mode]
                                chk_icmp_attrs = ['icmp_source_interface', 'icmp_source_ip', 'icmp_size', 'icmp_vrf', 'icmp_tos', 'icmp_ttl']
                                chk_icmp_attrs_dict = {'icmp_source_interface':'source-interface ', 'icmp_source_ip':'source-address ', 'icmp_size':'request-data-size ', 'icmp_vrf':'source-vrf ', 'icmp_tos':'tos ', 'icmp_ttl':'ttl '}
                                for attr in chk_icmp_attrs:
                                    if attr in data and data[attr].op != CachedDataWithOp.OP_DELETE:
                                        command = "vtysh -c 'configure terminal' -c 'ip sla {}' -c '{}' -c '{} {}'".\
                                        format(sla_id, icmp_cmd_mode, chk_icmp_attrs_dict[attr], data[attr].data)
                                        syslog.syslog(syslog.LOG_INFO, 'Execute Icmp Cmd {}'.format(command))
                                        if not self.__run_command(table, command):
                                            syslog.syslog(syslog.LOG_ERR, 'failed to add icmp config for  ip sla {}'.format(sla_id))
                                            continue

                    syslog.syslog(syslog.LOG_INFO, 'Done with Icmp {}'.format(sla_id))
                    if not key_map.run_command(self, table, data, cmd_prefix, sla_id):
                        syslog.syslog(syslog.LOG_ERR, 'failed running ip sla command')
                        continue
                if 'tcp_source_interface' in data or 'tcp_source_port' in data or 'tcp_source_ip' in data or 'tcp_dst_ip' in data or 'tcp_dst_port' in data or 'tcp_vrf' in data or 'tcp_ttl' in data or 'tcp_tos' in data:
                    cmd_prefix = ['configure terminal','ip sla {}'.format(sla_id)]
                    tcp_config = True
                    for key, entry in ipsla_table.items():
                        ipsla_id = key
                        if sla_id == ipsla_id:
                            if 'tcp_dst_ip' in entry and 'tcp_dst_port' in entry:
                                tcp_cmd = ("tcp", "connect")
                                tcp_cmd_str = "-".join(tcp_cmd)
                                tcp_cmd_mode = tcp_cmd_str + " " + entry['tcp_dst_ip'] + " port " + entry['tcp_dst_port']
                                syslog.syslog(syslog.LOG_INFO, 'Init Config DB Data: tcp_cmd_str %s tcp_cmd_mode %s' % (tcp_cmd_str, tcp_cmd_mode))
                                cmd_prefix = ['configure terminal','ip sla {}'.format(sla_id), tcp_cmd_mode]
                                chk_tcp_attrs = ['tcp_source_interface', 'tcp_source_ip', 'tcp_source_port', 'tcp_vrf', 'tcp_tos', 'tcp_ttl']
                                chk_tcp_attrs_dict = {'tcp_source_interface':'source-interface ', 'tcp_source_ip':'source-address ', 'tcp_source_port':'source-port ', 'tcp_vrf':'source-vrf ', 'tcp_tos':'tos ', 'tcp_ttl':'ttl '}
                                for attr in chk_tcp_attrs:
                                    if attr in data and data[attr].op != CachedDataWithOp.OP_DELETE:
                                        command = "vtysh -c 'configure terminal' -c 'ip sla {}' -c '{}' -c '{} {}'".\
                                        format(sla_id, tcp_cmd_mode, chk_tcp_attrs_dict[attr], data[attr].data)
                                        syslog.syslog(syslog.LOG_INFO, 'Execute Tcp Cmd {}'.format(command))
                                        if not self.__run_command(table, command):
                                            syslog.syslog(syslog.LOG_ERR, 'failed to add Tcp config for  ip sla {}'.format(sla_id))
                                            continue

                    syslog.syslog(syslog.LOG_INFO, 'Done with Tcp {}'.format(sla_id))
                    if not key_map.run_command(self, table, data, cmd_prefix, sla_id):
                        syslog.syslog(syslog.LOG_ERR, 'failed running ip sla command')
                        continue
                if 'frequency' in data or 'threshold' in data or 'timeout' in data:
                    syslog.syslog(syslog.LOG_INFO, 'ip sla mode Configure freq/thresh/timeout for sla {}'.format(sla_id))
                    cmd_prefix = ['configure terminal','ip sla {}'.format(sla_id)]
                    if not key_map.run_command(self, table, data, cmd_prefix, sla_id):
                        syslog.syslog(syslog.LOG_ERR, 'failed running ip sla command')
                        continue

                elif icmp_config == False or tcp_config == False:
                    syslog.syslog(syslog.LOG_INFO, 'Basic mode Configure for ip sla {}'.format(sla_id))
                    cmd_prefix = ['configure terminal']

                # Always delete ip sla if it is not found in configdb
                if not found_in_configdb:
                    command = "vtysh -c 'configure terminal' -c 'no ip sla {}'".format(sla_id)
                    syslog.syslog(syslog.LOG_ERR, 'Entry deleted in ip sla config db')
                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete router ip sla {}'.format(sla_id))
                        continue
                elif not key_map.run_command(self, table, data, cmd_prefix, sla_id):
                    syslog.syslog(syslog.LOG_ERR, 'failed running ip sla command')
                    continue

            elif table == 'OSPFV2_ROUTER':
                vrf = prefix
                if not del_table:
                    syslog.syslog(syslog.LOG_INFO, 'Create router ospf vrf {}'.format(vrf))

                    cmd_prefix = ['configure terminal',
                                  'router ospf vrf {}'.format(vrf)]

                    if not key_map.run_command(self, table, data, cmd_prefix):
                        syslog.syslog(syslog.LOG_ERR, 'failed running ospf config command')
                        continue
                else:
                    command = "vtysh -c 'configure terminal' -c 'no router ospf vrf {}'".format(vrf)

                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete router ospf vrf {}'.format(vrf))
                        continue
                    else:
                        self.__ospf_delete(data)

            elif table == 'OSPFV2_ROUTER_AREA':
                vrf = prefix
                syslog.syslog(syslog.LOG_INFO, 'Create router ospf vrf {}'.format(vrf))

                cmd_prefix = ['configure terminal',
                              'router ospf vrf {}'.format(vrf)]

                if not key_map.run_command(self, table, data, cmd_prefix, key):
                    syslog.syslog(syslog.LOG_ERR, 'failed running ospf config command')
                    continue
            elif table == 'OSPFV2_ROUTER_AREA_VIRTUAL_LINK':
                vrf = prefix

                keyvals = key.split('|')
                area = keyvals[0]
                vlinkid = keyvals[1]

                syslog.syslog(syslog.LOG_INFO, 'Create router ospf vrf {}, Vlink: {}, tableop {}'.format(vrf, data, del_table))

                if data == {}:
                    command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no area {} virtual-link {}'".\
                    format(vrf, area, vlinkid)

                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete vlink {} {}'.format(area, vlinkid))
                        continue
                    else:
                        self.__ospf_delete(data)
                else:
                    cmd_prefix = ['configure terminal',
                                  'router ospf vrf {}'.format(vrf)]

                    if not key_map.run_command(self, table, data, cmd_prefix, area, vlinkid):
                        syslog.syslog(syslog.LOG_ERR, 'failed running ospf config command')
                        continue

                    if del_table:
                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no area {} virtual-link {}'".\
                        format(vrf, area, vlinkid)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to delete vlink {} {}'.format(area, vlinkid))
                            continue
                        else:
                            self.__ospf_delete(data)

            elif table == 'OSPFV2_ROUTER_AREA_NETWORK':
                vrf = prefix
                syslog.syslog(syslog.LOG_INFO, 'Create router ospf vrf {}'.format(vrf))

                keyvals = key.split('|')
                area = keyvals[0]
                network = keyvals[1]

                if not del_table:
                    command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'network {} area {}'".\
                    format(vrf, network, area)

                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to create network {} {}'.format(area, network))
                        continue
                else:
                    command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no network {} area {}'".\
                    format(vrf, network, area)

                    if not self.__run_command(table, command):
                        syslog.syslog(syslog.LOG_ERR, 'failed to delete network {} {}'.format(area, network))
                        continue
                    else:
                        self.__ospf_delete(data)

            elif table == 'OSPFV2_ROUTER_AREA_POLICY_ADDRESS_RANGE':
                vrf = prefix

                keyvals = key.split('|')
                area = keyvals[0]
                range = keyvals[1]

                syslog.syslog(syslog.LOG_INFO, 'Create router ospf vrf {}'.format(vrf))

                if data == {}:
                   if not del_table:
                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'area {} range {}'".\
                        format(vrf, area, range)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to create range {} {}'.format(area, range))
                            continue
                   else:
                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no area {} range {}'".\
                        format(vrf, area, range)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to delete range {} {}'.format(area, range))
                            continue
                        else:
                            self.__ospf_delete(data)
                else:
                    cmd_prefix = ['configure terminal',
                              'router ospf vrf {}'.format(vrf)]

                    if not key_map.run_command(self, table, data, cmd_prefix, area, range):
                        syslog.syslog(syslog.LOG_ERR, 'failed running ospf config command')
                        continue

            elif table == 'OSPFV2_ROUTER_DISTRIBUTE_ROUTE':
                vrf = prefix

                keyvals = key.split('|')
                protocol = keyvals[0]
                direction = keyvals[1]

                if (protocol == "DIRECTLY_CONNECTED"):
                    protocol = "CONNECTED"

                syslog.syslog(syslog.LOG_INFO, 'Create redistribute-list {} {}'.format(protocol, direction))

                cmd_suffix = ""
                del_cmd_suffix = ""
                cmd_oper = ""
                rmapcmd = ""
                metriccmd = ""
                metrictypecmd = ""
                alwayscmd = ""
                acclistname = ""
                rmapoper = ""
                metricoper = ""
                metrictypeoper = ""
                alwaysoper = ""
                acclistoper = ""

                if 'route-map' in data:
                    dval = data['route-map']
                    rmapoper = dval.op
                    rmapcmd = " route-map {}".format(dval.data)

                if 'access-list' in data:
                    dval = data['access-list']
                    acclistoper = dval.op
                    acclistname = dval.data

                if 'metric' in data:
                    dval = data['metric']
                    metricoper = dval.op
                    metriccmd = " metric {}".format(dval.data)

                if 'metric-type' in data:
                    dval = data['metric-type']
                    metrictypeoper = dval.op

                    if dval.data == "TYPE_1":
                        metrictypecmd = " metric-type 1"
                    else:
                        metrictypecmd = " metric-type 2"

                if 'always' in data:
                    dval = data['always']
                    alwaysoper = dval.op
                    alwayscmd = " always"

                if not del_table:

                    if ((rmapoper == CachedDataWithOp.OP_DELETE) or
                        (metricoper == CachedDataWithOp.OP_DELETE) or
                        (metrictypeoper == CachedDataWithOp.OP_DELETE) or
                        (alwaysoper == CachedDataWithOp.OP_DELETE) or
                        (acclistoper == CachedDataWithOp.OP_DELETE)):

                        cmd_oper = "no"

                        if (alwaysoper == CachedDataWithOp.OP_DELETE):
                            del_cmd_suffix = alwayscmd
                        if (rmapoper == CachedDataWithOp.OP_DELETE):
                            del_cmd_suffix = rmapcmd
                        if (metricoper == CachedDataWithOp.OP_DELETE):
                            del_cmd_suffix = metriccmd
                        if (metrictypeoper == CachedDataWithOp.OP_DELETE):
                            del_cmd_suffix = metrictypecmd

                    if (direction == "EXPORT"):
                        if (cmd_oper != "no"):
                            cmd_suffix = "distribute-list {} out {}".format(acclistname, protocol.lower())
                        else:
                            cmd_suffix = "no distribute-list {} out {}".format(acclistname, protocol.lower())

                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c '{}'".\
                            format(vrf, cmd_suffix)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to create distribute-list {} {}'.format(protocol, direction))
                            continue
                        else:
                            self.__ospf_apply_config(data, rmapoper, metricoper, metrictypeoper, alwaysoper, acclistoper)
                    elif (direction == "IMPORT"):
                        if (cmd_oper != "no"):
                            if (protocol == "DEFAULT_ROUTE"):
                                cmd_suffix = cmd_suffix + "default-information originate" + alwayscmd + rmapcmd + metriccmd + metrictypecmd
                            else:
                                cmd_suffix = cmd_suffix + "redistribute {}".format(protocol.lower()) + rmapcmd + metriccmd + metrictypecmd
                        else:
                            if (protocol == "DEFAULT_ROUTE"):
                                cmd_suffix = "no default-information originate" + del_cmd_suffix
                            else:
                                cmd_suffix = "no redistribute {}".format(protocol.lower()) + del_cmd_suffix

                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c '{}'".\
                            format(vrf, cmd_suffix)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to create default-info/redistribute {} {}'.format(protocol, direction))
                            continue
                        else:
                            self.__ospf_apply_config(data, rmapoper, metricoper, metrictypeoper, alwaysoper, acclistoper)
                else:
                    if (direction == "IMPORT"):
                        command = ""
                        if (protocol == "DEFAULT_ROUTE"):
                            command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no default-information originate'".\
                            format(vrf)
                        else:
                            command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no redistribute {}'".\
                            format(vrf, protocol.lower())

                        if (command != ""):
                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to delete default-info/redistribute {}'.format(protocol.lower()))
                                continue
                            else:
                                self.__ospf_delete(data)
                    else:
                        if (acclistname != ""):
                            command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no distribute-list {} out {}'".\
                            format(vrf, acclistname, protocol.lower())

                            if not self.__run_command(table, command):
                                syslog.syslog(syslog.LOG_ERR, 'failed to delete distribute-list {} {}'.format(protocol, direction))
                                continue

                        self.__ospf_delete(data)

            elif table == 'OSPFV2_INTERFACE':

                key = prefix + '|' + key
                if_name, if_addr = key.split('|')

                vrf = ""
                if 'vrf_name' in data :
                   vrf = 'vrf {}'.format(data['vrf_name'].data)

                cmd_prefix = ['configure terminal',
                              'interface {} {}'.format(if_name, vrf) ]

                if del_table and len(data) == 0:
                    syslog.syslog(syslog.LOG_INFO, 'Delete table {} {} data {}'.format(key, vrf, data))

                    cmd_data = {}
                    cache_tbl_key = 'OSPFV2_INTERFACE&&{}|{}'.format(if_name, if_addr)
                    syslog.syslog(syslog.LOG_INFO, 'Row delete key {}'.format(cache_tbl_key))

                    if cache_tbl_key in self.table_data_cache.keys():
                        cache_tbl_data = self.table_data_cache[cache_tbl_key]
                        syslog.syslog(syslog.LOG_INFO, 'Row delete cached data {} '.format(cache_tbl_data))

                        for key, data in cache_tbl_data.items() :
                            cached_op_data = CachedDataWithOp(data, CachedDataWithOp.OP_DELETE)
                            cmd_data.update({ key : cached_op_data } )

                    syslog.syslog(syslog.LOG_INFO, 'Row delete cmd data {} '.format(cmd_data))

                    if len(cmd_data) :
                        if not key_map.run_command(self, table, cmd_data, cmd_prefix, if_name, if_addr):
                            syslog.syslog(syslog.LOG_INFO, 'failed running interface no ip ospf config command')
                            self.__apply_config_delete_success(cmd_data)
                            continue
                    else :
                        self.__apply_config_delete_success(cmd_data)

                else :
                    syslog.syslog(syslog.LOG_INFO, 'Create/update ospf {} interface {} in {}'.format(key, if_name, vrf))

                    #Work arround for router area config fail, update area every time
                    if 'area-id' in data.keys():
                        dval = data['area-id']
                        if dval.op == CachedDataWithOp.OP_NONE :
                            dval.op = CachedDataWithOp.OP_ADD

                    if not key_map.run_command(self, table, data, cmd_prefix, if_name, if_addr):
                        syslog.syslog(syslog.LOG_ERR, 'failed running interface ip ospf config command')
                        if 'area-id' in data.keys():
                            dval = data['area-id']
                            if dval.op == CachedDataWithOp.OP_DELETE:
                                #Work arround for router area config delete fail
                                self.__apply_config_op_success(data, {'area-id': dval.op } )
                                syslog.syslog(syslog.LOG_INFO, 'area-id delete enforced')
                        continue
                    else :
                        self.__apply_config_op_success(data)

            elif table == 'OSPFV2_ROUTER_PASSIVE_INTERFACE':
                syslog.syslog(syslog.LOG_INFO, 'Create passive interface')

                vrf = prefix

                keyvals = key.split('|')
                if_name = keyvals[0]
                if_addr = keyvals[1]

                syslog.syslog(syslog.LOG_INFO, 'Create passive interface vrf {}'.format(vrf))

                if (if_addr == "0.0.0.0"):
                    if_addr = ""

                if data == {}:
                   if not del_table:

                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'passive-interface {} {}'".\
                        format(vrf, if_name, if_addr)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to create passive interface {} {}'.format(if_name, if_addr))
                            continue
                   else:
                        command = "vtysh -c 'configure terminal' -c 'router ospf vrf {}' -c 'no passive-interface {} {}'".\
                        format(vrf, if_name, if_addr)

                        if not self.__run_command(table, command):
                            syslog.syslog(syslog.LOG_ERR, 'failed to delete passive interface {} {}'.format(if_name, if_addr))
                            continue

            elif table == 'STATIC_ROUTE':
                vrf = prefix
                syslog.syslog(syslog.LOG_INFO, 'Set static IP route for vrf {} prefix {}'.format(vrf, key))
                op = CachedDataWithOp.OP_DELETE if del_table else CachedDataWithOp.OP_UPDATE
                data['ip_prefix'] = CachedDataWithOp(key, op)
                cmd_prefix = ['configure terminal', 'vrf {}'.format(vrf)]
                ret_val = key_map.run_command(self, table, data, cmd_prefix, vrf)
                del(data['ip_prefix'])
                if not ret_val:
                    syslog.syslog(syslog.LOG_ERR, 'failed running static route config command')
                    continue
                self.static_route_list.setdefault(vrf, {})[key] = self.upd_nh_set
            elif table == 'PIM_INTERFACE':

                vrf = prefix
                af, if_name = key.split('|')
                syslog.syslog(syslog.LOG_INFO,
                              'PIM interface update for vrf {}, af: {}, interface {}'.format(vrf, af, if_name))
                cmd_prefix = ['configure terminal',
                              'interface {}'.format(if_name)]
                syslog.syslog(syslog.LOG_INFO,
                              'Create/update PIM interface: key {} interface {} in {}'.format(key, if_name, vrf))

                # If sparse-mode has been disabled, clear other interface
                # entries in cache so that they will be re-programmed in FRR
                # on re-enabling of sparse-mode.

                if 'mode' in data:
                    modeval = data['mode']
                    modeval_pim_mode = modeval.data
                    modeval_op = modeval.op
                    if (modeval_op == CachedDataWithOp.OP_DELETE):
                        syslog.syslog(syslog.LOG_INFO,
                                      "Flushing PIM interface cache for deletion "
                                      "of PIM sparse-mode")
                        for dkey, dval in data.items():
                            dval.status = CachedDataWithOp.STAT_SUCC
                            dval.op = CachedDataWithOp.OP_DELETE

                    # Only send the VTYSH command to FRR if the PIM interface mode
                    # is present in the update.
                    if not key_map.run_command(self, table, data, cmd_prefix):
                        syslog.syslog(syslog.LOG_ERR, 'failed running PIM config command')
                        continue

            elif table == 'PIM_GLOBALS':

                vrf = prefix

                af = key.split('|')
                syslog.syslog(syslog.LOG_INFO,
                              'PIM global update for vrf {}, af: {}'.format(vrf, af))

                cmd_prefix = ['configure terminal',
                              'vrf {}'.format(vrf)]

                syslog.syslog(syslog.LOG_INFO,
                              'Create/update PIM global {} af {} in {}'.format(key, af, vrf))

                # if not key_map.run_command(self, table, data, cmd_prefix, vrf, af):
                if not key_map.run_command(self, table, data, cmd_prefix):
                    syslog.syslog(syslog.LOG_ERR, 'failed running PIM config command')
                    continue

            elif table == 'IGMP_INTERFACE':
                ifname = prefix

                syslog.syslog(syslog.LOG_INFO, 'IGMP Interface MCast Grp ifname {} prefix {}'.format(ifname, key))

                keyvals = key.split('|')
                mcast_grp = keyvals[0]
                source_ip = keyvals[1]

                syslog.syslog(syslog.LOG_INFO, 'Configure ip igmp join interface {}, mcast_grp {}, source_ip {}'.format(ifname, mcast_grp, source_ip))

                cmd_prefix = ['configure terminal',
                          'interface {}'.format(ifname)]

                if not key_map.run_command(self, table, data, cmd_prefix, mcast_grp, source_ip):
                    syslog.syslog(syslog.LOG_ERR, 'failed running ip igmp join config command')
                    continue


            elif table == 'IGMP_INTERFACE_QUERY':
                ifname = prefix

                syslog.syslog(syslog.LOG_INFO, 'IGMP Interface {} Config prefix {}'.format(ifname, key))

                cmd_prefix = ['configure terminal',
                          'interface {}'.format(ifname)]

                if not key_map.run_command(self, table, data, cmd_prefix):
                    syslog.syslog(syslog.LOG_ERR, 'failed running ip igmp interface config command')
                    continue


    def __add_op_to_data(self, table_key, data, comb_attr_list):
        cached_data = self.table_data_cache.setdefault(table_key, {})
        for key in cached_data:
            if key in data:
                # both in cache and data, update/none
                data[key] = (CachedDataWithOp(data[key], CachedDataWithOp.OP_NONE) if data[key] == cached_data[key] else
                             CachedDataWithOp(data[key], CachedDataWithOp.OP_UPDATE))
            else:
                # in cache but not in data, delete
                data[key] = CachedDataWithOp(cached_data[key], CachedDataWithOp.OP_DELETE)
        for key in data:
            if not isinstance(data[key], CachedDataWithOp):
                # in data but not in cache, add
                data[key] = CachedDataWithOp(data[key], CachedDataWithOp.OP_ADD)
        # combo attributes handling
        op_list = [CachedDataWithOp.OP_DELETE, CachedDataWithOp.OP_ADD, CachedDataWithOp.OP_UPDATE, CachedDataWithOp.OP_NONE]
        for key_set in comb_attr_list:
            all_in = True
            op_idx = len(op_list) - 1
            for key in key_set:
                if key not in data:
                    all_in = False
                    break
                idx = op_list.index(data[key].op)
                if idx >= 0 and idx < op_idx:
                    op_idx = idx
            if all_in:
                for key in key_set:
                    data[key].op = op_list[op_idx]
            else:
                # if one key doesn't exist, clean the whole key set
                for key in key_set:
                    data.pop(key, None)

    def __update_cache_data(self, table_key, data):
        cached_data = self.table_data_cache.setdefault(table_key, {})
        for key, val in data.items():
            if not isinstance(val, CachedDataWithOp) or val.op == CachedDataWithOp.OP_NONE or val.status == CachedDataWithOp.STAT_FAIL:
                syslog.syslog(syslog.LOG_DEBUG, 'ignore cache update for %s because of %s%s%s' %
                        (key, ('' if isinstance(val, CachedDataWithOp) else 'INV_DATA '),
                              ('NO_OP ' if isinstance(val, CachedDataWithOp) and val.op == CachedDataWithOp.OP_NONE else ''),
                              ('STAT_FAIL ' if isinstance(val, CachedDataWithOp) and val.status == CachedDataWithOp.STAT_FAIL else '')))
                continue
            if val.op == CachedDataWithOp.OP_ADD or val.op == CachedDataWithOp.OP_UPDATE:
                cached_data[key] = val.data
                syslog.syslog(syslog.LOG_INFO, 'Add {} data {} to cache'.format(key, cached_data[key]))
            elif val.op == CachedDataWithOp.OP_DELETE:
                syslog.syslog(syslog.LOG_INFO, 'delete {} data {} from cache'.format(key, cached_data.get(key, '')))
                cached_data.pop(key, None)
        if len(cached_data) == 0:
            syslog.syslog(syslog.LOG_INFO, 'delete table row {} from cache'.format(table_key))
            del(self.table_data_cache[table_key])


    def bgp_table_handler_common(self, table, key, data, comb_attr_list = []):
        syslog.syslog(syslog.LOG_DEBUG, '----------------------------------')
        syslog.syslog(syslog.LOG_DEBUG, ' BGP table handling')
        syslog.syslog(syslog.LOG_DEBUG, '----------------------------------')
        syslog.syslog(syslog.LOG_DEBUG, 'table : %s' % table)
        syslog.syslog(syslog.LOG_DEBUG, 'key   : %s' % key)
        op_str = ('DELETE' if data is None else 'SET')
        del_table = False
        if data is None:
            data = {}
            del_table = True
        syslog.syslog(syslog.LOG_DEBUG, 'op    : %s' % op_str)
        syslog.syslog(syslog.LOG_DEBUG, 'data  :')
        for dkey, dval in data.items():
            syslog.syslog(syslog.LOG_DEBUG, '        %-10s - %s' % (dkey, dval))
        syslog.syslog(syslog.LOG_DEBUG, '')
        table_key = ExtConfigDBConnector.get_table_key(table, key)
        self.__add_op_to_data(table_key, data, comb_attr_list)
        self.bgp_message.put((key, del_table, table, data))
        upd_data_list = []
        self.__update_bgp(upd_data_list)
        for table, key, data in upd_data_list:
            table_key = ExtConfigDBConnector.get_table_key(table, key)
            self.__update_cache_data(table_key, data)

    def bgp_global_handler(self, table, key, data):
        self.bgp_table_handler_common(table, key, data, [{'keepalive', 'holdtime'}])

    def bgp_af_handler(self, table, key, data):
        self.bgp_table_handler_common(table, key, data, [{'ebgp_route_distance', 'ibgp_route_distance', 'local_route_distance'},
                                        {'route_flap_dampen_reuse_threshold', 'route_flap_dampen_suppress_threshold', 'route_flap_dampen_max_suppress'}])

    def bgp_neighbor_handler(self, table, key, data):
        self.bgp_table_handler_common(table, key, data, [{'keepalive', 'holdtime'}])

    def comm_set_handler(self, table, key, data):
        self.bgp_table_handler_common(table, key, data, [{'set_type', 'match_action', 'community_member'}])

    def bfd_shop_handler(self, table, key, data):
        self.bgp_table_handler_common(table, key, data, [{'remote-address', 'vrf', 'interface'},
                                                         {'remote-address', 'vrf', 'interface', 'local-address'}])
    def bfd_mhop_handler(self, table, key, data):
        self.bgp_table_handler_common(table, key, data, [{'remote-address', 'vrf', 'local-address'}])

    def start(self):
        self.subscribe_all()
        self.config_db.listen()
    def stop(self):
        self.config_db.stop_listen()
        if self.config_db.sub_thread.is_alive():
            self.config_db.sub_thread.join()

main_loop = True

def sig_handler(signum, frame):
    global main_loop
    syslog.syslog(syslog.LOG_DEBUG, 'entering signal handler')
    main_loop = False

def main():
    global bgpd_client
    for sig_num in [signal.SIGTERM, signal.SIGINT]:
        signal.signal(sig_num, sig_handler)
    syslog.syslog(syslog.LOG_DEBUG, 'entering BGP configuration daemon')
    bgpd_client = BgpdClientMgr()
    bgpd_client.start()
    daemon = BGPConfigDaemon()
    daemon.start()
    while main_loop:
        signal.pause()
    syslog.syslog(syslog.LOG_DEBUG, 'leaving BGP configuration daemon')
    bgpd_client.shutdown()
    daemon.stop()

if __name__ == "__main__":
    main()
