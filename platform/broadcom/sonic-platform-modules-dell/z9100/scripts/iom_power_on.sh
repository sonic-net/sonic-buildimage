#!/bin/bash
# This script is used to power on IO modules
# IOM can be controlled via SMF using mailbox registers 

# write 0x1 to 0x04D9 to power up IOM 1
/usr/local/bin/io_rd_wr.py --set --val 0x04 --offset 0x210
/usr/local/bin/io_rd_wr.py --set --val 0xd9 --offset 0x211
/usr/local/bin/io_rd_wr.py --set --val 0x1 --offset 0x213
# write 0x1 to 0x04DA to power up IOM 2
/usr/local/bin/io_rd_wr.py --set --val 0xda --offset 0x211
/usr/local/bin/io_rd_wr.py --set --val 0x1 --offset 0x213
# write 0x1 to 0x04DB to power up IOM 3
/usr/local/bin/io_rd_wr.py --set --val 0xdb --offset 0x211
/usr/local/bin/io_rd_wr.py --set --val 0x1 --offset 0x213
# write 0x1 to 0x04DC to power up IOM 4
/usr/local/bin/io_rd_wr.py --set --val 0xdc --offset 0x211
/usr/local/bin/io_rd_wr.py --set --val 0x1 --offset 0x213
#Delay for SMF to power on IOMs
sleep 3
