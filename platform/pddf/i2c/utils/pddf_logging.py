"""Shared logging configuration for PDDF initialization scripts."""

import logging


def configure_logging() -> None:
    """Configure the root logger for a PDDF init script.

    Call once at the top of main() in each PDDF entry-point script.
    """
    logging.basicConfig(
        level=logging.INFO,
        format="%(levelname)s: [%(funcName)s:%(lineno)d] %(message)s",
    )
