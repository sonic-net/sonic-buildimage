# Xsight xplt tools package for es9632x
# Currently we use the same deb for xx and xq boxes.
# TODO: bring and install the deb per box type.

# Add platform specific tools
XPLT_VERSION=3.0.0-3-g343b118

XPLT_TOOLS = xplt_es9632xq_sonicnos-$(XPLT_VERSION)_amd64.deb
$(XPLT_TOOLS)_URL = "https://github.com/Xsight-Labs/SONiC/raw/main/amd64/utils/"$(XPLT_TOOLS)
SONIC_ONLINE_DEBS += $(XPLT_TOOLS)
