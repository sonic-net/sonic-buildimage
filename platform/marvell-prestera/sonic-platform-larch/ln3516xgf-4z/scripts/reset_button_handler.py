#!/usr/bin/env python3

import subprocess
import time
import sys

# ----------------------------- Configuration -----------------------------

# Replace with your actual register address (e.g., '0x12345678')
REGISTER_ADDRESS = '0xf2440150'

# Replace with the actual bit position you want to monitor (0-31)
BIT_POSITION = 28

# Number of consecutive polling cycles to confirm inactive state
CONFIRMATION_CYCLES = 3

# Polling intervals in seconds
POLL_INTERVAL_DEFAULT = 0.5
POLL_INTERVAL_FAST = 0.1

# Press duration thresholds in seconds
SHORT_PRESS_DURATION = 5
LONG_PRESS_DURATION = 10

# ---------------------------------------------------------------------------

def get_gpio_value(register, bit):
    """
    Reads the GPIO value from the specified register and bit position.

    Args:
        register (str): The memory address of the register (e.g., '0x12345678').
        bit (int): The bit position to read (0-31).

    Returns:
        int: The value of the specified GPIO bit (0 or 1).
    """
    try:
        # Execute the devmem command
        result = subprocess.run(['busybox', 'devmem', register],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                text=True,
                                check=True)
        output = result.stdout.strip()
        
        # Parse the hexadecimal output
        if output.startswith('0x') or output.startswith('0X'):
            value = int(output, 16)
        else:
            value = int(output)

        # Extract the specific bit
        gpio_bit = (value >> bit) & 1
        return gpio_bit

    except subprocess.CalledProcessError as e:
        #print(f"Error reading GPIO value: {e.stderr}", file=sys.stderr)
        return None
    except ValueError:
        #print(f"Unexpected output format: '{output}'", file=sys.stderr)
        return None

def handle_short_press(duration):
    
    result = subprocess.run(['reboot'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            text=True,
                            check=True)
    output = result.stdout.strip()

def handle_long_press(duration):
    """
    Handle a long press event.

    Args:
        duration (float): Duration of the press in seconds.
    """
    #print(f"Long press detected. Duration: {duration:.2f} seconds.")
    result = subprocess.run(['reset-factory'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                text=True,
                                check=True)
    output = result.stdout.strip()

def main():
    # State Definitions
    STATE_INACTIVE = 1
    STATE_ACTIVE = 0
    STATE_CONFIRMING_INACTIVE = 2

    state = STATE_INACTIVE
    poll_interval = POLL_INTERVAL_DEFAULT
    confirmation_counter = 0
    press_start_time = None

    #print("Starting GPIO polling...")
    #print(f"Monitoring REGISTER: {REGISTER_ADDRESS}, BIT: {BIT_POSITION}")
    #print("Press Ctrl+C to exit.")

    try:
        while True:
            gpio_value = get_gpio_value(REGISTER_ADDRESS, BIT_POSITION)
            #print(f"GPIO value: {gpio_value}")

            if gpio_value is None:
                # Failed to read GPIO value; skip this cycle
                time.sleep(poll_interval)
                continue

            if state == STATE_INACTIVE:
                if gpio_value == STATE_ACTIVE:
                    # GPIO became active
                    state = STATE_ACTIVE
                    poll_interval = POLL_INTERVAL_FAST
                    press_start_time = time.time()
                    #print("GPIO became ACTIVE. Switching to fast polling.")
            elif state == STATE_ACTIVE:
                if gpio_value == STATE_INACTIVE:
                    # Potentially inactive; start confirmation
                    state = STATE_CONFIRMING_INACTIVE
                    confirmation_counter = 1
                    #print("GPIO signal went INACTIVE. Confirming state...")
                else:
                    # Still active; reset confirmation counter if any
                    confirmation_counter = 0
            elif state == STATE_CONFIRMING_INACTIVE:
                if gpio_value == STATE_INACTIVE:
                    confirmation_counter += 1
                    if confirmation_counter >= CONFIRMATION_CYCLES:
                        # Confirmed inactive state
                        state = STATE_INACTIVE
                        poll_interval = POLL_INTERVAL_DEFAULT
                        press_duration = time.time() - press_start_time

                        if press_duration >= LONG_PRESS_DURATION:
                            handle_long_press(press_duration)
                            break
                        elif press_duration <= SHORT_PRESS_DURATION:
                            handle_short_press(press_duration)
                            break
                        else:
                            #print(f"Press duration {press_duration:.2f} seconds does not qualify as short or long press.")
                            pass

                        press_start_time = None
                        confirmation_counter = 0
                        #print("GPIO confirmed INACTIVE. Switching to default polling.")
                else:
                    # GPIO went back to active; cancel confirmation
                    state = STATE_ACTIVE
                    confirmation_counter = 0
                    #print("GPIO went back to ACTIVE during confirmation. Continuing fast polling.")

            # Sleep for the current polling interval
            time.sleep(poll_interval)

    except KeyboardInterrupt:
        pass
        #print("\nGPIO polling terminated by user.")

if __name__ == "__main__":
    main()
