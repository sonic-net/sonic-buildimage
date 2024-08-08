#!/usr/bin/python
import os
import sys
import re
import time
import sys
import subprocess
import shlex

interface_output = str(os.popen('show interface stat').read())
pattern_int = re.findall('Ethernet[0-9]+',interface_output)

os.system('sudo config feature state iccpd enabled')
mclag_output = str(os.popen('mclagdctl dump state').read())
   
commands = []

if not mclag_output:

    commands = commands + ['show platform syseeprom', 'show version', 'show boot', 'show tacacs', 'show radius', 'show aaa',
                'show platform psustatus', 'show platform pcieinfo', 'show platform temp',
                'show platform summary', 'show platform firmware status',
                'show platform fan', 'show interface status', 'sudo ifconfig -a', 'show ip interface',
                'show vlan brief', 'show lldp neighbors' , 'show lldp table', 'show ztp status', 'show ip bgp summary', 'show ip bgp neighbors', 'show route-map',
                'show interface portchannel', 'show ip route', 'sudo route -n', 'show arp', 'sudo arp -n', 'show mac',
                'show acl rule', 'sudo aclshow -a -vv', 'show acl table', 'show interface counters -a', 'show int autoneg status',
                'sudo show system-health detail', 'show system-mem',
                'docker ps', 'top -n 1', 'show uptime', 'show reboot-cause history', 
                'sudo show system-health summary','show int transceiver presence', 'show int transceiver eeprom', 'show int transceiver error-status',
                'show int transceiver lpmode', 'show clock', 'show ntp', 'sudo timedatectl status', 'show runningconfiguration all']

else:

    mclag_domain = re.findall('Domain id: [0-9]+',mclag_output)
    mclag_final = mclag_domain[0].split(':')    
    commands = commands + ['show platform syseeprom', 'show version', 'show boot', 'show tacacs', 'show radius', 'show aaa',
                'show platform psustatus', 'show platform pcieinfo', 'show platform temp',
                'show platform summary', 'show platform firmware status',
                'show platform fan', 'show interface status', 'sudo ifconfig -a', 'show ip interface',
                'show vlan brief', 'show lldp neighbors' , 'show lldp table', 'show ztp status', 'show ip bgp summary', 'show ip bgp neighbors', 'show route-map',
                'show interface portchannel', 'show ip route', 'sudo route -n', 'show arp', 'sudo arp -n', 'show mac',
                'show acl rule', 'sudo aclshow -a -vv', 'show acl table', 'show interface counters -a', 'show int autoneg status',
                'mclagdctl dump state', ('mclagdctl dump mac -i ' + str(mclag_final[1])), ('mclagdctl dump arp -i ' + str(mclag_final[1])),
                'sudo show system-health detail', 'show system-mem',
                'docker ps', 'top -n 1', 'show uptime', 'show reboot-cause history', 
                'sudo show system-health summary','show int transceiver presence', 'show int transceiver eeprom', 'show int transceiver error-status',
                'show int transceiver lpmode', 'show clock', 'show ntp', 'sudo timedatectl status', 'show runningconfiguration all']    
    
output = ''

print('Fetching data....This might take a few minutes...')

toolbar_width = 83

# setup toolbar
sys.stdout.write("[%s]" % (" " * toolbar_width))
sys.stdout.flush()
sys.stdout.write("\b" * (toolbar_width+1)) # return to start of line, after '['

for command in commands:
    time.sleep(0.1) 
    output =  output + str(command) + '\n'
    proc = subprocess.Popen(shlex.split(command), text=True, stdout=subprocess.PIPE)
    (out,_) = proc.communicate()
    output = output + out + '\n'
    output = str(output)
# update the bar
    sys.stdout.write("-")
    sys.stdout.flush()
    
for interface in pattern_int:
    time.sleep(0.1) 
    int_command = '\n' + 'show interface counters detailed ' + str(interface) + '\n'
    output = '\n' + output + int_command + '\n'
    proc = subprocess.Popen(shlex.split(int_command), text=True, stdout=subprocess.PIPE)
    (out,_) = proc.communicate()
    output = output + out
    output = str(output)
# update the bar
    sys.stdout.write("-")
    sys.stdout.flush()
    
# end the progress bar
sys.stdout.write("\n") 

print('Generated a text file techsupport.txt in the tmp directory')
orig_stdout = sys.stdout
f = open('/tmp/techsupport.txt', 'w')
sys.stdout = f

print(output)

sys.stdout = orig_stdout
f.close()
