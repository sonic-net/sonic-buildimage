# docker image for orchagent

#
# DOCKER_ORCHAGENT comes from rules/docker-orchagent.mk which is
# included before the platform rules.mk
#
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /var/cache/cisco:/var/cache/cisco
