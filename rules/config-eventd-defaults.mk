# ENABLE_SYSTEM_EVENTD default derivation.
#
# EVENTD is a special case: its default boot-enabled state historically
# depended on BUILD_REDUCE_IMAGE_SIZE (see
# files/build_templates/init_cfg.json.j2). Since both INCLUDE_SYSTEM_EVENTD
# and BUILD_REDUCE_IMAGE_SIZE can be overridden by rules/config.<SONIC_PROFILE>
# and rules/config.user, this derivation must run *after* both of those are
# loaded.
#
# This file is also included last (after rules/config, rules/config.<SONIC_PROFILE>,
# and rules/config.user) by Makefile.work and slave.mk.
#
ifndef ENABLE_SYSTEM_EVENTD
ifeq ($(BUILD_REDUCE_IMAGE_SIZE),y)
ENABLE_SYSTEM_EVENTD := n
else
ENABLE_SYSTEM_EVENTD := y
endif
endif
