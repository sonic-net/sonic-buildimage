import sys

from sonic_platform.platform_thrift_client import thrift_try

_MAX_FAN = 10

def fan_speed_set(fan, percent):
    def set_fan_speed(client):
        return client.pltfm_mgr.pltfm_mgr_fan_speed_set(fan, percent)
    return thrift_try(set_fan_speed)

def fan_speed_info_get():
    for fan_num in range(1, _MAX_FAN + 1):
        def get_data(client, fan_num=fan_num):
            return client.pltfm_mgr.pltfm_mgr_fan_info_get(fan_num)
        fan_info = thrift_try(get_data)
        if fan_info.fan_num == fan_num:
            yield fan_info

if __name__ == '__main__':
    def print_usage():
        print("Usage: platform_fancontrol.py <function> <param list>  ", file=sys.stderr)
        print("           function: fan_speed_set <fan #> <percent>   ", file=sys.stderr)
        print("                     fan_speed_info_get                ", file=sys.stderr)

    argc = len(sys.argv)
    if argc == 1:
      print_usage()
      exit(0)

    if sys.argv[1] == "fan_speed_set":
      if argc != 4:
        print_usage()
        exit(0)

      fan = int(sys.argv[2])
      percent = int(sys.argv[3])

      if (fan > _MAX_FAN) | (fan < 0):
        print("Invalid value for fan #.\n", file=sys.stderr)
        print_usage()
        exit(0)

      if (percent > 100) | (percent < 0):
        print("Invalid value for precent\n", file=sys.stderr)
        print_usage()
        exit(0)

      fan_speed_set(fan, percent)
      exit(0)

    if sys.argv[1] == "fan_speed_info_get":
      for fan_info in fan_speed_info_get():
        print("fan number: %d front rpm: %d rear rpm: %d percent: %d%% " %
            (fan_info.fan_num, fan_info.front_rpm, fan_info.rear_rpm, fan_info.percent))

      exit(0)

    print_usage()
