import sys
import codecs
from urllib.parse import quote

from sonic_platform.platform_thrift_client import thrift_try

def platform_sensors_get(args):
    options = ""
    if len(args)!=0:
        options = quote(" ".join(args))
    def get_data(client):
        return client.pltfm_mgr.pltfm_mgr_sensor_info_get(options)
    raw_out = thrift_try(get_data)
    raw_list = raw_out.split('\"')
    if len(raw_list) >= 2:
        sensors_out = raw_list[1]
        sensors_out = codecs.decode(sensors_out, "unicode_escape")
        return sensors_out
    return None

if __name__ == '__main__':
    data = platform_sensors_get(sys.argv[1:])
    if data:
        print(data)
    else:
        print("No sensors info available", file=sys.stderr)
