"""SONiC LED plugin to control LED on/off/blinking when link-up/link-down/flowing-traffic
"""
def platform_configure_led(isg, lane, link_status, detailed_speed, activity_status):
    """ Configure LED to turn on/off/blink based on link-up/link-down/activity-status
    Args:
        isg, lane, link_status, detailed_speed, activity_status
    Returns:
        cmd
    Raises:
        none 
    """
    base_cmd = 'i2cset -f -y 0 0x38'
    isg_to_bitset_map = {
            'ISG0':'0',
            'ISG1':'1',
            'ISG2':'2',
            'ISG3':'3',
            'ISG4':'4',
            'ISG5':'5',
            'ISG6':'6',
            'ISG7':'7',
            'ISG8':'8',
            'ISG9':'9',
            'ISG10':'a',
            'ISG11':'b',
            'ISG12':'c',
            'ISG13':'d',
            'ISG14':'e',
            'ISG15':'f'
    }
    detailed_speed_to_bitset_map = {
            '400G/50G':'0',
            '200G/50G':'1',
            '200G/25G':'2',
            '100G/50G':'3',
            '50G/50G':'4',
            '100G/25G':'5',
            '50G/25G':'6',
            '25G/25G':'7',
            '40G/10G':'8',
            '10G/10G':'9'
    }

    try:
        if link_status == 'D':
            arg = '0x' + isg_to_bitset_map[isg] + str(lane)
            cmd = base_cmd + ' ' + arg + ' 0x00'
        elif link_status == 'U':
            arg1 = '0x' + isg_to_bitset_map[isg] + str(lane)
            arg2 = '0x' + str(1+(2*activity_status)) + detailed_speed_to_bitset_map[detailed_speed] 
            cmd = base_cmd + ' ' + arg1 + ' ' + arg2

        return cmd

    except Exception:
        print('Error when processing command: {}'.format(cmd))
        raise

def platform_configure_switch_led_control_from_TL5_to_CPU():
    """ Configure switch LED stream from TL5 to CPU through i2c
    Args:
        none
    Returns:
        cmd
    Raises:
        none 
    """
    cmd = 'i2cset -f -y 0 0x30 0xa0 0xf'
    return cmd

def main():
    """ Main function.
    Args:
        none
    Returns:
        none
    Raises:
        none
    """
    return


if __name__ == '__main__':
    main()
