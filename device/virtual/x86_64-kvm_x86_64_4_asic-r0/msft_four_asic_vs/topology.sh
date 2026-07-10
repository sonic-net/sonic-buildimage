#!/bin/bash
# Topolgy script for 4 ASIC PLATFORM
# 2 frontend asic , 2 backend asic.
# 8 front-panel interfaces.
FIRST_FRONTEND_ASIC=0
LAST_FRONTEND_ASIC=1
FIRST_BACKEND_ASIC=2
LAST_BACKEND_ASIC=3
NUM_INTERFACES_PER_ASIC=8

start () {
    # Move external links into assigned frontend namespaces
    # eth0  - eth15: asic2
    # eth16 - eth31: asic3
    # eth32 - eth47: asic4
    # eth48 - eth63: asic5
    for ASIC in `seq $FIRST_FRONTEND_ASIC $LAST_FRONTEND_ASIC`; do
        for NUM in `seq 1 4`; do
            ORIG="eth$((4 * $ASIC + $NUM))"
            TEMP="ethTemp999"
            NEW="eth$(($NUM))"
            echo "$ASIC : $NEW old $ORIG"
            ip link set dev $ORIG down
            ip link set dev $ORIG name $TEMP # rename to prevent conflicts before renaming in new namespace
            ip link set dev $TEMP netns asic$ASIC
            sudo ip netns exec asic$ASIC ip link set $TEMP name $NEW # rename too final interface name
            sudo ip netns exec asic$ASIC ip link set dev $NEW mtu 9100
            sudo ip netns exec asic$ASIC ip link set $NEW up
        done
    done

    # Connect all backend namespaces to frontend namespaces
    for BACKEND in `seq $FIRST_BACKEND_ASIC $LAST_BACKEND_ASIC`; do
        for FRONTEND in `seq $FIRST_FRONTEND_ASIC $LAST_FRONTEND_ASIC`; do
            for LINK in `seq 1 2`; do
                FRONT_NAME="eth$((2 * $(($BACKEND - $FIRST_BACKEND_ASIC)) + $LINK + 4))"
                BACK_NAME="eth$((2 * $FRONTEND + $LINK))"
                echo "$FRONTEND:$FRONT_NAME - $BACKEND:$BACK_NAME"
                TEMP_BACK="ethBack999"
                TEMP_FRONT="ethFront999"
                ip link add $TEMP_BACK type veth peer name $TEMP_FRONT # temporary name to prevent conflicts between interfaces
                ip link set dev $TEMP_BACK netns asic$BACKEND
                ip link set dev $TEMP_FRONT netns asic$FRONTEND

                sudo ip netns exec asic$BACKEND ip link set $TEMP_BACK name $BACK_NAME
                sudo ip netns exec asic$FRONTEND ip link set $TEMP_FRONT name $FRONT_NAME

                sudo ip netns exec asic$BACKEND ip link set dev $BACK_NAME mtu 9100
                sudo ip netns exec asic$BACKEND ip link set $BACK_NAME up
                sudo ip netns exec asic$FRONTEND ip link set dev $FRONT_NAME mtu 9100
                sudo ip netns exec asic$FRONTEND ip link set $FRONT_NAME up
            done
        done
    done
}
stop() {
    for ASIC in `seq $FIRST_FRONTEND_ASIC $LAST_FRONTEND_ASIC`; do
        for NUM in `seq 1 4`; do
            TEMP="eth999"
            OLD="eth$(($NUM))"
            NAME="eth$((4 * $ASIC + $NUM))"
            sudo ip netns exec asic$ASIC ip link set dev $OLD down
            sudo ip netns exec asic$ASIC ip link set dev $OLD name $TEMP
            sudo ip netns exec asic$ASIC ip link set dev $TEMP netns 1
            ip link set dev $TEMP name $NAME
            ip link set dev $NAME up
        done
    done

    for ASIC in `seq $FIRST_BACKEND_ASIC $LAST_BACKEND_ASIC`; do
        for NUM in `seq 1 4`; do
            sudo ip netns exec asic$ASIC ip link set dev eth$NUM down
            sudo ip netns exec asic$ASIC ip link delete dev eth$NUM
        done
    done
}

case "$1" in
    start|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        ;;
esac
