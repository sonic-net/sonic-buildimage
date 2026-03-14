from swsscommon.swsscommon import events_init_publisher, events_deinit_publisher, event_publish, FieldValueMap
import time
import sys
import ipaddress
import random
import argparse
import json
import logging

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers = [
        logging.FileHandler("debug.log"),
        logging.StreamHandler(sys.stdout)
    ]
)

def getTag(sourceTag):
    try:
        return sourceTag.split(":", 1)[1]
    except Exception as ex:
        logging.info("Unable to find : in <source>:tag\n")
        return sourceTag

def getFVMFromParams(params):
    param_dict = FieldValueMap()
    for key, value in params.items():
        key = str(key)
        value = str(value)
        param_dict[key] = value
    return param_dict

def publishEvents(line, publisher_handle):
    try:
        json_dict = json.loads(line)
    except Exception as ex:
        logging.error("JSON string not able to be parsed\n")
        return
    if not json_dict or len(json_dict) != 1:
        logging.error("JSON string not able to be parsed\n")
        return
    sourceTag = list(json_dict)[0]
    params = list(json_dict.values())[0]
    tag = getTag(sourceTag)
    param_dict = getFVMFromParams(params)
    if param_dict:
        event_publish(publisher_handle, tag, param_dict)

def publishEventsFromFile(publisher_handle, infile, count, pause):
    try:
        with open(infile, 'r') as f:
            for line in f.readlines():
                line.rstrip()
                publishEvents(line, publisher_handle)
                time.sleep(pause)
    except Exception as ex:
        logging.error("Unable to open file from given path or has incorrect json format, gives exception {}\n".format(ex))
        logging.info("Switching to default bgp state publish events\n")
        publishBGPEvents(publisher_handle, count, pause)

def publishBGPEvents(publisher_handle, count, pause):
    ip_addresses = []
    param_dict = FieldValueMap()

    for _ in range(count):
        ip = str(ipaddress.IPv4Address(random.randint(0, 2 ** 32)))
        ip_addresses.append(ip)

    # publish down events
    for ip in ip_addresses:
        param_dict["ip"] = ip
        param_dict["status"] = "down"
        event_publish(publisher_handle, "bgp-state", param_dict)
        time.sleep(pause)

    # publish up events
    for ip in ip_addresses:
        param_dict["ip"] = ip
        event_publish(publisher_handle, "bgp-state", param_dict)
        time.sleep(pause)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--source", nargs='?', const='test-event-source', default='test-event-source', help="Source of event, default us test-event-source")
    parser.add_argument("-f", "--file", nargs='?', const='', default='', help="File containing json event strings, must be in format \'{\"<source>:foo\": {\"aaa\": \"AAA\", \"bbb\": \"BBB\"}}\'")
    parser.add_argument("-c", "--count", nargs='?', type=int, const=10, default=10, help="Count of default bgp events to be generated")
    parser.add_argument("-p", "--pause", nargs='?', type=float, const=0.0, default=0.0, help="Pause time wanted between each event, default is 0")
    args = parser.parse_args()
    publisher_handle = events_init_publisher(args.source)
    if args.file == '':
        publishBGPEvents(publisher_handle, args.count, args.pause)
    else:
        publishEventsFromFile(publisher_handle, args.file, args.count, args.pause)
    events_deinit_publisher(publisher_handle)

if __name__ == "__main__":
    main()
