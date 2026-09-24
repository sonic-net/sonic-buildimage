#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################


try:
    from sonic_platform_pddf_base.pddf_platform import PddfPlatform
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Platform(PddfPlatform):
    """
    PDDF Platform-Specific Platform Class
    """

    def __init__(self):
        PddfPlatform.__init__(self)

    # Provide the functions/variables below for which implementation is to be overwritten
