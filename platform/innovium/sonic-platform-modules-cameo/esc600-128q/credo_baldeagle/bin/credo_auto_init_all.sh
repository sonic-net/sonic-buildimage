d1=$(date +"%s")
#long action here
i2cset -y 0 0x30 0xa2 0x0
sleep 1
i2cset -y 0 0x30 0xa2 0xff
sleep 1
./credo_auto1357.sh
./credo_auto2468.sh
echo "All Slots Done"
d2=$(date +"%s")
echo $((d2-d1))"sec"
