from netifaces import ifaddresses, AF_INET6
from subprocess import Popen, PIPE, DEVNULL
import json
import os

class Metric(object):

    def __init__(self, sensor_id, sensor_key, value, label):
        self._value = self.parse_value(value)
        self._sensor_id = sensor_id
        self._sensor_key = sensor_key
        self._label = label

    @classmethod
    def parse_value(cls, value):
        parse = getattr(cls, "parse")
        return parse(value)

    # For debug purposes
    def __repr__(self):
        return "%s, %s: %s %s [%s]" % (
            self._sensor_id,
            self._sensor_key,
            self._value,
            getattr(self, "unit", "?"),
            self._label)

class Temperature(Metric):
    parse = float
    unit = "°C"

class FanRpm(Metric):
    parse = float
    unit = "RPM"

class FanFault(Metric):
    parse = float

class Voltage(Metric):
    parse = float
    unit = "V"

class Power(Metric):
    parse = float
    unit = "W"

class Current(Metric):
    parse = float
    unit = "A"

def get_metric_value(metrics, name):
    label, sensor_id, sensor_key = name.split("_")
    for metric in metrics:
        if metric._label == label and metric._sensor_id == sensor_id and metric._sensor_key == sensor_key:
            return metric._value
    return None

def get_link_local_interface():
    cdc_ether_path = "/sys/bus/usb/drivers/cdc_ether"
    for ether in os.listdir(cdc_ether_path):
        concrete_ether = os.path.join(cdc_ether_path, ether)
        if os.path.isdir(concrete_ether):
            concrete_ether_net = os.path.join(concrete_ether, 'net')
            if os.path.exists(concrete_ether_net):
                return os.listdir(concrete_ether_net)[0]

def get_link_local_address(link_local_interface):
    for addr in ifaddresses(link_local_interface)[AF_INET6]:
        address = addr['addr'].split('%')[0]
        # according to rfc4291 this ipv6 address is used for link local connection
        if address.startswith('fe80:'):
            # first address is taken for BMC and second for this host
            return address[:-1] + '1'
    return None

def get_psu_metrics():
    link_local_interface = get_link_local_interface()
    link_local_address = get_link_local_address(link_local_interface)

    http_address = "http://[%s%%%s]:8080" % (link_local_address, link_local_interface)
    args = "/api/sys/bmc/sensors/%20-A%20-u%20"
    cmd = "curl " + http_address + args
    output = Popen(cmd.split(), stdout=PIPE, stderr=DEVNULL).stdout.read()
    output = json.loads(output.decode())["Information"]["Description"][0].strip()
    sections = output.split("\n\n")

    metrics = []
    # iterating through drivers and their sensors
    for section in sections:
        fields = section.split("\n")

        label = None
        # iterating through sensors and their inputs
        for field in fields[1:]: # skipping driver name
            # parsing input sensor
            if field.startswith("  "):
                field = field.replace("  ", "")
                # split sensor into name and value
                field_key, field_value = field.split(": ")
                if "_" in field_key:
                    sensor_id, sensor_key = field_key.split("_", 1)
                    if sensor_key == "input":
                        if sensor_id.startswith("temp"):
                            metrics.append(
                                Temperature(sensor_id, sensor_key, field_value, label=label))
                        elif sensor_id.startswith("in"):
                            metrics.append(
                                Voltage(sensor_id, sensor_key, field_value, label=label))
                        elif sensor_id.startswith("power"):
                            metrics.append(
                                Power(sensor_id, sensor_key, field_value, label=label))
                        elif sensor_id.startswith("curr"):
                            metrics.append(
                                Current(sensor_id, sensor_key, field_value, label=label))
                        elif sensor_id.startswith("fan"):
                            metrics.append(
                                FanRpm(sensor_id, sensor_key, field_value, label=label))
                    elif sensor_key == "fault":
                        if sensor_id.startswith("fan"):
                            metrics.append(
                                FanFault(sensor_id, sensor_key, field_value, label=label))
            elif field.startswith("ERROR"):
                syslog.syslog(syslog.LOG_INFO, field)
            else:
                label = field[:-1]  # strip off trailing ":" character

    return metrics
