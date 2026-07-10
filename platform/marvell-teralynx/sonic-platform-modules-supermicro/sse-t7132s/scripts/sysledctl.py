#!/usr/bin/python3
import sys
import subprocess
import re
import sonic_platform.platform

def systemctl():
    log_path = sys.argv[0] + ".log"
    with open(log_path, 'w') as f:
        out = subprocess.run(['systemctl', 'list-jobs'], capture_output=True, text=True).stdout
        f.write(out)
        chassis = sonic_platform.platform.Platform().get_chassis()

        x = re.search("reboot.target[ ]+start", out)
        if x:
            f.write("starting reboot\n")
            chassis.set_status_led('green_blink')

        x = re.search("kexec.target[ ]+start", out)
        if x:
            f.write("starting kexec\n")
            chassis.set_status_led('green_blink')

        x = re.search("halt.target[ ]+start", out)
        if x:
            f.write("starting halt\n")
            chassis.set_status_led('red')

        x = re.search("poweroff.target[ ]+start", out)
        if x:
            f.write("starting poweroff\n")
            chassis.set_status_led('off')
            chassis.set_cpld2_s3(1)

        f.write("done\n")

def reboot():
    log_path = sys.argv[0] + ".log"
    with open(log_path, 'w') as f:
        f.write("fast/warm reboot\n")
        chassis = sonic_platform.platform.Platform().get_chassis()
        chassis.set_status_led('green_blink')
        f.write("done\n")

def main():
    if len(sys.argv)>=2 and sys.argv[1]=='start':
        systemctl()
    if len(sys.argv)>=2 and sys.argv[1]=='reboot':
        reboot()


if __name__ == '__main__':
    main()
