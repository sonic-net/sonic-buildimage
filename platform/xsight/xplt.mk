# Xsight xplt tools package for es9618xx

# Add platform specific tools

ACCTON_ES9618XX_XPLT_PLATFORM_MODULE = xplt-es9618xx-sonicnos-py3.13_3.4.1-79-g50272c9_amd64.deb
ES9618XX_XPLT_SRV = "https://raw.githubusercontent.com/xsightlabs/sonic-xsight-binaries/main/amd64/utils/"
$(ACCTON_ES9618XX_XPLT_PLATFORM_MODULE)_URL = $(ES9618XX_XPLT_SRV)$(ACCTON_ES9618XX_XPLT_PLATFORM_MODULE)
$(ACCTON_ES9618XX_XPLT_PLATFORM_MODULE)_PLATFORM = x86_64-accton_es9618xx-r0
SONIC_ONLINE_DEBS += $(ACCTON_ES9618XX_XPLT_PLATFORM_MODULE)
