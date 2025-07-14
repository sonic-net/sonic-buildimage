#!/usr/bin/env python3
#
# Copyright (C) 2019 Pegatron, Inc.
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

from pegaProcess import common

#message handler
def doCommand(cmd):
	"""
	Command:
		global          : Set global config
		device          : Set device config
	"""
	if len(cmd[0:]) < 1 or cmd[0] not in common.CMD_TYPE:
		print(doCommand.__doc__)
		return
	msg = ' '.join(str(data) for data in cmd)
	result = common.doSend(msg, common.SOCKET_PORT)
	print (result)
	
	return

def main():
	"""
	Command:
		install		: Install drivers
		uninstall	: Uninstall drivers
		cmd		: Commands
	"""	
	args = common.sys.argv[1:]

	if len(args[0:]) < 1:
                print(main.__doc__)
                return
	
	if args[0] == 'uninstall':
		doCommand(['global', 'uninstall'])
	elif args[0] == 'cmd':
		doCommand(args[1:])
	else:
		print(main.__doc__)
	common.sys.exit(0)

if __name__ == "__main__":
	main()
