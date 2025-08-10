BALDEAGLESDK_EXE="/usr/local/bin/BaldEagleSdk_v2_18.py"

EXE2=$BALDEAGLESDK_EXE
EXE4=$BALDEAGLESDK_EXE
EXE6=$BALDEAGLESDK_EXE
EXE8=$BALDEAGLESDK_EXE

# credo_auto2468.sh x x x x
#   x: type of PHY module, 1 = init, 0 = don't init
if [ $# -ne 4 ]; then
    echo "invalid parameter"
    exit 1
fi

for var in $1 $2 $3 $4
do
    if [ $var -lt 0 ] || [ $var -gt 1 ]; then
        echo "invalid parameter"
        exit 1
    fi
done

d1=$(date +"%s")
#long action here
if [ $1 -ne 0 ]; then
    python $EXE2 2 &
fi
if [ $2 -ne 0 ]; then
    python $EXE4 4 &
fi
if [ $3 -ne 0 ]; then
    python $EXE6 6 &
fi
if [ $4 -ne 0 ]; then
    python $EXE8 8 &
fi
wait

echo "All Process Done"
d2=$(date +"%s")
echo $((d2-d1))"sec"

