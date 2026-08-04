FROM docker-base

MAINTAINER Xudong Wu

## Make apt-get non-interactive
ENV DEBIAN_FRONTEND=noninteractive

## Update apt's cache of available packages
RUN apt-get clean && apt-get update

COPY deps /deps

RUN dpkg -i /deps/libopennsl_*.deb; \
    dpkg -i /deps/libsaibcm_*.deb;  \
    apt-get -y install -f

RUN mv /deps/basic_router /usr/sbin/basic_router

COPY ["start.sh", "/usr/bin/"]
COPY ["supervisord.conf", "/etc/supervisor/conf.d/"]

ENTRYPOINT ["/usr/local/bin/supervisord"]
