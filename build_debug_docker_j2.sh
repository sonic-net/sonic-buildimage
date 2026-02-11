#! /bin/bash

echo "
ARG BASE=$1
FROM \$BASE AS base

ARG docker_container_name

## Make apt-get non-interactive
ENV DEBIAN_FRONTEND=noninteractive

{% if $2 is defined %}
{% if $2|length %}

COPY \
{% for deb in $2.split(' ') -%}
debs/{{ deb }}{{' '}}
{%- endfor -%}
debs/

RUN apt update

RUN dpkg -i \
{% for deb in $2.split(' ') -%}
debs/{{ deb }}{{' '}}
{%- endfor %} 2>&1 \
| sed -n 's/.*Package \(.*\) is not installed\./\1/p' \
| sort -u > /tmp/missing_deps.txt || true

RUN xargs -a /tmp/missing_deps.txt -r apt install -y

RUN dpkg --configure -a

{% endif %}
{% endif %}

{% if $3 is defined %}
{% if $3|length %}

RUN apt install -f -y \
{% for dbg in $3.split(' ') -%}
{{ dbg }}{{' '}}
{%- endfor %}

{% endif %}
{% endif %}


## Clean up
RUN apt-get clean -y; apt-get autoclean -y; apt-get autoremove -y
RUN rm -rf /debs

FROM \$BASE

RUN --mount=type=bind,from=base,target=/changes-to-image rsync -axAX --no-D --exclude=/sys --exclude=/proc --exclude=/dev --exclude=resolv.conf /changes-to-image/ /

"
