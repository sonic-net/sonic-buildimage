import click
import utilities_common.cli as clicommon


@click.group(cls=clicommon.AliasedGroup, name="dhcp_server")
def dhcp_server():
    pass


def register(cli):
    # cli.add_command(dhcp_server)
    pass


if __name__ == '__main__':
    dhcp_server()
