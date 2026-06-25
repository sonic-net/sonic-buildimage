import subprocess
import time
from typing import Callable, Literal, TypeVar

import click

T = TypeVar("T")


def wait_until(
    condition: Callable[[], T],
    timeout_secs: float,
    initial_interval_secs: float = 1.0,
    max_interval_secs: float | None = None,
    backoff_factor: float = 2.0,
    logger=None,
    description: str = "",
) -> T | Literal[False]:
    """Wait for `condition()` to return a truthy value, polling with exponential backoff.

    Args:
        condition: A callable that returns a truthy value when the wait is over.
        timeout_secs: Maximum wall-clock seconds to wait.
        initial_interval_secs: Sleep duration before the first retry.
        max_interval_secs: Cap on the sleep interval (defaults to `timeout_secs`).
        backoff_factor: Multiplier applied to the interval after each poll.
                        For constant polling, set this to 1.0.
        logger: Optional logger with `log_info` and `log_error` methods.
        description: Human-readable label for log messages.

    Returns:
        The truthy value returned by `condition()`, or False on timeout.
    """
    if max_interval_secs is None:
        max_interval_secs = timeout_secs
    deadline = time.monotonic() + timeout_secs
    interval = initial_interval_secs
    while True:
        result = condition()
        if result:
            return result

        remaining = deadline - time.monotonic()
        if remaining <= 0:
            if logger:
                logger.log_error(f"Timed out waiting for '{description}' after {timeout_secs}s")
            return False

        sleep_time = min(interval, remaining)
        if logger:
            logger.log_info(f"'{description}' not yet ready, retrying in {sleep_time:.1f}s")
        time.sleep(sleep_time)

        interval = min(interval * backoff_factor, max_interval_secs)


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