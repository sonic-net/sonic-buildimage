#! /usr/bin/env python3

import argparse
import json
import os
import syslog

import docker
from swsscommon import swsscommon

CTR_NAMES_FILE = "/usr/share/sonic/templates/ctr_image_names.json"

LATEST_TAG = "latest"

# DB fields
CT_OWNER = 'current_owner'
CT_ID = 'container_id'
SYSTEM_STATE = 'system_state'

def _get_local_image_name(feature):
    d = {}
    if os.path.exists(CTR_NAMES_FILE):
        with open(CTR_NAMES_FILE, "r") as s:
            d = json.load(s)
    return d[feature] if feature in d else None


def _remove_container(client, feature):
    try:
        # Remove stopped container instance, if any for this feature
        container = client.containers.get(feature)
        container.remove()
        syslog.syslog(syslog.LOG_INFO, "Removed container for {}".
                format(feature))
        return 0
    except Exception as err:
        syslog.syslog(syslog.LOG_INFO, "No container to remove for {} err={}".
                format(feature, str(err)))
        return -1


def _tag_container_image(feature, container_id, image_name, image_tag):
    client = docker.from_env()

    try:
        container = client.containers.get(container_id)

        # Tag this image for given name & tag
        container.image.tag(image_name, image_tag, force=True)

        syslog.syslog(syslog.LOG_INFO,
                "Tagged image for {} with container-id={} to {}:{}".
                format(feature, container_id, image_name, image_tag))
        ret = _remove_container(client, feature)
        return ret

    except Exception as err:
        syslog.syslog(syslog.LOG_ERR, "Image tag: container:{} {}:{} failed with {}".
                format(container_id, image_name, image_tag, str(err)))
        return -1


def tag_feature(feature=None, image_name=None, image_tag=None):
    ret = 0
    state_db = swsscommon.DBConnector("STATE_DB", 0)
    tbl = swsscommon.Table(state_db, 'FEATURE')
    keys = tbl.getKeys()
    for k in keys:
        if (not feature) or (k == feature):
            d = dict(tbl.get(k)[1])
            owner = d.get(CT_OWNER, "")
            id = d.get(CT_ID, "")
            if not image_name:
                image_name = _get_local_image_name(k)
            if not image_tag:
                image_tag = LATEST_TAG
            if id and (owner == "kube") and image_name:
                ret = _tag_container_image(k, id, image_name, image_tag)
            else:
                syslog.syslog(syslog.LOG_ERR,
                        "Skip to tag feature={} image={} tag={}".format(
                            k, str(image_name), str(image_tag)))
                ret = -1

            image_name=None
    return ret


def func_tag_feature(args):
    return tag_feature(args.feature, args.image_name, args.image_tag)


def func_tag_all(args):
    return tag_feature()


def func_kill_all(args):
    client = docker.from_env()
    state_db = swsscommon.DBConnector("STATE_DB", 0)
    tbl = swsscommon.Table(state_db, 'FEATURE')

    keys = tbl.getKeys()
    for k in keys:
        data = dict(tbl.get(k)[1])
        is_up = data.get(SYSTEM_STATE, "").lower() == "up"
        id = data.get(CT_ID, "") if is_up else ""
        if id:
            try:
                container = client.containers.get(id)
                container.kill()
                syslog.syslog(syslog.LOG_INFO,
                        "Killed container for {} with id={}".format(k, id))
            except Exception as err:
                syslog.syslog(syslog.LOG_ERR,
                        "kill: feature={} id={} unable to get container".
                        format(k, id))
                return -1
    return 0


def main():
    parser = argparse.ArgumentParser(description="Remote container mgmt tools")
    subparsers = parser.add_subparsers(title="ctrmgr_tools")

    parser_tag = subparsers.add_parser('tag')
    parser_tag.add_argument("-f", "--feature", required=True)
    parser_tag.add_argument("-n", "--image-name")
    parser_tag.add_argument("-t", "--image-tag")
    parser_tag.set_defaults(func=func_tag_feature)

    parser_tag_all = subparsers.add_parser('tag-all')
    parser_tag_all.set_defaults(func=func_tag_all)

    parser_kill_all = subparsers.add_parser('kill-all')
    parser_kill_all.set_defaults(func=func_kill_all)

    args = parser.parse_args()
    if len(args.__dict__) < 1:
        parser.print_help()
        return -1

    ret = args.func(args)
    return ret


if __name__ == "__main__":
    main()
