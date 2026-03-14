#!/bin/bash
#topolgy script for 6 ASIC PLATFORM
FIRST_FRONTEND_ASIC=0
LAST_FRONTEND_ASIC=3
FIRST_BACKEND_ASIC=4
LAST_BACKEND_ASIC=5
NUM_INTERFACES_PER_ASIC=32

start () {
    # Move external links into assigned frontend namespaces
    # eth1  - eth16: asic0
    # eth17 - eth32: asic1 
    # eth33 - eth48: asic2
    # eth49 - eth64: asic3
    for ASIC in `seq $FIRST_FRONTEND_ASIC $LAST_FRONTEND_ASIC`; do
        for NUM in `seq 1 16`; do
            ORIG="eth$((16 * $ASIC + $NUM))"
            TEMP="ethTemp999"
            NEW="eth$(($NUM))"
            echo "$ASIC : $NEW old $ORIG"
            ip link set dev $ORIG down
            ip link set dev $ORIG name $TEMP # rename to prevent conflicts before renaming in new namespace
            ip link set dev $TEMP netns asic$ASIC
            sudo ip netns exec asic$ASIC ip link set $TEMP name $NEW # rename to final interface name
            sudo ip netns exec asic$ASIC ip link set dev $NEW mtu 9100
            sudo ip netns exec asic$ASIC ip link set $NEW up 
        done
    done

    # Connect all backend namespaces to frontend namespaces
    for BACKEND in `seq $FIRST_BACKEND_ASIC $LAST_BACKEND_ASIC`; do
        for FRONTEND in `seq $FIRST_FRONTEND_ASIC $LAST_FRONTEND_ASIC`; do
            for LINK in `seq 1 8`; do
		FRONT_NAME="eth$((8 * $(($BACKEND - $FIRST_BACKEND_ASIC)) + $LINK + 16))"
		BACK_NAME="eth$((8 * $FRONTEND + $LINK))"
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
        for NUM in `seq 1 16`; do
            TEMP="eth999"
            OLD="eth$(($NUM))"
            NAME="eth$((16 * $ASIC + $NUM))"
            sudo ip netns exec asic$ASIC ip link set dev $OLD down
            sudo ip netns exec asic$ASIC ip link set dev $OLD name $TEMP
            sudo ip netns exec asic$ASIC ip link set dev $TEMP netns 1
            ip link set dev $TEMP name $NAME
            ip link set dev $NAME up
        done
    done

    for ASIC in `seq $FIRST_BACKEND_ASIC $LAST_BACKEND_ASIC`; do
        for NUM in `seq 1 $NUM_INTERFACES_PER_ASIC`; do
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
