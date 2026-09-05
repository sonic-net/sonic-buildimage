#
# Arista LED controls for SONiC
#

try:
    import arista.utils.sonic_leds as arista_leds
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

LedControl = arista_leds.getLedControl()
