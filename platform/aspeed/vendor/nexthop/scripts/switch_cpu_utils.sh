#!/bin/bash
# Switch CPU Utility Script for ASPEED AST2700 BMC
# Provides utilities to manage the switch CPU (x86) from BMC
#
# Usage: switch_cpu_utils.sh <command> [options]

set -e

# Hardware register addresses and values
RESET_REG_ADDR="0x14c0b208"
RESET_VALUE_OUT="3"      # Value to bring CPU out of reset
RESET_VALUE_IN="2"       # Value to put CPU into reset

# Script metadata
SCRIPT_NAME="$(basename "$0")"
LOG_TAG="switch-cpu-utils"

#######################################
# Print usage information
#######################################
usage() {
    cat << EOF
Usage: ${SCRIPT_NAME} <command> [options]

Switch CPU management utilities for ASPEED AST2700 BMC.

Commands:
    reset-out           Bring switch CPU out of reset
    reset-in            Put switch CPU into reset
    reset-cycle         Cycle switch CPU reset (in -> out)
    status              Show switch CPU reset status
    help                Show this help message

Options:
    -h, --help          Show this help message

Examples:
    ${SCRIPT_NAME} reset-out
    ${SCRIPT_NAME} reset-cycle
    ${SCRIPT_NAME} status

EOF
}

#######################################
# Bring switch CPU out of reset
#######################################
bring_switch_cpu_out_of_reset() {
    logger -t ${LOG_TAG} "Bringing switch CPU out of reset..."
    
    # Write to reset register to bring CPU out of reset
    if busybox devmem ${RESET_REG_ADDR} 32 ${RESET_VALUE_OUT} 2>/dev/null; then
        logger -t ${LOG_TAG} "Switch CPU successfully brought out of reset (wrote ${RESET_VALUE_OUT} to ${RESET_REG_ADDR})"
        echo "Switch CPU brought out of reset successfully"
        return 0
    else
        logger -t ${LOG_TAG} "ERROR: Failed to bring switch CPU out of reset"
        echo "ERROR: Failed to bring switch CPU out of reset" >&2
        return 1
    fi
}

#######################################
# Put switch CPU into reset
#######################################
put_switch_cpu_into_reset() {
    logger -t ${LOG_TAG} "Putting switch CPU into reset..."
    
    # Write to reset register to put CPU into reset
    if busybox devmem ${RESET_REG_ADDR} 32 ${RESET_VALUE_IN} 2>/dev/null; then
        logger -t ${LOG_TAG} "Switch CPU successfully put into reset (wrote ${RESET_VALUE_IN} to ${RESET_REG_ADDR})"
        echo "Switch CPU put into reset successfully"
        return 0
    else
        logger -t ${LOG_TAG} "ERROR: Failed to put switch CPU into reset"
        echo "ERROR: Failed to put switch CPU into reset" >&2
        return 1
    fi
}

#######################################
# Cycle switch CPU reset (in -> out)
#######################################
cycle_switch_cpu_reset() {
    logger -t ${LOG_TAG} "Cycling switch CPU reset..."
    echo "Cycling switch CPU reset (in -> out)..."
    
    # Put into reset
    if ! put_switch_cpu_into_reset; then
        return 1
    fi
    
    # Wait for reset to take effect
    echo "Waiting 2 seconds..."
    sleep 2
    
    # Bring out of reset
    if ! bring_switch_cpu_out_of_reset; then
        return 1
    fi
    
    logger -t ${LOG_TAG} "Switch CPU reset cycle completed successfully"
    echo "Switch CPU reset cycle completed successfully"
    return 0
}

#######################################
# Show switch CPU reset status
#######################################
show_switch_cpu_status() {
    logger -t ${LOG_TAG} "Reading switch CPU reset status..."
    
    # Read current value from reset register
    CURRENT_VALUE=$(busybox devmem ${RESET_REG_ADDR} 32 2>/dev/null)
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to read reset register at ${RESET_REG_ADDR}" >&2
        return 1
    fi
    
    echo "Switch CPU Reset Status:"
    echo "  Register Address: ${RESET_REG_ADDR}"
    echo "  Current Value:    ${CURRENT_VALUE}"
    
    # Interpret the value
    if [ "${CURRENT_VALUE}" = "${RESET_VALUE_OUT}" ] || [ "${CURRENT_VALUE}" = "0x00000003" ]; then
        echo "  Status:           OUT OF RESET (running)"
    elif [ "${CURRENT_VALUE}" = "${RESET_VALUE_IN}" ] || [ "${CURRENT_VALUE}" = "0x00000000" ]; then
        echo "  Status:           IN RESET (held in reset)"
    else
        echo "  Status:           UNKNOWN (value: ${CURRENT_VALUE})"
    fi
    
    return 0
}

#######################################
# Main function
#######################################
main() {
    # Handle no arguments
    if [ $# -eq 0 ]; then
        echo "ERROR: No command specified" >&2
        echo ""
        usage
        exit 1
    fi
    
    # Parse command
    COMMAND="$1"
    shift
    
    case "${COMMAND}" in
        reset-out)
            bring_switch_cpu_out_of_reset
            ;;
        reset-in)
            put_switch_cpu_into_reset
            ;;
        reset-cycle)
            cycle_switch_cpu_reset
            ;;
        status)
            show_switch_cpu_status
            ;;
        help|--help|-h)
            usage
            exit 0
            ;;
        *)
            echo "ERROR: Unknown command '${COMMAND}'" >&2
            echo ""
            usage
            exit 1
            ;;
    esac

    exit $?
}

# Run main function
main "$@"

