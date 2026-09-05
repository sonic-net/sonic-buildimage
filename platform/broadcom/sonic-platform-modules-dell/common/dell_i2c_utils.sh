# Perform an i2c device configuration : instantiate / delete.
# Input is of the form:
# "echo [driver] <i2c-address> >  <i2c-bus/operation>"
# where operation = "new_device" or "delete_device"

i2c_config() {
    local count=0
    local MAX_BUS_RETRY=20
    local MAX_I2C_OP_RETRY=10

    i2c_bus_op=`echo "$@" | cut -d'>' -f 2`
    i2c_bus=$(dirname $i2c_bus_op)

    # check if bus exists
    while [[ "$count" -lt "$MAX_BUS_RETRY" ]]; do
        [[ -e $i2c_bus ]] && break || sleep .1
        count=$((count+1))
    done

    if [[ "$count" -eq "$MAX_BUS_RETRY" ]]; then
        echo "dell_i2c_utils : ERROR: $@ : i2c bus not created"
        return
    fi

    # perform the add/delete
    count=0
    while [[ "$count" -lt "$MAX_I2C_OP_RETRY" ]]; do
        eval "$@" > /dev/null 2>&1
        [[ $? == 0 ]] && break || sleep .2
        count=$((count+1))
    done

    if [[ "$count" -eq "$MAX_I2C_OP_RETRY" ]]; then
        echo "dell_i2c_utils : ERROR: $@ : i2c operation failed"
        return
    fi
}

# Check if a i2c bus exists. Poll for upto 2 seconds since mux creation may take time..
# Input: bus to check for existence

i2c_poll_bus_exists() {
    local count=0
    local MAX_BUS_RETRY=20
    local i2c_bus

    i2c_bus=$1

    # check if bus exists
    while [[ "$count" -lt "$MAX_BUS_RETRY" ]]; do
        [[ -e $i2c_bus ]] && break || sleep .1
        count=$((count+1))
    done

    if [[ "$count" -eq "$MAX_BUS_RETRY" ]]; then
        echo "dell_i2c_utils : ERROR: $@ : i2c bus not created"
        return 1
    else
        return 0
    fi
}

# Perform an i2c mux device create
# Input is of the form:
# i2c_mux_create mux_driver i2c_addr i2c_bus_num i2c_child_bus_num_start
# where i2c_bus_num is the bus number in which the mux is to be created and
# i2c_child_bus_num_start is the first of the 8 bus channels that this mux should create
i2c_mux_create() {
    local MAX_MUX_CHANNEL_RETRY=3
    local MAX_MUX_CHANNELS=8
    local count=0
    local i
    local mux_driver=$1
    local i2c_addr=$2
    local i2c_bus_num=$3
    local i2c_child_bus_num_start=$4

    # Construct the i2c bus, the first and last bus channels that will be created under the MUX
    i2c_bus=/sys/bus/i2c/devices/i2c-$i2c_bus_num
    i2c_mux_channel_first=$i2c_bus/i2c-$i2c_child_bus_num_start
    i2c_mux_channel_last=$i2c_bus/i2c-$(expr $i2c_child_bus_num_start + $MAX_MUX_CHANNELS - 1)

    if  i2c_poll_bus_exists  $i2c_bus; then
        while [[ "$count" -lt "$MAX_MUX_CHANNEL_RETRY" ]]; do
            eval "echo $mux_driver $i2c_addr > /sys/bus/i2c/devices/i2c-$i2c_bus_num/new_device" > /dev/null 2>&1
            ret=$?

            # Give more time for the mux channels to get created based on retries
            i=0
            while [[ "$i" -lt "$count" ]]; do
                sleep 1
                i=$((i+1))
            done

            # Check if the (first and last) mux channels got created
            if [[ $ret -eq "0" && -e $i2c_mux_channel_first && -e $i2c_mux_channel_last ]]; then
                break;
            else
                # If the channel did not get created, remove the mux, reset the mux tree and retry
                echo "dell_i2c_utils :  ERROR: i2c mux channel not created for $mux_driver,$i2c_addr,$i2c_bus_num"
                i2c_mux_delete $i2c_addr $i2c_bus_num
                reset_muxes
            fi

            count=$((count+1))
        done
    fi

    if [[ "$count" -eq "$MAX_MUX_CHANNEL_RETRY" ]]; then
        echo "dell_i2c_utils : ERROR: $1,$2 : i2c mux channel not created"
        return
    fi

    return
}

# Perform an i2c mux device delete
# Input is of the form:
# i2c_mux_delete i2c_addr i2c_bus_num
i2c_mux_delete() {
    local i2c_addr
    local i2c_bus_num

    i2c_addr=$1
    i2c_bus_num=$2
    i2c_config "echo $i2c_addr > /sys/bus/i2c/devices/i2c-$i2c_bus_num/delete_device"
}
