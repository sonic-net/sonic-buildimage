import click
import utilities_common.cli as clicommon
from sonic_py_common import multi_asic
from swsscommon.swsscommon import ConfigDBConnector
from utilities_common.constants import DEFAULT_NAMESPACE
from utilities_common.db import Db

#
# 'macsec' group ('config macsec ...')
#
@click.group(cls=clicommon.AbbreviationGroup, name='macsec')
# TODO add "hidden=True if this is a single ASIC platform, once we have click 7.0 in all branches.
@click.option('-n', '--namespace', help='Namespace name',
             required=True if multi_asic.is_multi_asic() else False, type=click.Choice(multi_asic.get_namespace_list()))
@click.pass_context
def macsec(ctx, namespace):
    """MACsec-related configuration tasks"""
    if not ctx.obj or isinstance(ctx.obj, Db):
        # Set namespace to default_namespace if it is None.
        if namespace is None:
            namespace = DEFAULT_NAMESPACE
        config_db = ConfigDBConnector(use_unix_socket_path=True, namespace=str(namespace))
        config_db.connect()
        ctx.obj = config_db


#
# 'port' group ('config macsec port ...')
#
@macsec.group(cls=clicommon.AbbreviationGroup, name='port')
def macsec_port():
    """Enable MACsec or disable MACsec on the specified port"""
    pass

#
# 'add' command ('config macsec port add ...')
#
@macsec_port.command('add')
@click.argument('port', metavar='<port_name>', required=True)
@click.argument('profile', metavar='<profile_name>', required=True)
def add_port(port, profile):
    """
    Add MACsec port
    """
    ctx = click.get_current_context()
    config_db = ctx.obj

    if clicommon.get_interface_naming_mode() == "alias":
        port = interface_alias_to_name(config_db, port)
        if port is None:
            ctx.fail("cannot find port name for alias {}".format(port))

    profile_entry = config_db.get_entry('MACSEC_PROFILE', profile)
    if len(profile_entry) == 0:
        ctx.fail("profile {} doesn't exist".format(profile))

    port_entry = config_db.get_entry('PORT', port)
    if len(port_entry) == 0:
        ctx.fail("port {} doesn't exist".format(port))

    port_entry['macsec'] = profile

    config_db.set_entry("PORT", port, port_entry)


#
# 'del' command ('config macsec port del ...')
#
@macsec_port.command('del')
@click.argument('port', metavar='<port_name>', required=True)
def del_port(port):
    """
    Delete MACsec port
    """
    ctx = click.get_current_context()
    config_db = ctx.obj

    if clicommon.get_interface_naming_mode() == "alias":
        port = interface_alias_to_name(config_db, port)
        if port is None:
            ctx.fail("cannot find port name for alias {}".format(port))

    port_entry = config_db.get_entry('PORT', port)
    if len(port_entry) == 0:
        ctx.fail("port {} doesn't exist".format(port))

    del port_entry['macsec']

    config_db.set_entry("PORT", port, port_entry)


#
# 'profile' group ('config macsec profile ...')
#
@macsec.group(cls=clicommon.AbbreviationGroup, name='profile')
def macsec_profile():
    pass


def is_hexstring(hexstring: str):
    try:
        int(hexstring, 16)
        return True
    except ValueError:
        return False


#
# 'add' command ('config macsec profile add ...')
#
@macsec_profile.command('add')
@click.argument('profile', metavar='<profile_name>', required=True)
@click.option('--priority', metavar='<priority>', required=False, default=255, show_default=True, type=click.IntRange(0, 255), help="For Key server election. In 0-255 range with 0 being the highest priority.")
@click.option('--cipher_suite', metavar='<cipher_suite>', required=False, default="GCM-AES-128", show_default=True, type=click.Choice(["GCM-AES-128", "GCM-AES-256", "GCM-AES-XPN-128", "GCM-AES-XPN-256"]), help="The cipher suite for MACsec.")
@click.option('--primary_cak', metavar='<primary_cak>', required=True, type=str, help="Primary Connectivity Association Key.")
@click.option('--primary_ckn', metavar='<primary_cak>', required=True, type=str, help="Primary CAK Name.")
@click.option('--policy', metavar='<policy>', required=False, default="security", show_default=True, type=click.Choice(["integrity_only", "security"]), help="MACsec policy. INTEGRITY_ONLY: All traffic, except EAPOL, will be converted to MACsec packets without encryption.  SECURITY: All traffic, except EAPOL, will be encrypted by SecY.")
@click.option('--enable_replay_protect/--disable_replay_protect', metavar='<replay_protect>', required=False, default=False, show_default=True, is_flag=True, help="Whether enable replay protect.")
@click.option('--replay_window', metavar='<enable_replay_protect>', required=False, default=0, show_default=True, type=click.IntRange(0, 2**32), help="Replay window size that is the number of packets that could be out of order. This field works only if ENABLE_REPLAY_PROTECT is true.")
@click.option('--send_sci/--no_send_sci', metavar='<send_sci>', required=False, default=True, show_default=True, is_flag=True, help="Send SCI in SecTAG field of MACsec header.")
@click.option('--rekey_period', metavar='<rekey_period>', required=False, default=0, show_default=True, type=click.IntRange(min=0), help="The period of proactively refresh (Unit second).")
def add_profile(profile, priority, cipher_suite, primary_cak, primary_ckn, policy, enable_replay_protect, replay_window, send_sci, rekey_period):
    """
    Add MACsec profile
    """
    ctx = click.get_current_context()
    config_db = ctx.obj

    profile_entry = config_db.get_entry('MACSEC_PROFILE', profile)
    if not len(profile_entry) == 0:
        ctx.fail("{} already exists".format(profile))

    profile_table = {}

    profile_table["priority"] = priority

    profile_table["cipher_suite"] = cipher_suite

    if "128" in cipher_suite:
        if len(primary_cak) != 66:
            ctx.fail("Expect the length of CAK is 66, but got {}".format(len(primary_cak)))
    elif "256" in cipher_suite:
        if len(primary_cak) != 130:
            ctx.fail("Expect the length of CAK is 130, but got {}".format(len(primary_cak)))
    if not is_hexstring(primary_cak):
        ctx.fail("Expect the primary_cak is valid hex string")
    if not is_hexstring(primary_ckn):
        ctx.fail("Expect the primary_ckn is valid hex string")
    profile_table["primary_cak"] = primary_cak
    profile_table["primary_ckn"] = primary_ckn

    profile_table["policy"] = policy

    if enable_replay_protect and replay_window > 0:
        profile_table["enable_replay_protect"] = enable_replay_protect
        profile_table["replay_window"] = replay_window

    profile_table["send_sci"] = send_sci

    if rekey_period > 0:
        profile_table["rekey_period"] = rekey_period

    for k, v in profile_table.items():
        if isinstance(v, bool):
            if v:
                profile_table[k] = "true"
            else:
                profile_table[k] = "false"
        else:
            profile_table[k] = str(v)
    config_db.set_entry("MACSEC_PROFILE", profile, profile_table)


#
# 'del' command ('config macsec profile del ...')
#
@macsec_profile.command('del')
@click.argument('profile', metavar='<profile_name>', required=True)
def del_profile( profile):
    """
    Delete MACsec profile
    """
    ctx = click.get_current_context()
    config_db = ctx.obj

    profile_entry = config_db.get_entry('MACSEC_PROFILE', profile)
    if len(profile_entry) == 0:
        ctx.fail("{} doesn't exist".format(profile))

    # Check if the profile is being used by any port
    for port in config_db.get_keys('PORT'):
        attr = config_db.get_entry('PORT', port)
        if 'macsec' in attr and attr['macsec'] == profile:
            ctx.fail("{} is being used by port {}, Please remove the MACsec from the port firstly".format(profile, port))

    config_db.set_entry("MACSEC_PROFILE", profile, None)


def register(cli):
    cli.add_command(macsec)


if __name__ == '__main__':
    macsec()
