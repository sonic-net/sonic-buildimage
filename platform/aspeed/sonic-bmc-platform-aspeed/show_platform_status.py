#!/usr/bin/env python3
"""
Simple script to display platform status (fans and thermals)
Mimics SONiC 'show platform' commands for testing
"""

import sys
from sonic_platform.chassis import Chassis

def show_fans():
    """Display fan status in table format"""
    chassis = Chassis()
    num_fans = chassis.get_num_fans()
    
    print("\n" + "="*80)
    print("FAN STATUS")
    print("="*80)
    print(f"{'Fan':<15} {'Speed (RPM)':<15} {'Speed (%)':<15} {'Status':<15}")
    print("-"*80)
    
    for i in range(num_fans):
        fan = chassis.get_fan(i)
        name = fan.get_name()
        rpm = fan.get_speed()
        percentage = fan.get_speed_percentage()
        status = fan.get_status()
        
        status_str = "OK" if status else "Not OK"
        print(f"{name:<15} {rpm:<15} {percentage:<15.1f} {status_str:<15}")
    
    print("="*80)
    print(f"Total fans: {num_fans}")
    print()

def show_thermals():
    """Display thermal sensor status in table format"""
    chassis = Chassis()
    num_thermals = chassis.get_num_thermals()
    
    print("\n" + "="*100)
    print("THERMAL SENSOR STATUS")
    print("="*100)
    print(f"{'Sensor':<20} {'Temperature':<15} {'High TH':<15} {'Crit High TH':<15} {'Status':<15}")
    print("-"*100)
    
    for i in range(num_thermals):
        thermal = chassis.get_thermal(i)
        name = thermal.get_name()
        temp = thermal.get_temperature()
        high_th = thermal.get_high_threshold()
        crit_high_th = thermal.get_high_critical_threshold()
        status = thermal.get_status()
        
        status_str = "OK" if status else "Not OK"
        temp_str = f"{temp:.1f}°C" if temp is not None else "N/A"
        high_str = f"{high_th:.1f}°C" if high_th is not None else "N/A"
        crit_str = f"{crit_high_th:.1f}°C" if crit_high_th is not None else "N/A"
        
        print(f"{name:<20} {temp_str:<15} {high_str:<15} {crit_str:<15} {status_str:<15}")
    
    print("="*100)
    print(f"Total thermal sensors: {num_thermals}")
    print()

def show_summary():
    """Display platform summary"""
    chassis = Chassis()
    
    print("\n" + "="*80)
    print("PLATFORM SUMMARY")
    print("="*80)
    print(f"Platform: {chassis.get_name()}")
    print(f"Model: {chassis.get_model()}")
    print(f"Serial: {chassis.get_serial()}")
    print(f"Fans: {chassis.get_num_fans()}")
    print(f"Thermal Sensors: {chassis.get_num_thermals()}")
    print("="*80)
    print()

def main():
    """Main function"""
    if len(sys.argv) > 1:
        cmd = sys.argv[1].lower()
        if cmd == "fan" or cmd == "fans":
            show_fans()
        elif cmd == "thermal" or cmd == "temperature":
            show_thermals()
        elif cmd == "summary":
            show_summary()
        else:
            print(f"Unknown command: {cmd}")
            print("Usage: show_platform_status.py [fan|thermal|summary|all]")
            sys.exit(1)
    else:
        # Show all by default
        show_summary()
        show_fans()
        show_thermals()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

