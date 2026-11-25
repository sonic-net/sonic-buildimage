import click
import utilities_common.cli as clicommon


@click.group(cls=clicommon.AbbreviationGroup, name="ptp")
def ptp():
    """Placeholder PTP show CLI (implemented in sonic-utilities)."""


def register(cli):
    pass


if __name__ == "__main__":
    ptp()