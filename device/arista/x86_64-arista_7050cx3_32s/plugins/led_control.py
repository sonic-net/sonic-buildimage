try:
    import arista.utils.sonic_leds as arista_leds
except ImportError, e:
    raise ImportError (str(e) + "- required module not found")

LedControl = arista_leds.getLedControl()
