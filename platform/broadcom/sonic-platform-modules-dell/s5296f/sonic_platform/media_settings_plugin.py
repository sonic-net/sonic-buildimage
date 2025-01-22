# Media settings key plugin
#
# Generate keys used for lookup in media_settings,json

def get_media_settings_key(physical_port, transceiver_dict):
    d = transceiver_dict[physical_port]
    media_interface = d['media_interface']
    generic_key = '{}-{}'.format(d['form_factor'], media_interface)
    if media_interface == 'CR':
        generic_key = '{}-{}'.format(generic_key, d['cable_length_detailed'])
    return ['{}-{}'.format(d['manufacturename'], d['modelname']),
            generic_key
           ]
