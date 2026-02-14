import typing
from natsort import natsorted
import datetime
import pickle
import os
import copy
import json
import click
from tabulate import tabulate

import utilities_common.multi_asic as multi_asic_util
from swsscommon.swsscommon import CounterTable, MacsecCounter, SonicV2Connector
from utilities_common.cli import UserCache
from sonic_py_common import device_info

CACHE_MANAGER = UserCache(app_name="macsec")
CACHE_FILE = os.path.join(CACHE_MANAGER.get_directory(), "macsecstats{}")

DB_CONNECTOR = None
COUNTER_TABLE = None

class MACsecCfgMeta(object):
    def __init__(self, *args) -> None:
        SEPARATOR = DB_CONNECTOR.get_db_separator(DB_CONNECTOR.CONFIG_DB)
        self.key = self.__class__.get_cfg_table_name() + SEPARATOR + \
            SEPARATOR.join(args)
        self.cfgMeta = DB_CONNECTOR.get_all(
            DB_CONNECTOR.CONFIG_DB, self.key)
        if len(self.cfgMeta) == 0:
            raise ValueError("No such MACsecCfgMeta: {}".format(self.key))
        for k, v in self.cfgMeta.items():
            setattr(self, k, v)

class MACsecAppMeta(object):
    def __init__(self, *args) -> None:
        SEPARATOR = DB_CONNECTOR.get_db_separator(DB_CONNECTOR.APPL_DB)
        self.key = self.__class__.get_appl_table_name() + SEPARATOR + \
            SEPARATOR.join(args)
        self.meta = DB_CONNECTOR.get_all(
            DB_CONNECTOR.APPL_DB, self.key)
        if len(self.meta) == 0:
            raise ValueError("No such MACsecAppMeta: {}".format(self.key))
        for k, v in self.meta.items():
            setattr(self, k, v)


class MACsecCounters(object):
    def __init__(self, *args) -> None:
        _, fvs = COUNTER_TABLE.get(MacsecCounter(), ":".join(args))
        self.counters = dict(fvs)


class MACsecSA(MACsecAppMeta, MACsecCounters):
    def __init__(self, port_name: str, sci: str, an: str) -> None:
        self.port_name = port_name
        self.sci = sci
        self.an = an
        MACsecAppMeta.__init__(self, port_name, sci, an)
        MACsecCounters.__init__(self, port_name, sci, an)

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()
        meta = sorted(self.meta.items(), key=lambda x: x[0])
        counters = copy.deepcopy(self.counters)
        if cache:
            for k, v in counters.items():
                if k in cache.counters and k.startswith("SAI_MACSEC_SA_STAT"):
                    counters[k] = int(counters[k]) - int(cache.counters[k])
        counters = sorted(counters.items(), key=lambda x: x[0])
        buffer += tabulate(meta + counters)
        buffer = "\n".join(["\t\t" + line for line in buffer.splitlines()])
        return buffer


class MACsecIngressSA(MACsecSA):
    def __init__(self, port_name: str, sci: str, an: str) -> None:
        super(MACsecIngressSA, self).__init__(port_name, sci, an)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_INGRESS_SA_TABLE"

    def get_header(self):
        return "MACsec Ingress SA ({})\n".format(self.an)


class MACsecEgressSA(MACsecSA):
    def __init__(self, port_name: str, sci: str, an: str) -> None:
        super(MACsecEgressSA, self).__init__(port_name, sci, an)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_EGRESS_SA_TABLE"

    def get_header(self):
        return "MACsec Egress SA ({})\n".format(self.an)


class MACsecSC(MACsecAppMeta):
    def __init__(self, port_name: str, sci: str) -> None:
        self.port_name = port_name
        self.sci = sci
        super(MACsecSC, self).__init__(port_name, sci)


class MACsecIngressSC(MACsecSC):
    def __init__(self, port_name: str, sci: str) -> None:
        super(MACsecIngressSC, self).__init__(port_name, sci)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_INGRESS_SC_TABLE"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()
        buffer = "\n".join(["\t" + line for line in buffer.splitlines()])
        return buffer

    def get_header(self):
        return "MACsec Ingress SC ({})\n".format(self.sci)


class MACsecEgressSC(MACsecSC):
    def __init__(self, port_name: str, sci: str) -> None:
        super(MACsecEgressSC, self).__init__(port_name, sci)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_EGRESS_SC_TABLE"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()
        buffer += tabulate(sorted(self.meta.items(), key=lambda x: x[0]))
        buffer = "\n".join(["\t" + line for line in buffer.splitlines()])
        return buffer

    def get_header(self):
        return "MACsec Egress SC ({})\n".format(self.sci)


class MACsecPort(MACsecAppMeta, MACsecCfgMeta):
    def __init__(self,  port_name: str) -> None:
        self.port_name = port_name
        MACsecAppMeta.__init__(self, port_name)
        MACsecCfgMeta.__init__(self, port_name)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_PORT_TABLE"

    @classmethod
    def get_cfg_table_name(cls) -> str:
        return "PORT"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()

        # Add the profile information to the meta dict from config meta dict
        self.meta["profile"] = self.cfgMeta["macsec"]

        buffer += tabulate(sorted(self.meta.items(), key=lambda x: x[0]))
        return buffer

    def get_header(self) -> str:
        return "MACsec port({})\n".format(self.port_name)

class MACsecProfile(MACsecCfgMeta):
    def __init__(self, profile_name: str) -> None:
        self.profile_name = profile_name
        super(MACsecProfile, self).__init__(profile_name)

    @classmethod
    def get_cfg_table_name(cls) -> str:
        return "MACSEC_PROFILE"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()

        # Don't display the primary and fallback CAK
        if 'primary_cak' in self.cfgMeta: del self.cfgMeta['primary_cak']
        if 'fallback_cak' in self.cfgMeta: del self.cfgMeta['fallback_cak']

        t_buffer = tabulate(sorted(self.cfgMeta.items(), key=lambda x: x[0]))
        t_buffer = "\n".join(["\t" + line for line in t_buffer.splitlines()])
        buffer += t_buffer
        return buffer

    def get_header(self) -> str:
        return "MACsec profile : {}\n".format(self.profile_name)

def create_macsec_obj(key: str) -> MACsecAppMeta:
    attr = key.split(":")
    try:
        if attr[0] == MACsecPort.get_appl_table_name():
            return MACsecPort(attr[1])
        elif attr[0] == MACsecIngressSC.get_appl_table_name():
            return MACsecIngressSC(attr[1], attr[2])
        elif attr[0] == MACsecEgressSC.get_appl_table_name():
            return MACsecEgressSC(attr[1], attr[2])
        elif attr[0] == MACsecIngressSA.get_appl_table_name():
            return MACsecIngressSA(attr[1], attr[2], attr[3])
        elif attr[0] == MACsecEgressSA.get_appl_table_name():
            return MACsecEgressSA(attr[1], attr[2], attr[3])
        raise TypeError("Unknown MACsec object type")
    except ValueError as e:
        return None

def create_macsec_profile_obj(key: str) -> MACsecCfgMeta:
    attr = key.split("|")
    try:
        if attr[0] == MACsecProfile.get_cfg_table_name():
            return MACsecProfile(attr[1])
        raise TypeError("Unknown MACsec object type")
    except ValueError as e:
        return None

def create_macsec_objs(interface_name: str) -> typing.List[MACsecAppMeta]:
    objs = []
    objs.append(create_macsec_obj(MACsecPort.get_appl_table_name() + ":" + interface_name))
    egress_scs = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecEgressSC.get_appl_table_name() + ":" + interface_name + ":*")
    for sc_name in natsorted(egress_scs):
        sc = create_macsec_obj(sc_name)
        if sc is None:
            continue
        objs.append(sc)
        egress_sas = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecEgressSA.get_appl_table_name() + ":" + ":".join(sc_name.split(":")[1:]) + ":*")
        for sa_name in natsorted(egress_sas):
            sa = create_macsec_obj(sa_name)
            if sa is None:
                continue
            objs.append(sa)
    ingress_scs = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecIngressSC.get_appl_table_name() + ":" + interface_name + ":*")
    for sc_name in natsorted(ingress_scs):
        sc = create_macsec_obj(sc_name)
        if sc is None:
            continue
        objs.append(sc)
        ingress_sas = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecIngressSA.get_appl_table_name() + ":" + ":".join(sc_name.split(":")[1:]) + ":*")
        for sa_name in natsorted(ingress_sas):
            sa = create_macsec_obj(sa_name)
            if sa is None:
                continue
            objs.append(sa)
    return objs


def create_macsec_profiles_objs(profile_name: str) -> typing.List[MACsecCfgMeta]:
    objs = []
    objs.append(create_macsec_profile_obj(MACsecProfile.get_cfg_table_name() + "|" + profile_name))
    return objs


def cache_find(cache: dict, target: MACsecAppMeta) -> MACsecAppMeta:
    if not cache or not cache["objs"]:
        return None
    for obj in cache["objs"]:
        if type(obj) == type(target) and obj.key == target.key:
            # MACsec SA may be refreshed by a cycle that use the same key
            # So, use the SA as the identifier
            if isinstance(obj, MACsecSA) and obj.sak != target.sak:
                continue
            return obj
    return None


@click.command()
@click.argument('interface_name', required=False)
@click.option('--profile', is_flag=True, required=False, default=False, help="show all macsec profiles")
@click.option('--dump-file', is_flag=True, required=False, default=False, help="store show output to a file")
@click.option('--post-status', is_flag=True, required=False, default=False, help="show macsec FIPS POST(Pre-Operational Self-Test) status")
@click.option('--fips-module', is_flag=True, required=False, default=False, help="show macsec FIPS module")
@multi_asic_util.multi_asic_click_options
def macsec(interface_name, dump_file, namespace, display, profile, post_status, fips_module):
    if post_status:
        if interface_name is not None or profile or dump_file or fips_module:
            click.echo('POST status is not valid with other options/arguments')
            return
        MacsecContext(namespace, display).show_post_status()
        return
    if fips_module:
        if interface_name is not None or profile or dump_file or post_status:
            click.echo('fips-module is not valid with other options/arguments')
            return
        MacsecContext(namespace, display).show_fips_module()
        return
    if interface_name is not None and profile:
        click.echo('Interface name is not valid with profile option')
        return
    MacsecContext(namespace, display).show(interface_name, dump_file, profile)

class MacsecContext(object):

    def __init__(self, namespace_option, display_option):
        self.db = None
        self.multi_asic = multi_asic_util.MultiAsic(
            display_option, namespace_option)
        self.macsec_profiles = []

    @multi_asic_util.run_on_multi_asic
    def show(self, interface_name, dump_file, profile):
        global DB_CONNECTOR
        global COUNTER_TABLE
        DB_CONNECTOR = self.db

        if not profile:
            COUNTER_TABLE = CounterTable(self.db.get_redis_client(self.db.COUNTERS_DB))

            interface_names = [name.split(":")[1] for name in self.db.keys(self.db.APPL_DB, "MACSEC_PORT*")]
            if interface_name is not None:
                if interface_name not in interface_names:
                    return
                interface_names = [interface_name]
            objs = []

            for interface_name in natsorted(interface_names):
                objs += create_macsec_objs(interface_name)
        else:
            profile_names = [name.split("|")[1] for name in self.db.keys(self.db.CONFIG_DB, "MACSEC_PROFILE*")]
            objs = []

            for profile_name in natsorted(profile_names):
                # Check if this macsec profile is already added to profile list. This is in case of
                # multi-asic devices where all namespaces will have the same macsec profile defined.
                if profile_name not in self.macsec_profiles and not dump_file:
                    self.macsec_profiles.append(profile_name)
                    objs += create_macsec_profiles_objs(profile_name)

        cache = {}
        if os.path.isfile(CACHE_FILE.format(self.multi_asic.current_namespace)):
            cache = pickle.load(open(CACHE_FILE.format(self.multi_asic.current_namespace), "rb"))

        if not dump_file:
            if cache and cache["time"] and objs:
                print("Last cached time was {}".format(cache["time"]))
            for obj in objs:
                cache_obj = cache_find(cache, obj)
                print(obj.dump_str(cache_obj))
        else:
            dump_obj = {
                "time": datetime.datetime.now(),
                "objs": objs
            }
            with open(CACHE_FILE.format(self.multi_asic.current_namespace), 'wb') as dump_file:
                pickle.dump(dump_obj, dump_file)
                dump_file.flush()

    @multi_asic_util.run_on_multi_asic
    def show_post_status(self):
        """Show POST (Pre-Operational Self-Test) status"""
        # Define the table name
        table_name = "FIPS_MACSEC_POST_TABLE"

        def format_module_status(module, namespace):
            # Get all fields from the table
            post_data = state_db.get_all("STATE_DB", table_name+"|"+module)

            # Format according to SONiC CLI guidelines
            output = []
            indent = "  " if namespace else ""
            output.append(f"{indent}{'Module'.ljust(11)} : {module}")

            for field in post_data:
                value = post_data[field]
                # Format field name for consistent alignment (capitalize and pad to 11 chars)
                display_name = field.capitalize().ljust(11)
                output.append(f"{indent}{display_name} : {value}")

            return "\n".join(output)

        namespace = self.multi_asic.current_namespace
        # Connect to STATE_DB
        state_db = SonicV2Connector(use_unix_socket_path=True, namespace=namespace)
        state_db.connect(state_db.STATE_DB)

        # Get all keys in the FIPS_MACSEC_POST_TABLE
        all_keys = state_db.keys(state_db.STATE_DB, table_name + "|*")
        if not len(all_keys):
            click.echo("")  # Add blank line for separation
            if namespace:
                click.echo(f"Namespace ({namespace})")
                click.echo("  No entries found")
            else:
                click.echo("No entries found")
            return

        # Extract module names from the keys and sort them
        modules = []
        for key in all_keys:
            # Key format is "FIPS_MACSEC_POST_TABLE|module_name"
            if "|" in key:
                module = key.split("|", 1)[1]
                modules.append(module)

        # Sort modules for consistent output
        modules.sort()

        display_output = [""]
        if namespace:
            display_output.append(f"Namespace ({namespace})")
        for i, module in enumerate(modules):
            if i > 0:
                display_output.append("")  # Add separator between modules
            module_output = format_module_status(module, namespace)
            display_output.append(module_output)

        if display_output:
            click.echo("\n".join(display_output))

    def show_fips_module(self):
        json_file = device_info.get_path_to_platform_dir() + '/' + device_info.PLATFORM_JSON_FILE
        if not os.path.exists(json_file):
            return
        try:
            with open(json_file, 'r') as file:
                platform_data = json.load(file)
        except (json.JSONDecodeError, IOError, TypeError, ValueError):
            return
        if 'fips_module' in platform_data:
            click.echo(platform_data['fips_module'])

def register(cli):
    cli.add_command(macsec)


if __name__ == '__main__':
    macsec(None)
