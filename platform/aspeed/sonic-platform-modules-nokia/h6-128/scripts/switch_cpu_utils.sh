#!/bin/bash
# Switch CPU Utility Script for ASPEED AST2700 BMC
# Provides utilities to manage the switch CPU (x86) from BMC
# Uses the SwitchHostModule Python platform API
#
# Usage: switch_cpu_utils.sh <command> [options]

set -e

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
This script uses the SwitchHostModule platform API.

Commands:
    power-on            Power on switch CPU (bring out of reset)
    power-off           Power off switch CPU (put into reset)
    power-cycle         Power cycle switch CPU
    reset               Power cycle switch CPU
    status              Show switch CPU operational status
    help                Show this help message

Options:
    -h, --help          Show this help message

Examples:
    ${SCRIPT_NAME} power-on
    ${SCRIPT_NAME} power-off
    ${SCRIPT_NAME} power-cycle
    ${SCRIPT_NAME} reset
    ${SCRIPT_NAME} status

EOF
}

#######################################
# Execute Python platform API command
#######################################
execute_python_command() {
    local command="$1"
    
    python3 << EOF
import sys

try:
    from sonic_platform.switch_host_module import SwitchHostModule
    
    # Create module instance
    module = SwitchHostModule(module_index=0)

    # Assign the Bash variable to a Python variable safely
    cmd = "${command}"
    
    # Execute command
    if cmd == "power-on":
        result = module.set_admin_state(True)
        if result:
            print("Switch CPU powered on successfully")
            sys.exit(0)
        else:
            print("ERROR: Failed to power on switch CPU", file=sys.stderr)
            sys.exit(1)
    
    elif cmd == "power-off":
        result = module.set_admin_state(False)
        if result:
            print("Switch CPU powered off successfully")
            sys.exit(0)
        else:
            print("ERROR: Failed to power off switch CPU", file=sys.stderr)
            sys.exit(1)
    
    elif cmd == "power-cycle":
        result = module.do_power_cycle()
        if result:
            print("Switch CPU power cycle completed successfully")
            sys.exit(0)
        else:
            print("ERROR: Failed to power cycle switch CPU", file=sys.stderr)
            sys.exit(1)
    
    elif cmd == "status":
        status = module.get_oper_status()
        name = module.get_name()
        desc = module.get_description()
        
        print("Switch CPU Status:")
        print(f"  Name:             {name}")
        print(f"  Description:      {desc}")
        print(f"  Operational State: {status}")
        
        # Additional human-readable interpretation
        if status == module.MODULE_STATUS_ONLINE:
            print("  Interpretation:   CPU is ONLINE (OUT OF RESET)")
        elif status == module.MODULE_STATUS_OFFLINE:
            print("  Interpretation:   CPU is OFFLINE (held in reset)")
        elif status == module.MODULE_STATUS_FAULT:
            print("  Interpretation:   ERROR reading CPU status")
        else:
            print(f"  Interpretation:   Unknown status ({status})")
        
        sys.exit(0)
    
    else:
        print(f"ERROR: Unknown command '{cmd}'", file=sys.stderr)
        sys.exit(1)

except ImportError as e:
    print(f"ERROR: Failed to import SwitchHostModule: {e}", file=sys.stderr)
    print("Make sure the platform module is installed correctly", file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f"ERROR: {e}", file=sys.stderr)
    sys.exit(1)
EOF
    
    return $?
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
        power-on)
            logger -t ${LOG_TAG} "Executing power-on command via SwitchHostModule"
            execute_python_command "power-on"
            ;;
        power-off)
            logger -t ${LOG_TAG} "Executing power-off command via SwitchHostModule"
            execute_python_command "power-off"
            ;;
        power-cycle | reset)
            logger -t ${LOG_TAG} "Executing power-cycle command via SwitchHostModule"
            execute_python_command "power-cycle"
            ;;
        status)
            execute_python_command "status"
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
