import os
import click

import show.plugins.macsec as show_macsec
import utilities_common.cli as clicommon
from sonic_py_common import multi_asic

@click.group(cls=clicommon.AliasedGroup)
def macsec():
    pass


@macsec.command('macsec')
@click.option('--clean-cache', type=bool, required=False, default=False, help="If the option of clean cache is true, next show commands will show the raw counters which based on the service booted instead of the last clear command.")
def macsec_clear_counters(clean_cache):
    """ 
        Clear MACsec counts.
        This clear command will generated a cache for next show commands which will base on this cache as the zero baseline to show the increment of counters.
    """

    if clean_cache:
        for namespace in multi_asic.get_namespace_list():
            if os.path.isfile(show_macsec.CACHE_FILE.format(namespace)):
                os.remove(show_macsec.CACHE_FILE.format(namespace))
            print("Cleaned cache")
        return

    clicommon.run_command(['show', 'macsec', '--dump-file'])
    print("Clear MACsec counters")

def register(cli):
    cli.add_command(macsec_clear_counters)


if __name__ == '__main__':
    macsec_clear_counters(None)
