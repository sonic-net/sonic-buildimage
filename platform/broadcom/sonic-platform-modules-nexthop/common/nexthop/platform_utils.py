import subprocess
import click
import os
import sys


def run_cmd(cmd, check_exit=True):
    """
    Run a command and return the output
    Args:
      cmd (str): The command to run
      check_exit (bool): Whether to check the exit code and raise an exception if it's non-zero
    Returns:
      str: The output of the command (stdout)
    Raises:
      subprocess.CalledProcessError: If check_exit is True and command fails
      subprocess.SubprocessError: If there's an error running the command
      OSError: If there's an OS-level error
    """
    try:
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True, check=check_exit
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        # Re-raise with more context
        raise subprocess.CalledProcessError(
            e.returncode, cmd, output=e.stdout, stderr=e.stderr
        )
    except (subprocess.SubprocessError, OSError):
        raise


def run_and_report(description, command):
    """
    Run a command and report the result with colored output.
    
    Args:
        description (str): Description of the operation being performed
        command (str): The command to execute
    """
    click.secho(f"{description}:", fg="cyan")
    try:
        run_cmd(command)
        click.secho(f"Successfully {description.lower()}", fg="green")
    except (subprocess.CalledProcessError, subprocess.SubprocessError, OSError):
        click.secho(f"Failed to {description.lower()}", fg="red")