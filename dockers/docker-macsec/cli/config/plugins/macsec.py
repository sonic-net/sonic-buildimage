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

    if 'macsec' in port_entry:
        del port_entry['macsec']
        config_db.set_entry("PORT", port, port_entry)
    else:
        click.echo("port {} has no configured macsec profile".format(port))

#
# 'profile' group ('config macsec profile ...')
#
@macsec.group(cls=clicommon.AbbreviationGroup, name='profile')
def macsec_profile():
    pass


CIPHER_SUITES = ["GCM-AES-128", "GCM-AES-256", "GCM-AES-XPN-128", "GCM-AES-XPN-256"]

DEFAULT_CIPHER_SUITE = "GCM-AES-128"


def is_hexstring(hexstring: str):
    try:
        int(hexstring, 16)
        return True
    except ValueError:
        return False


def expected_cak_length(cipher_suite):
    """Length, in characters, of an encoded CAK for the given cipher suite."""
    if "128" in cipher_suite:
        return 66
    elif "256" in cipher_suite:
        return 130
    return None


def validate_cak(ctx, cipher_suite, cak):
    length = expected_cak_length(cipher_suite)
    if length is not None and len(cak) != length:
        ctx.fail("Expect the length of CAK is {}, but got {}".format(length, len(cak)))
    if not is_hexstring(cak):
        ctx.fail("Expect the CAK is valid hex string")


def validate_ckn(ctx, ckn):
    if not is_hexstring(ckn):
        ctx.fail("Expect the CKN is valid hex string")


def validate_fallback(ctx, cipher_suite, primary_ckn, fallback_cak, fallback_ckn):
    validate_cak(ctx, cipher_suite, fallback_cak)
    validate_ckn(ctx, fallback_ckn)
    # A CA is keyed by its CKN, so the fallback cannot reuse the primary's name.
    # macsecmgr rejects such a profile outright and the YANG model carries the
    # same constraint.
    if fallback_ckn.lower() == primary_ckn.lower():
        ctx.fail("Expect the fallback_ckn is different from the primary_ckn")


def ports_using_profile(config_db, profile):
    """Names of the ports that currently have 'profile' applied."""
    ports = []
    for port in config_db.get_keys('PORT'):
        attr = config_db.get_entry('PORT', port)
        if attr.get('macsec') == profile:
            ports.append(port)
    return ports


#
# 'add' command ('config macsec profile add ...')
#
@macsec_profile.command('add')
@click.argument('profile', metavar='<profile_name>', required=True)
@click.option('--priority', metavar='<priority>', required=False, default=255, show_default=True, type=click.IntRange(0, 255), help="For Key server election. In 0-255 range with 0 being the highest priority.")
@click.option('--cipher_suite', metavar='<cipher_suite>', required=False, default=DEFAULT_CIPHER_SUITE, show_default=True, type=click.Choice(CIPHER_SUITES), help="The cipher suite for MACsec.")
@click.option('--primary_cak', metavar='<primary_cak>', required=True, type=str, help="Primary Connectivity Association Key.")
@click.option('--primary_ckn', metavar='<primary_ckn>', required=True, type=str, help="Primary CAK Name.")
@click.option('--fallback_cak', metavar='<fallback_cak>', required=False, default=None, type=str, help="Fallback Connectivity Association Key, used as a standby CA that takes over the port if the primary CA fails. Must be given together with --fallback_ckn.")
@click.option('--fallback_ckn', metavar='<fallback_ckn>', required=False, default=None, type=str, help="Fallback CAK Name. Must be given together with --fallback_cak and must differ from the primary CKN.")
@click.option('--policy', metavar='<policy>', required=False, default="security", show_default=True, type=click.Choice(["integrity_only", "security"]), help="MACsec policy. INTEGRITY_ONLY: All traffic, except EAPOL, will be converted to MACsec packets without encryption.  SECURITY: All traffic, except EAPOL, will be encrypted by SecY.")
@click.option('--enable_replay_protect/--disable_replay_protect', metavar='<replay_protect>', required=False, default=False, show_default=True, is_flag=True, help="Whether enable replay protect.")
@click.option('--replay_window', metavar='<replay_window>', required=False, default=0, show_default=True, type=click.IntRange(0, 2**32), help="Replay window size that is the number of packets that could be out of order. This field works only if ENABLE_REPLAY_PROTECT is true.")
@click.option('--send_sci/--no_send_sci', metavar='<send_sci>', required=False, default=True, show_default=True, is_flag=True, help="Send SCI in SecTAG field of MACsec header.")
@click.option('--rekey_period', metavar='<rekey_period>', required=False, default=0, show_default=True, type=click.IntRange(min=0), help="The period of proactively refresh (Unit second).")
def add_profile(profile, priority, cipher_suite, primary_cak, primary_ckn, fallback_cak, fallback_ckn, policy, enable_replay_protect, replay_window, send_sci, rekey_period):
    """
    Add MACsec profile
    """
    ctx = click.get_current_context()
    config_db = ctx.obj

    profile_entry = config_db.get_entry('MACSEC_PROFILE', profile)
    if not len(profile_entry) == 0:
        ctx.fail("{} already exists".format(profile))

    if (fallback_cak is None) != (fallback_ckn is None):
        ctx.fail("Expect --fallback_cak and --fallback_ckn are provided together")

    profile_table = {}

    profile_table["priority"] = priority

    profile_table["cipher_suite"] = cipher_suite

    validate_cak(ctx, cipher_suite, primary_cak)
    validate_ckn(ctx, primary_ckn)
    profile_table["primary_cak"] = primary_cak
    profile_table["primary_ckn"] = primary_ckn

    if fallback_cak is not None:
        validate_fallback(ctx, cipher_suite, primary_ckn, fallback_cak, fallback_ckn)
        profile_table["fallback_cak"] = fallback_cak
        profile_table["fallback_ckn"] = fallback_ckn

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
# 'update' command ('config macsec profile update ...')
#
@macsec_profile.command('update')
@click.argument('profile', metavar='<profile_name>', required=True)
@click.option('--old_ckn', metavar='<old_ckn>', required=True, type=str, help="CAK Name of the key being replaced. It selects which CA of the profile is rotated, so it has to match the profile's current primary CKN or its fallback CKN.")
@click.option('--new_ckn', metavar='<new_ckn>', required=True, type=str, help="New CAK Name. Must differ from the old CKN and from the CKN of the other CA of the profile.")
@click.option('--new_cak', metavar='<new_cak>', required=True, type=str, help="New Connectivity Association Key, of the length required by the cipher suite of the profile.")
def update_profile(profile, old_ckn, new_ckn, new_cak):
    """
    Rotate one of the keys of a MACsec profile

    Replaces the key named by --old_ckn with --new_ckn and --new_cak. The old
    CKN selects which CA is rotated, so exactly one key is touched and the
    other stays live to protect the port. Every other field of the profile is
    left as it is.

    A CA is keyed by its CKN, so a rotation always establishes a new CKN and
    re-using the old one is rejected. Keys can only be replaced, not added or
    removed: a profile that needs a fallback key it wasn't created with has to
    be deleted and added again.
    """
    ctx = click.get_current_context()
    config_db = ctx.obj

    profile_entry = config_db.get_entry('MACSEC_PROFILE', profile)
    if len(profile_entry) == 0:
        ctx.fail("{} doesn't exist".format(profile))

    primary_ckn = profile_entry.get("primary_ckn", "")
    fallback_ckn = profile_entry.get("fallback_ckn", "")

    # The old CKN names the CA to rotate. Anything else would be a request to
    # add a key rather than replace one, which this command does not do.
    if primary_ckn and old_ckn.lower() == primary_ckn.lower():
        rotating_primary = True
    elif fallback_ckn and old_ckn.lower() == fallback_ckn.lower():
        rotating_primary = False
    else:
        ctx.fail(
            "Expect the old_ckn matches the primary_ckn or the fallback_ckn of "
            "{}, a key that is not configured cannot be rotated".format(profile))

    # A participant is keyed by its CKN, so re-using it would leave the running
    # MKA session on the old CAK until wpa_supplicant restarts.
    if new_ckn.lower() == old_ckn.lower():
        ctx.fail(
            "Expect the new_ckn is different from the old_ckn, the CAK of a "
            "CKN that is already established cannot be replaced in place")

    # The stored cipher suite fixes the length the new CAK has to satisfy.
    cipher_suite = profile_entry.get("cipher_suite", DEFAULT_CIPHER_SUITE)

    profile_table = dict(profile_entry)

    if rotating_primary:
        validate_cak(ctx, cipher_suite, new_cak)
        validate_ckn(ctx, new_ckn)
        # Taking over the fallback's CKN would promote the standby rather than
        # rotate the primary, leaving the port with a single CA.
        if fallback_ckn and new_ckn.lower() == fallback_ckn.lower():
            ctx.fail(
                "Expect the new_ckn is different from the fallback_ckn of {}, "
                "the fallback key has to stay in place to protect the port "
                "while the primary key is rotated".format(profile))

        # macsecmgr rotates the primary CA by retiring the running participant
        # before adding its replacement, and leans on the fallback CA to carry
        # traffic in between. Without a fallback it refuses the rotation, so
        # reject it here rather than leaving the change stuck in CONFIG_DB.
        if not fallback_ckn:
            ports = ports_using_profile(config_db, profile)
            if ports:
                ctx.fail(
                    "{} is in use by {} and has no fallback key, so its primary "
                    "key cannot be rotated without leaving the ports "
                    "unprotected".format(profile, ", ".join(ports)))

        profile_table["primary_cak"] = new_cak
        profile_table["primary_ckn"] = new_ckn
    else:
        validate_fallback(ctx, cipher_suite, primary_ckn, new_cak, new_ckn)
        profile_table["fallback_cak"] = new_cak
        profile_table["fallback_ckn"] = new_ckn

    # Unlike 'add', nothing here is a bool or an int, so the entry needs no
    # conversion before it is written back.
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
