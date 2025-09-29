#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0


def setup_click_mocks():
    """
    Set up click mocks to handle unsupported arguments in older click versions.

    This function creates wrapper functions for click decorators that automatically
    filter out known unsupported keyword arguments, allowing code written for newer
    click versions to run in environments with older click installations.

    The mock filters out predefined unsupported parameters like 'autocompletion'
    from click decorators.

    Example usage:
        # This would fail in older click versions:
        @click.argument("file", autocompletion=complete_files)
        def process_file(file):
            pass

        # With setup_click_mocks() called, the autocompletion parameter
        # is automatically filtered out in older click versions, allowing
        # the decorator to work without errors.
    """
    import click

    # Store original click functions
    original_argument = click.argument
    original_option = click.option
    original_command = click.command
    original_group = click.group

    # Predefined list of unsupported parameters to filter out
    UNSUPPORTED_PARAMS = {'autocompletion'}

    def filter_unsupported_kwargs(**kwargs):
        """Remove predefined unsupported kwargs from click decorator calls."""
        filtered_kwargs = {k: v for k, v in kwargs.items() if k not in UNSUPPORTED_PARAMS}
        return filtered_kwargs

    def safe_argument(*args, **kwargs):
        """Wrapper for click.argument that filters unsupported kwargs."""
        filtered_kwargs = filter_unsupported_kwargs(**kwargs)
        return original_argument(*args, **filtered_kwargs)

    def safe_option(*args, **kwargs):
        """Wrapper for click.option that filters unsupported kwargs."""
        filtered_kwargs = filter_unsupported_kwargs(**kwargs)
        return original_option(*args, **filtered_kwargs)

    def safe_command(*args, **kwargs):
        """Wrapper for click.command that filters unsupported kwargs."""
        filtered_kwargs = filter_unsupported_kwargs(**kwargs)
        return original_command(*args, **filtered_kwargs)

    def safe_group(*args, **kwargs):
        """Wrapper for click.group that filters unsupported kwargs."""
        filtered_kwargs = filter_unsupported_kwargs(**kwargs)
        return original_group(*args, **filtered_kwargs)

    # Replace click functions with safe versions
    click.argument = safe_argument
    click.option = safe_option
    click.command = safe_command
    click.group = safe_group
        
        
setup_click_mocks()
