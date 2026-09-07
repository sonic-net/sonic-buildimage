# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

# docker image for orchagent

#
# DOCKER_ORCHAGENT comes from rules/docker-orchagent.mk which is
# included before the platform rules.mk
#
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /var/cache/cisco:/var/cache/cisco
