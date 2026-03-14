#
# Arista SFP transceiver interface for SONiC
#

try:
    import arista.utils.sonic_sfputil as arista_sfputil
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

SfpUtil = arista_sfputil.getSfpUtil()
