import click
import utilities_common.cli as clicommon


@click.group(cls=clicommon.AliasedGroup, name="ptp")
def ptp():
    """Placeholder PTP clear CLI (implemented in sonic-utilities)."""


def register(cli):
    pass


if __name__ == "__main__":
    ptp()