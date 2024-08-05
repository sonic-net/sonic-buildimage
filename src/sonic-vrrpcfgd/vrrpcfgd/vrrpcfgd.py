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
import ipaddress
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

vrrpd_client = None

def g_run_command(table, command, daemons, ignore_fail = False):
    syslog.syslog(syslog.LOG_DEBUG, "execute command {} for table {}.".format(command, table))
    if not command.startswith('vtysh '):
        syslog.syslog(syslog.LOG_ERR, 'Command no start with "vtysh". Command: "{}"'.format(command))
    else:
        if not vrrpd_client.run_vtysh_command(table, command, daemons) and not ignore_fail:
            syslog.syslog(syslog.LOG_ERR, 'command execution failure. Command: "{}"'.format(command))
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

class VrrpdClientMgr(threading.Thread):
    VTYSH_MARK = 'vtysh '
    PROXY_SERVER_ADDR = '/etc/frr/vrrpd_client_sock'
    ALL_DAEMONS = ['vrrpd']
    TABLE_DAEMON = {
            'VRRP': ['vrrpd'],
            'VRRP6': ['vrrpd'],
            'VRRP_TRACK': ['vrrpd'],
            'VRRP6_TRACK': ['vrrpd'],
    }
    VTYSH_CMD_DAEMON = [(r'show vrrp($|\s+\S+)', ['vrrpd'])]
    @staticmethod
    def __create_proxy_socket():
        try:
            os.unlink(VrrpdClientMgr.PROXY_SERVER_ADDR)
        except OSError:
            if os.path.exists(VrrpdClientMgr.PROXY_SERVER_ADDR):
                raise
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.bind(VrrpdClientMgr.PROXY_SERVER_ADDR)
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
        super(VrrpdClientMgr, self).__init__(name = 'VTYSH sub-process manager')
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
        syslog.syslog(syslog.LOG_DEBUG, 'terminate vrrpcfgd client manager')
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
                syslog.syslog(syslog.LOG_ERR, '[vrrp cfgd] Failed handling config DB update with exception:' + str(e))
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
        self.sub_thread = threading.Thread(target=self.listen_thread, args=(0.01,))
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

class VRRPConfigDaemon:
    DEFAULT_VRF = 'default'

    def __init__(self):
        self.config_db = ExtConfigDBConnector()
        try:
            self.config_db.connect()
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, '[vrrp cfgd] Failed connecting to config DB with exception:' + str(e))
        db_entry = self.config_db.get_entry('DEVICE_METADATA', 'localhost')
        if 'docker_routing_config_mode' in db_entry:
            self.config_mode = db_entry['docker_routing_config_mode']
        else:
            self.config_mode = "separated"

        self.table_handler_list = [
            ('VRRP', self.vrrp_handler),
            ('VRRP6', self.vrrp6_handler),
            ('VRRP_TRACK', self.vrrp_track_handler),
            ('VRRP6_TRACK', self.vrrp6_track_handler),
        ]
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
        return g_run_command(table, command, daemons)

    def vrrp_handler(self, table, key, data):
        syslog.syslog(syslog.LOG_INFO, '[vrrp cfgd](vrrp) value for {} changed to {}'.format(key, data))
        #get frr vrrp session key
        key_params = key.split('|')
        intf_cmd = 'interface {}'.format(key_params[0])
        cmd = 'vrrp {}'.format(key_params[1])
        table_key = ExtConfigDBConnector.get_table_key(table, key)
        comb_attr_list = ['vip']
        if not data:
            #VRRP instance is deleted
            command = "vtysh -c 'configure terminal' -c '{}' -c 'no {}'".format(intf_cmd, cmd)
            self.__run_command(table, command)
            #del cache data
            del(self.table_data_cache[table_key])
        else:
            #create/update case
            command = "vtysh -c 'configure terminal' -c '{}'".format(intf_cmd)
            self.__add_op_to_data(table_key, data, comb_attr_list)
            cached_data = self.table_data_cache.setdefault(table_key, {})
            for param in data:
                if param == 'vid':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    elif 'vip' not in data:
                        command = command + " -c '{}'".format(cmd)
                elif param == 'vip':
                    if 'vip' in cached_data:
                        cache_address = cached_data[param]
                        data_address = data[param].data
                        # add vip
                        for d_address in data_address:
                            if d_address in cache_address:
                                continue
                            elif d_address != "":
                                d_addr = d_address.split('/')
                                try:
                                    ip_address = ipaddress.ip_interface(d_addr[0])
                                except ValueError as err:
                                    syslog.syslog(syslog.LOG_ERR, '[bgp vrrpd] IP address is not valid:{}'.format(err))
                                if ip_address.version == 4:
                                    command = command + " -c '{} ip {}'".format(cmd, d_addr[0])
                        # del vip
                        for c_address in cache_address:
                            if c_address in data_address:
                                continue
                            elif c_address != "":
                                c_addr = c_address.split('/')
                                try:
                                    ip_address = ipaddress.ip_interface(c_addr[0])
                                except ValueError as err:
                                    syslog.syslog(syslog.LOG_ERR, '[bgp vrrpd] IP address is not valid:{}'.format(err))
                                if ip_address.version == 4:
                                    command = command + " -c 'no {} ip {}'".format(cmd, c_addr[0])
                    else:
                        # first time to config
                        data_address = data[param].data
                        for d_address in data_address:
                            d_addr = d_address.split('/')
                            try:
                                ip_address = ipaddress.ip_interface(d_addr[0])
                            except ValueError as err:
                                syslog.syslog(syslog.LOG_ERR, '[bgp vrrpd] IP address is not valid:{}'.format(err))
                            if ip_address.version == 4:
                                command = command + " -c '{} ip {}'".format(cmd, d_addr[0])
                elif param == 'priority':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        command = command + " -c '{} priority {}'".format(cmd, data[param].data)
                elif param == 'adv_interval':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        command = command + " -c '{} advertisement-interval {}'".format(cmd, data[param].data)
                elif param == 'version':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        command = command + " -c '{} version {}'".format(cmd, data[param].data)
                elif param == 'admin_status':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        if data[param].data == 'down':
                            command = command + " -c '{} shutdown'".format(cmd)
                        elif data[param].data == 'up' or data[param].data == '':
                            command = command + " -c 'no {} shutdown'".format(cmd)
                elif param == 'preempt':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        if data[param].data == 'enabled':
                            command = command + " -c '{} preempt'".format(cmd)
                        elif data[param].data == 'disabled':
                            command = command + " -c 'no {} preempt'".format(cmd)
                data[param].status = CachedDataWithOp.STAT_SUCC
            self.__update_cache_data(table_key, data)
            self.__run_command(table, command)

    def vrrp6_handler(self, table, key, data):
        syslog.syslog(syslog.LOG_INFO, '[bgp cfgd](vrrp6) value for {} changed to {}'.format(key, data))
        #get frr vrrp6 session key
        key_params = key.split('|')
        intf_cmd = 'interface {}'.format(key_params[0])
        cmd = 'vrrp6 {}'.format(key_params[1])
        table_key = ExtConfigDBConnector.get_table_key(table, key)
        comb_attr_list = ['vip']
        if not data:
            #VRRP instance is deleted
            command = "vtysh -c 'configure terminal' -c '{}' -c 'no {}'".format(intf_cmd, cmd)
            self.__run_command(table, command)
            #del cache data
            del(self.table_data_cache[table_key])
        else:
            #create/update case
            command = "vtysh -c 'configure terminal' -c '{}'".format(intf_cmd)
            self.__add_op_to_data(table_key, data, comb_attr_list)
            cached_data = self.table_data_cache.setdefault(table_key, {})
            for param in data:
                if param == 'vid':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    elif 'vip' not in data:
                        command = command + " -c '{}'".format(cmd)
                elif param == 'vip':
                    if 'vip' in cached_data:
                        cache_address = cached_data[param]
                        data_address = data[param].data
                        for d_address in data_address:
                            if d_address in cache_address:
                                continue
                            elif d_address != "":
                                d_addr = d_address.split('/')
                                try:
                                    ip_address = ipaddress.ip_interface(d_addr[0])
                                except ValueError as err:
                                    syslog.syslog(syslog.LOG_ERR, '[bgp vrrpd] IPv6 address is not valid:{}'.format(err))
                                if ip_address.version == 6:
                                    command = command + " -c '{} ipv6 {}'".format(cmd, d_addr[0])

                        for c_address in cache_address:
                            if c_address in data_address:
                                continue
                            elif c_address != "":
                                c_addr = c_address.split('/')
                                try:
                                    ip_address = ipaddress.ip_interface(c_addr[0])
                                except ValueError as err:
                                    syslog.syslog(syslog.LOG_ERR, '[bgp vrrpd] IPv6 address is not valid:{}'.format(err))
                                if ip_address.version == 6:
                                    command = command + " -c 'no {} ipv6 {}'".format(cmd, c_addr[0])
                    else:
                        # first time to config 
                        data_address = data[param].data
                        for d_address in data_address:
                            d_addr = d_address.split('/')
                            try:
                                ip_address = ipaddress.ip_interface(d_addr[0])
                            except ValueError as err:
                                syslog.syslog(syslog.LOG_ERR, '[bgp vrrpd] IPv6 address is not valid:{}'.format(err))
                            if ip_address.version == 6:
                                command = command + " -c '{} ipv6 {}'".format(cmd, d_addr[0])
                elif param == 'priority':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        command = command + " -c '{} priority {}'".format(cmd, data[param].data)
                elif param == 'adv_interval':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        command = command + " -c '{} advertisement-interval {}'".format(cmd, data[param].data)
                elif param == 'version':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        command = command + " -c '{} version {}'".format(cmd, data[param].data)
                elif param == 'admin_status':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        if data[param].data == 'down':
                            command = command + " -c '{} shutdown'".format(cmd)
                        elif data[param].data == 'up' or data[param].data == '':
                            command = command + " -c 'no {} shutdown'".format(cmd)
                elif param == 'preempt':
                    if param in cached_data and data[param].data == cached_data[param]:
                        continue
                    else:
                        if data[param].data == 'enabled':
                            command = command + " -c '{} preempt'".format(cmd)
                        elif data[param].data == 'disabled':
                            command = command + " -c 'no {} preempt'".format(cmd)
                data[param].status = CachedDataWithOp.STAT_SUCC
            self.__update_cache_data(table_key, data)
            self.__run_command(table, command)

    def vrrp_track_handler(self, table, key, data):
        syslog.syslog(syslog.LOG_INFO, '[bgp cfgd](vrrp track) value for {} changed to {}'.format(key, data))
        #get frr vrrp track key
        key_params = key.split('|')
        intf_cmd = 'interface {}'.format(key_params[0])
        cmd = 'vrrp {} track-interface {}'.format(key_params[1], key_params[2])

        if not data:
            #VRRP track instance is deleted
            command = "vtysh -c 'configure terminal' -c '{}' -c 'no {}'".format(intf_cmd, cmd)
            self.__run_command(table, command)
        else:
            #create/update case
            if 'priority_increment' in data:
                command = "vtysh -c 'configure terminal' -c '{}' -c '{} priority-dec {}'".format(intf_cmd, cmd, data['priority_increment'])
                self.__run_command(table, command)

    def vrrp6_track_handler(self, table, key, data):
        syslog.syslog(syslog.LOG_INFO, '[bgp cfgd](vrrp6 track) value for {} changed to {}'.format(key, data))
        #get frr vrrp6 track key
        key_params = key.split('|')
        intf_cmd = 'interface {}'.format(key_params[0])
        cmd = 'vrrp6 {} track-interface {}'.format(key_params[1], key_params[2])

        if not data:
            #VRRP track instance is deleted
            command = "vtysh -c 'configure terminal' -c '{}' -c 'no {}'".format(intf_cmd, cmd)
            self.__run_command(table, command)
        else:
            #create/update case
            if 'priority_increment' in data:
                command = "vtysh -c 'configure terminal' -c '{}' -c '{} priority-dec {}'".format(intf_cmd, cmd, data['priority_increment'])
                self.__run_command(table, command)

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
    global vrrpd_client
    for sig_num in [signal.SIGTERM, signal.SIGINT]:
        signal.signal(sig_num, sig_handler)
    syslog.syslog(syslog.LOG_DEBUG, 'entering VRRP configuration daemon')
    vrrpd_client = VrrpdClientMgr()
    vrrpd_client.start()
    daemon = VRRPConfigDaemon()
    daemon.start()
    while main_loop:
        signal.pause()
    syslog.syslog(syslog.LOG_DEBUG, 'leaving VRRP configuration daemon')
    vrrpd_client.shutdown()
    daemon.stop()

if __name__ == "__main__":
    main()
