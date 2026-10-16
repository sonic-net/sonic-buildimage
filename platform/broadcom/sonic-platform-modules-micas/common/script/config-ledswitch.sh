#!/bin/bash
###########################################################################
# Script Name: ledswitch.sh
# Description: Control LED switch monitoring and display current status
# Usage: ./ledswitch.sh -l                    # Show current LED status
#        ./ledswitch.sh -s {sdk|cpld}         # Set LED status (sdk or cpld)
#        ./ledswitch.sh -h                    # Show help information
###########################################################################

SCRIPT_DIR="/usr/share/sonic/platform"
SCRIPT_NAME="ledswitch_monitor.py"
SCRIPT_PATH="${SCRIPT_DIR}/${SCRIPT_NAME}"
PID_FILE="/var/run/ledswitch_monitor.pid"
LOG_FILE="/var/log/ledswitch_monitor.log"

# Logging function
log_info() {
    echo "[INFO] $(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a ${LOG_FILE}
}

log_error() {
    echo "[ERROR] $(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a ${LOG_FILE}
}

# Check if script exists
check_script() {
    if [ ! -f "${SCRIPT_PATH}" ]; then
        log_error "Script not found: ${SCRIPT_PATH}"
        return 1
    fi
    return 0
}

# Check if bcmcmd exists
check_bcmcmd() {
    if ! command -v bcmcmd &> /dev/null; then
        log_error "bcmcmd command not found"
        return 1
    fi
    return 0
}

# Start service (CPLD mode)
start_service() {
    if check_script; then
        # Check if already running
        if [ -f "${PID_FILE}" ]; then
            local pid=$(cat "${PID_FILE}")
            if kill -0 "${pid}" 2>/dev/null; then
                log_info "Service already running (PID: ${pid})"
                return 0
            else
                rm -f "${PID_FILE}"
            fi
        fi
        
        log_info "Starting LED switch monitoring service (CPLD mode)..."
        nohup python3 "${SCRIPT_PATH}" start >> "${LOG_FILE}" 2>&1 &
        local pid=$!
        echo "${pid}" > "${PID_FILE}"
        log_info "Service started (PID: ${pid})"
    fi
}

# Stop service (SDK mode)
stop_service() {
    if [ -f "${PID_FILE}" ]; then
        local pid=$(cat "${PID_FILE}")
        if kill -0 "${pid}" 2>/dev/null; then
            log_info "Stopping LED switch monitoring service (PID: ${pid})..."
            kill -TERM "${pid}"
            sleep 2
            if kill -0 "${pid}" 2>/dev/null; then
                log_error "Service did not stop gracefully, force killing..."
                kill -9 "${pid}"
            fi
            rm -f "${PID_FILE}"
            log_info "Service stopped"
        else
            log_info "Service not running"
            rm -f "${PID_FILE}"
        fi
    else
        # Try to find by process name
        local pid=$(pgrep -f "${SCRIPT_NAME}")
        if [ -n "${pid}" ]; then
            log_info "Found running service (PID: ${pid}), stopping..."
            kill -TERM ${pid}
            rm -f "${PID_FILE}"
            log_info "Service stopped"
        else
            log_info "Service not running"
        fi
    fi
}

# Check service status (returns 0 if running, 1 if not)
is_service_running() {
    if [ -f "${PID_FILE}" ]; then
        local pid=$(cat "${PID_FILE}")
        if kill -0 "${pid}" 2>/dev/null; then
            return 0
        else
            rm -f "${PID_FILE}"
            return 1
        fi
    else
        local pid=$(pgrep -f "${SCRIPT_NAME}")
        if [ -n "${pid}" ]; then
            echo "${pid}" > "${PID_FILE}"
            return 0
        else
            return 1
        fi
    fi
}

# Set CPLD mode (use Python monitoring service)
set_cpld_mode() {
    log_info "Setting LED mode to CPLD..."
    
    # Stop SDK mode first (stop bcmcmd led)
    if check_bcmcmd; then
        log_info "Stopping bcmcmd led..."
        bcmcmd "led stop" 2>/dev/null
        if [ $? -eq 0 ]; then
            log_info "bcmcmd led stop executed successfully"
        else
            log_error "Failed to execute bcmcmd led stop"
        fi
    fi
    
    # Start Python monitoring service
    start_service
    
    log_info "LED mode switched to CPLD"
}

# Set SDK mode (use bcmcmd led control)
set_sdk_mode() {
    log_info "Setting LED mode to SDK..."
    
    # Stop Python monitoring service first
    stop_service
    
    # Start bcmcmd led
    if check_bcmcmd; then
        log_info "Starting bcmcmd led..."
        bcmcmd "led start" 2>/dev/null
        if [ $? -eq 0 ]; then
            log_info "bcmcmd led start executed successfully"
        else
            log_error "Failed to execute bcmcmd led start"
        fi
    fi
    
    log_info "LED mode switched to SDK"
}

# Show current LED status
show_led_status() {
    echo ""
    echo "=========================================="
    echo "           LED Switch Status"
    echo "=========================================="
    
    if is_service_running; then
        echo "  Current Mode: CPLD"
        local pid=$(cat "${PID_FILE}" 2>/dev/null)
        if [ -n "${pid}" ]; then
            echo "  PID:           ${pid}"
        fi
    else
        echo "  Current Mode: SDK"
    fi
    
    echo "=========================================="
    echo ""
}

# Show help information
show_help() {
    cat << EOF
===========================================
        LED Switch Control Script
===========================================

USAGE:
    ledswitch.sh -l                    Show current LED status
    ledswitch.sh -s {sdk|cpld}         Set LED mode (sdk or cpld)
    ledswitch.sh -h                    Show this help message

DESCRIPTION:
    This script controls the LED switch monitoring service.
    
    - CPLD mode: Uses Python monitoring service to control LEDs based on
                 port status and breakout mode configuration.
    
    - SDK mode:  Uses bcmcmd "led start/stop" commands to let the SDK
                 handle LED control directly.

EXAMPLES:
    ledswitch.sh -l                    # Display current mode (CPLD or SDK)
    ledswitch.sh -s cpld               # Switch to CPLD mode
    ledswitch.sh -s sdk                # Switch to SDK mode
    ledswitch.sh -h                    # Display this help

LOG FILE:
    /var/log/ledswitch_monitor.log

===========================================
EOF
}

# Main function
main() {
    # Parse command line arguments
    while getopts "ls:h" opt; do
        case ${opt} in
            l)
                show_led_status
                exit 0
                ;;
            s)
                MODE="$OPTARG"
                case "${MODE}" in
                    sdk)
                        set_sdk_mode
                        ;;
                    cpld)
                        set_cpld_mode
                        ;;
                    *)
                        echo "Error: Invalid mode '$MODE'"
                        echo "Usage: ledswitch.sh -s {sdk|cpld}"
                        exit 1
                        ;;
                esac
                exit 0
                ;;
            h)
                show_help
                exit 0
                ;;
            \?)
                echo "Invalid option: -$OPTARG" >&2
                show_help
                exit 1
                ;;
            :)
                echo "Option -$OPTARG requires an argument." >&2
                exit 1
                ;;
        esac
    done
    
    # If no arguments provided, show help
    if [ $OPTIND -eq 1 ]; then
        show_help
        exit 0
    fi
}

# Run main function
main "$@"
