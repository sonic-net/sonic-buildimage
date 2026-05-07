#!/usr/bin/env python3
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys, os, syslog
import getopt, subprocess, threading
import time, socket
import signal, select
from functools import partial
import queue as Queue

RUN = False
PASS = 0
FAIL = 1

SOCKET_PORT = 50000
SOCKET_RECV_BUFFER = 4096
SOCKET_TIME_OUT = 20
FILE_LOCK = threading.Lock()
SOCKET_LOCK = threading.Lock()

CMD_TYPE = ['global', 'device']
RESPONSE_ERROR_PARAMETER = "Parameters error"
I2C_PREFIX = '/sys/bus/i2c/devices/'
def doBash(cmd):
	status, output = subprocess.getstatusoutput(cmd)

	return  status, output

def doSend(msg, port):
	if SOCKET_LOCK.acquire():
		host = socket.gethostname()
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(SOCKET_TIME_OUT)
	
		try:
			s.connect((host, port))
		except:
			sys.exit(0)
	
		s.sendall(msg.encode(encoding='utf-8'))
		result = s.recv(SOCKET_RECV_BUFFER).decode(encoding='utf-8')
		s.close()
		SOCKET_LOCK.release()
	return result

def readFile(path):
	if FILE_LOCK.acquire():
		try:
			file = open(path)
		except IOError as e:
			print("Error: unable to open file: %s" % str(e))
			FILE_LOCK.release()
			return 'Error'

		value = file.readline().rstrip()
		file.close()
		FILE_LOCK.release()

	return value

def writeFile(path, value):
	if FILE_LOCK.acquire():
		try:
			file = open(path, "w")
		except IOError as e:
			print("Error: unable to open file: %s" % str(e))
			FILE_LOCK.release()
			return 'Error'
		
		file.seek(0)
		file.write(str(value))
		file.close()
		FILE_LOCK.release()

	return "Success"
