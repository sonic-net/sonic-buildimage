#!/usr/bin/env python3

from inspect import getframeinfo, stack
from swsscommon.swsscommon import events_init_publisher, event_publish, FieldValueMap
from swsscommon.swsscommon import event_receive_op_t, event_receive, events_init_subscriber
from swsscommon.swsscommon import events_deinit_subscriber, events_deinit_publisher
import argparse
import os
import threading
import time
import syslog
import uuid

chk_log_level = syslog.LOG_ERR

test_source = "sonic-host"
test_event_tag = "device-test-event"
test_event_key = "{}:{}".format(test_source, test_event_tag)
test_event_params = {
    "sender": os.path.basename(__file__),
    "reason": "monit periodic test",
    "batch-id": str(uuid.uuid1()),
    "index": "0"
}

# Async connection wait time in seconds.
ASYNC_CONN_WAIT = 0.3
RECEIVE_TIMEOUT = 1000

# Thread results
rc_test_receive = -1
publish_cnt = 0

def _log_msg(lvl, pfx, msg):
    if lvl <= chk_log_level:
        caller = getframeinfo(stack()[2][0])
        fmsg = "{}:{}:{}".format(caller.function, caller.lineno, msg)
        print("{}: {}".format(pfx, fmsg))
        syslog.syslog(lvl, fmsg)

def log_err(m):
    _log_msg(syslog.LOG_ERR, "Err", m)

def log_info(m):
    _log_msg(syslog.LOG_INFO, "Info",  m)

def log_notice(m):
    _log_msg(syslog.LOG_NOTICE, "Notice", m)

def log_debug(m):
    _log_msg(syslog.LOG_DEBUG, "Debug", m)


def map_dict_fvm(s, d):
    for k, v in s.items():
        d[k] = v


# Invoked in a separate thread
def test_receiver(event_obj, cnt):
    global rc_test_receive

    sh = events_init_subscriber(False, RECEIVE_TIMEOUT, None)

    # Sleep ASYNC_CONN_WAIT to ensure async connectivity is complete.
    time.sleep(ASYNC_CONN_WAIT)

    exp_params = dict(test_event_params)

    # Signal main thread that subscriber is ready to receive
    event_obj.set()
    cnt_done = 0

    while cnt_done < cnt:
        p = event_receive_op_t()
        rc = event_receive(sh, p)

        if rc > 0 and publish_cnt < 2:
            # ignore timeout as test code has not yet published an event yet
            continue

        if rc != 0:
            log_notice("Failed to receive. {}/{} rc={}".format(cnt_done, cnt, rc))
            break

        if test_event_key != p.key:
            # received a different event than published
            continue

        exp_params["index"] = str(cnt_done)
        rcv_params = {}
        map_dict_fvm(p.params, rcv_params)

        for k, v in exp_params.items():
            if k in rcv_params:
                if (rcv_params[k] != v):
                    log_notice("key:{} exp:{} != exist:{}".format(
                        k, v, rcv_params[k]))
                    rc = -1
            else:
                log_notice("key:{} is missing".format(k))
                rc = -1

        if (rc != 0):
            log_notice("params mismatch {}/{}".format(cnt_done, cnt))
            break

        if p.missed_cnt != 0:
            log_notice("Expect missed_cnt {} == 0 {}/{}".format(p.missed_cnt, cnt_done, cnt))
            break

        if p.publish_epoch_ms == 0:
            log_notice("Expect publish_epoch_ms != 0 {}/{}".format(cnt_done, cnt))
            break

        cnt_done += 1
        log_debug("Received {}/{}".format(cnt_done + 1, cnt))

    if (cnt_done == cnt):
        rc_test_receive = 0
    else:
        log_notice("test receive abort {}/{}".format(cnt_done, cnt))

    # Signal main thread that subscriber thread is done
    event_obj.set()
    events_deinit_subscriber(sh)


def publish_events(cnt):
    global publish_cnt
    rc = -1
    ph = events_init_publisher(test_source)
    if not ph:
        log_notice("Failed to get publisher handle")
        return rc

    # Sleep ASYNC_CONN_WAIT to ensure async connectivity is complete.
    # Messages published before connection are silently dropped by ZMQ.
    time.sleep(ASYNC_CONN_WAIT)

    pub_params = dict(test_event_params)

    for i in range(cnt):
        pd  = FieldValueMap()
        pub_params["index"] = str(i)
        map_dict_fvm(pub_params, pd)

        rc = event_publish(ph, test_event_tag, pd)
        if (rc != 0):
            log_notice("Failed to publish. {}/{} rc={}".format(i, cnt, rc))
            break

        log_debug("published: {}/{}".format(i+1, cnt))
        publish_cnt += 1

    # Sleep ASYNC_CONN_WAIT to ensure publish complete, before closing channel.
    time.sleep(ASYNC_CONN_WAIT)

    events_deinit_publisher(ph)

    log_debug("publish_events Done. cnt={}".format(cnt))

    return rc



def run_test(cnt):
    global rc_test_receive

    # Initialising event objects
    event_sub = threading.Event()

    # Start subscriber thread
    thread_sub = threading.Thread(target=test_receiver, args=(event_sub, cnt))
    thread_sub.start()

    # Wait until subscriber thread completes the async subscription
    # Any event published prior to that could get lost
    # Subscriber would wait for ASYNC_CONN_WAIT. Wait additional 200ms
    # for signal from test_receiver as ready.
    event_sub.wait(ASYNC_CONN_WAIT + 0.2)
    event_sub.clear()

    rc_pub = publish_events(cnt)
    if (rc_pub != 0):
        log_notice("Failed in publish_events")
    else:
        # Wait for subscriber to complete with 1 sec timeout.
        event_sub.wait(1)
        if (rc_test_receive != 0):
            log_notice("Failed to receive events")

    log_debug("run_test_DONE rc_pub={} rc_test_receive={}".format(
        rc_pub, rc_test_receive))

    if (rc_pub != 0):
        return rc_pub

    if (rc_test_receive == 0):
        return rc_test_receive

    return 0


def main():
    global chk_log_level

    parser=argparse.ArgumentParser(
            description="Check events from publish to receive via gNMI")
    parser.add_argument('-l', "--loglvl", default=syslog.LOG_ERR, type=int,
            help="log level")
    parser.add_argument('-n', "--cnt", default=5, type=int,
            help="count of events to publish/receive")
    args = parser.parse_args()

    chk_log_level = args.loglvl
    rc = run_test(args.cnt)

    if(rc == 0):
        log_info("eventd test succeeded")
    else:
        log_notice("eventd monit test failed rc={}".format(rc))


if __name__ == "__main__":
    main()
