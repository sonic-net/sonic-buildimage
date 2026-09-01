#!/usr/bin/env python3
import os
import sys
import json
import ptpmgmt

import contextlib
from datetime import datetime
from threading import Event, Lock, Thread
import swsscommon.swsscommon as swsscommon

import pprint


def fvs_from_dict(d):
    return swsscommon.FieldValuePairs([(k, str(v)) for k, v in d.items()])

def get_bit(number: int, position: int) -> int:
    if position < 0:
        raise ValueError("Bit position must be non-negative.")
    if not isinstance(number, int):
        raise TypeError("Number must be an integer.")
    return (number >> position) & 1


class _SnapshotPortIdentity(object):
    """Detached port identity; ptpmgmt PortIdentity_t aliases reused TLV memory."""

    __slots__ = ('clock_identity', 'port_number')

    def __init__(self, value):
        if isinstance(value, _SnapshotPortIdentity):
            self.clock_identity = value.clock_identity
            self.port_number = value.port_number
            return
        if not isinstance(value, ptpmgmt.PortIdentity_t):
            raise TypeError("value must be a PortIdentity_t")
        self.clock_identity = _snapshot_clock_identity(value.clockIdentity)
        self.port_number = int(value.portNumber)


def _snapshot_clock_identity(value):
    """Snapshot clock identity before ptpmgmt reuses the parse buffer."""
    if isinstance(value, ptpmgmt.ClockIdentity_t):
        return value.string()
    if isinstance(value, str):
        return value
    raise TypeError("clock identity must be ClockIdentity_t or str")


def _snapshot_port_identity(value):
    return _SnapshotPortIdentity(value)


def _port_stats_np_tlv_fields(data):
    """
    PORT_STATS_NP rxMsgType/txMsgType are C arrays; the Python binding does not
    support indexing. tlv2json exposes the same fields as linuxptp pmc.
    """
    return json.loads(ptpmgmt.tlv2json(ptpmgmt.PORT_STATS_NP, data))


class ClockQuality(object):
    def __init__(self):
        pass

    @property
    def clock_class(self):
        if not hasattr(self, '_clock_class'):
            return 0
        return self._clock_class

    @clock_class.setter
    def clock_class(self, value):
        if not isinstance(value, int):
            raise TypeError("clock_class must be an integer")
        self._clock_class = value

    @property
    def clock_accuracy(self):
        if not hasattr(self, '_clock_accuracy'):
            return 0
        return self._clock_accuracy

    @clock_accuracy.setter
    def clock_accuracy(self, value):
        if not isinstance(value, int):
            raise TypeError("clock_accuracy must be an integer")
        self._clock_accuracy = value

    @property
    def offset_scaled_log_variance(self):
        if not hasattr(self, '_offset_scaled_log_variance'):
            return 0
        return self._offset_scaled_log_variance

    @offset_scaled_log_variance.setter
    def offset_scaled_log_variance(self, value):
        if not isinstance(value, int):
            raise TypeError("offset_scaled_log_variance must be an integer")
        self._offset_scaled_log_variance = value

    def populate(self, data):
        # using the ptpmgmt.ClockQuality_t type
        assert isinstance(data, ptpmgmt.ClockQuality_t)
        self.clock_class = data.clockClass
        self.clock_accuracy = data.clockAccuracy
        self.offset_scaled_log_variance = data.offsetScaledLogVariance

    def to_dict(self):
        return {
            "clock-class": self.clock_class,
            "clock-accuracy": self.clock_accuracy,
            "offset-scaled-log-variance": self.offset_scaled_log_variance
        }


class PtpClockDefaultDs(object):
    def __init__(self):
        self._clock_quality = ClockQuality()

    @property
    def two_step_flag(self):
        if not hasattr(self, '_two_step_flag'):
            return False
        return self._two_step_flag

    @two_step_flag.setter
    def two_step_flag(self, value):
        if not isinstance(value, bool):
            raise TypeError("two_step_flag must be a boolean")
        self._two_step_flag = value

    @property
    def clock_identity(self):
        if not hasattr(self, '_clock_identity'):
            return ''
        return self._clock_identity

    @clock_identity.setter
    def clock_identity(self, value):
        self._clock_identity = _snapshot_clock_identity(value)

    @property
    def number_of_ports(self):
        if not hasattr(self, '_number_of_ports'):
            return 0
        return self._number_of_ports

    @number_of_ports.setter
    def number_of_ports(self, value):
        if not isinstance(value, int):
            raise TypeError("number_of_ports must be an integer")
        self._number_of_ports = value

    @property
    def clock_quality(self):
        return self._clock_quality

    @property
    def priority1(self):
        if not hasattr(self, '_priority1'):
            return 0
        return self._priority1

    @priority1.setter
    def priority1(self, value):
        if not isinstance(value, int):
            raise TypeError("priority1 must be an integer")
        self._priority1 = value

    @property
    def priority2(self):
        if not hasattr(self, '_priority2'):
            return 0
        return self._priority2

    @priority2.setter
    def priority2(self, value):
        if not isinstance(value, int):
            raise TypeError("priority2 must be an integer")
        self._priority2 = value

    @property
    def clock_domain(self):
        if not hasattr(self, '_domain_number'):
            return 0
        return self._domain_number

    @clock_domain.setter
    def clock_domain(self, value):
        if not isinstance(value, int):
            raise TypeError("clock_domain must be an integer")

        self._domain_number = value

    @property
    def clock_slave_only(self):
        if not hasattr(self, '_slave_only'):
            return False
        return self._slave_only

    @clock_slave_only.setter
    def clock_slave_only(self, value):
        if not isinstance(value, bool):
            raise TypeError("clock_slave_only must be a boolean")
        self._slave_only = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.DEFAULT_DATA_SET_t)
        self.two_step_flag = bool(get_bit(data.flags, 0))
        self.clock_identity = data.clockIdentity
        self.number_of_ports = data.numberPorts
        self._clock_quality.populate(data.clockQuality)
        self.priority1 = data.priority1
        self.priority2 = data.priority2
        self.clock_domain = data.domainNumber
        self.clock_slave_only = bool(get_bit(data.flags, 1))

    def to_dict(self):
        return {
            "two-step-flag": self.two_step_flag,
            "clock-identity": self.clock_identity,
            "number-ports": self.number_of_ports,
            "clock-quality": self._clock_quality.to_dict(),
            "priority1": self.priority1,
            "priority2": self.priority2,
            "domain-number": self.clock_domain,
            "slave-only": self.clock_slave_only
        }

    def __str__(self):
        return (
            "PtpClockDefaultDs("
            f"two_step_flag={self.two_step_flag}, "
            f"clock_identity={self.clock_identity}, "
            f"number_of_ports={self.number_of_ports}, "
            f"clock_class={self.clock_class}, "
            f"clock_accuracy={self.clock_accuracy}, "
            f"priority1={self.priority1}, "
            f"priority2={self.priority2}, "
            f"clock_domain={self.clock_domain}, "
            f"clock_slave_only={self.clock_slave_only})"
        )


class PtpClockCurrentDs(object):
    def __init__(self):
        pass

    @property
    def steps_removed(self):
        if not hasattr(self, '_steps_removed'):
            return 0
        return self._steps_removed

    @steps_removed.setter
    def steps_removed(self, value):
        if not isinstance(value, int):
            raise TypeError("steps_removed must be an integer")
        self._steps_removed = value

    @property
    def offset_from_master(self):
        if not hasattr(self, '_offset_from_master'):
            return 0
        return self._offset_from_master

    @offset_from_master.setter
    def offset_from_master(self, value):
        if not isinstance(value, int):
            raise TypeError("offset_from_master must be an integer")
        self._offset_from_master = value

    @property
    def mean_path_delay(self):
        if not hasattr(self, '_mean_path_delay'):
            return 0
        return self._mean_path_delay

    @mean_path_delay.setter
    def mean_path_delay(self, value):
        if not isinstance(value, int):
            raise TypeError("mean_path_delay must be an integer")
        self._mean_path_delay = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.CURRENT_DATA_SET_t)
        self.steps_removed = data.stepsRemoved
        self.offset_from_master = data.offsetFromMaster.getIntervalInt()
        self.mean_path_delay = data.meanPathDelay.getIntervalInt()

    def to_dict(self):
        return {
            "steps-removed": self.steps_removed,
            "offset-from-master": self.offset_from_master,
            "mean-path-delay": self.mean_path_delay
        }

    def __str__(self):
        return (
            "PtpClockCurrentDs("
            f"steps_removed={self.steps_removed}, "
            f"offset_from_master={self.offset_from_master}, "
            f"mean_path_delay={self.mean_path_delay})"
        )


class PtpClockParentDs(object):
    def __init__(self):
        self.grandmaster_clock_quality = ClockQuality()

    @property
    def parent_port_clock_identity(self):
        if not hasattr(self, '_parent_port_clock_identity'):
            return ''
        return self._parent_port_clock_identity

    @parent_port_clock_identity.setter
    def parent_port_clock_identity(self, value):
        self._parent_port_clock_identity = _snapshot_clock_identity(value)

    @property
    def parent_port_number(self):
        if not hasattr(self, '_parent_port_number'):
            return 0
        return self._parent_port_number

    @parent_port_number.setter
    def parent_port_number(self, value):
        if not isinstance(value, int):
            raise TypeError("parent_port_number must be an integer")
        self._parent_port_number = value

    @property
    def parent_stats(self):
        if not hasattr(self, '_parent_stats'):
            return False
        return self._parent_stats

    @parent_stats.setter
    def parent_stats(self, value):
        if not isinstance(value, bool):
            raise TypeError("parent_stats must be a boolean")
        self._parent_stats = value

    @property
    def observed_parent_offset_scaled_log_variance(self):
        if not hasattr(self, '_observed_parent_offset_scaled_log_variance'):
            return 0
        return self._observed_parent_offset_scaled_log_variance

    @observed_parent_offset_scaled_log_variance.setter
    def observed_parent_offset_scaled_log_variance(self, value):
        if not isinstance(value, int):
            raise TypeError("observed_parent_offset_scaled_log_variance must be an integer")
        self._observed_parent_offset_scaled_log_variance = value

    @property
    def observed_parent_clock_phase_change_rate(self):
        if not hasattr(self, '_observed_parent_clock_phase_change_rate'):
            return 0
        return self._observed_parent_clock_phase_change_rate

    @observed_parent_clock_phase_change_rate.setter
    def observed_parent_clock_phase_change_rate(self, value):
        if not isinstance(value, int):
            raise TypeError("observed_parent_clock_phase_change_rate must be an integer")
        self._observed_parent_clock_phase_change_rate = value

    @property
    def grandmaster_identity(self):
        if not hasattr(self, '_grandmaster_identity'):
            return ''
        return self._grandmaster_identity

    @grandmaster_identity.setter
    def grandmaster_identity(self, value):
        self._grandmaster_identity = _snapshot_clock_identity(value)

    @property
    def grandmaster_clock_class(self):
        if not hasattr(self, '_grandmaster_clock_class'):
            return 0
        return self._grandmaster_clock_class

    @grandmaster_clock_class.setter
    def grandmaster_clock_class(self, value):
        if not isinstance(value, int):
            raise TypeError("grandmaster_clock_class must be an integer")
        self._grandmaster_clock_class = value

    @property
    def grandmaster_clock_accuracy(self):
        if not hasattr(self, '_grandmaster_clock_accuracy'):
            return 0
        return self._grandmaster_clock_accuracy

    @grandmaster_clock_accuracy.setter
    def grandmaster_clock_accuracy(self, value):
        if not isinstance(value, int):
            raise TypeError("grandmaster_clock_accuracy must be an integer")
        self._grandmaster_clock_accuracy = value

    @property
    def grandmaster_offset_scaled_log_variance(self):
        if not hasattr(self, '_grandmaster_offset_scaled_log_variance'):
            return 0
        return self._grandmaster_offset_scaled_log_variance

    @grandmaster_offset_scaled_log_variance.setter
    def grandmaster_offset_scaled_log_variance(self, value):
        if not isinstance(value, int):
            raise TypeError("grandmaster_offset_scaled_log_variance must be an integer")
        self._grandmaster_offset_scaled_log_variance = value

    @property
    def grandmaster_priority1(self):
        if not hasattr(self, '_grandmaster_priority1'):
            return 0
        return self._grandmaster_priority1

    @grandmaster_priority1.setter
    def grandmaster_priority1(self, value):
        if not isinstance(value, int):
            raise TypeError("grandmaster_priority1 must be an integer")
        self._grandmaster_priority1 = value

    @property
    def grandmaster_priority2(self):
        if not hasattr(self, '_grandmaster_priority2'):
            return 0
        return self._grandmaster_priority2

    @grandmaster_priority2.setter
    def grandmaster_priority2(self, value):
        if not isinstance(value, int):
            raise TypeError("grandmaster_priority2 must be an integer")
        self._grandmaster_priority2 = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.PARENT_DATA_SET_t)
        self.parent_port_clock_identity = data.parentPortIdentity.clockIdentity
        self.parent_port_number = data.parentPortIdentity.portNumber
        self.parent_stats = bool(data.flags)
        self.observed_parent_offset_scaled_log_variance = data.observedParentOffsetScaledLogVariance
        self.observed_parent_clock_phase_change_rate = data.observedParentClockPhaseChangeRate
        self.grandmaster_identity = data.grandmasterIdentity
        self.grandmaster_clock_quality.populate(data.grandmasterClockQuality)
        self.grandmaster_priority1 = data.grandmasterPriority1
        self.grandmaster_priority2 = data.grandmasterPriority2

    def to_dict(self):
        return {
            "parent-port-identity": {
                "clock-identity": self.parent_port_clock_identity,
                "port-number": self.parent_port_number
            },
            "parent-stats": self.parent_stats,
            "observed-parent-offset-scaled-log-variance": self.observed_parent_offset_scaled_log_variance,
            "observed-parent-clock-phase-change-rate": self.observed_parent_clock_phase_change_rate,
            "grandmaster-identity": self.grandmaster_identity,
            "grandmaster-clock-quality": self.grandmaster_clock_quality.to_dict(),
            "grandmaster-priority1": self.grandmaster_priority1,
            "grandmaster-priority2": self.grandmaster_priority2
        }

    def __str__(self):
        return (
            "PtpClockParentDs("
            f"parent_port_clock_identity={self.parent_port_clock_identity}, "
            f"parent_port_number={self.parent_port_number}, "
            f"parent_stats={self.parent_stats}, "
            f"observed_parent_offset_scaled_log_variance={self.observed_parent_offset_scaled_log_variance}, "
            f"observed_parent_clock_phase_change_rate={self.observed_parent_clock_phase_change_rate}, "
            f"grandmaster_identity={self.grandmaster_identity}, "
            f"grandmaster_clock_class={self.grandmaster_clock_class}, "
            f"grandmaster_clock_accuracy={self.grandmaster_clock_accuracy}, "
            f"grandmaster_offset_scaled_log_variance={self.grandmaster_offset_scaled_log_variance}, "
            f"grandmaster_priority1={self.grandmaster_priority1}, "
            f"grandmaster_priority2={self.grandmaster_priority2})"
        )


class PtpClockTimePropertiesDs(object):
    def __init__(self):
        pass

    @property
    def current_utc_offset_valid(self):
        if not hasattr(self, '_current_utc_offset_valid'):
            return False
        return self._current_utc_offset_valid

    @current_utc_offset_valid.setter
    def current_utc_offset_valid(self, value):
        if not isinstance(value, bool):
            raise TypeError("current_utc_offset_valid must be a boolean")
        self._current_utc_offset_valid = value

    @property
    def current_utc_offset(self):
        if not hasattr(self, '_current_utc_offset'):
            return 0
        return self._current_utc_offset

    @current_utc_offset.setter
    def current_utc_offset(self, value):
        if not isinstance(value, int):
            raise TypeError("current_utc_offset must be an integer")
        self._current_utc_offset = value

    @property
    def leap59(self):
        if not hasattr(self, '_leap59'):
            return False
        return self._leap59

    @leap59.setter
    def leap59(self, value):
        if not isinstance(value, bool):
            raise TypeError("leap59 must be a boolean")
        self._leap59 = value

    @property
    def leap61(self):
        if not hasattr(self, '_leap61'):
            return False
        return self._leap61

    @leap61.setter
    def leap61(self, value):
        if not isinstance(value, bool):
            raise TypeError("leap61 must be a boolean")
        self._leap61 = value

    @property
    def time_traceable(self):
        if not hasattr(self, '_time_traceable'):
            return False
        return self._time_traceable

    @time_traceable.setter
    def time_traceable(self, value):
        if not isinstance(value, bool):
            raise TypeError("time_traceable must be a boolean")
        self._time_traceable = value

    @property
    def frequency_traceable(self):
        if not hasattr(self, '_frequency_traceable'):
            return False
        return self._frequency_traceable

    @frequency_traceable.setter
    def frequency_traceable(self, value):
        if not isinstance(value, bool):
            raise TypeError("frequency_traceable must be a boolean")
        self._frequency_traceable = value

    @property
    def ptp_timescale(self):
        if not hasattr(self, '_ptp_timescale'):
            return False
        return self._ptp_timescale

    @ptp_timescale.setter
    def ptp_timescale(self, value):
        if not isinstance(value, bool):
            raise TypeError("ptp_timescale must be a boolean")
        self._ptp_timescale = value

    @property
    def time_source(self):
        if not hasattr(self, '_time_source'):
            return 0
        return self._time_source

    @time_source.setter
    def time_source(self, value):
        if not isinstance(value, int):
            raise TypeError("time_source must be an integer")
        self._time_source = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.TIME_PROPERTIES_DATA_SET_t)
        self.current_utc_offset = data.currentUtcOffset
        self.current_utc_offset_valid = ptpmgmt.Message().is_UTCV(data.flags)
        self.leap59 = ptpmgmt.Message().is_LI_59(data.flags)
        self.leap61 = ptpmgmt.Message().is_LI_61(data.flags)
        self.time_traceable = ptpmgmt.Message().is_TTRA(data.flags)
        self.frequency_traceable = ptpmgmt.Message().is_FTRA(data.flags)
        self.ptp_timescale = ptpmgmt.Message().is_PTP(data.flags)
        self.time_source = data.timeSource

    def to_dict(self):
        return {
            "current-utc-offset-valid": self.current_utc_offset_valid,
            "current-utc-offset": self.current_utc_offset,
            "leap59": self.leap59,
            "leap61": self.leap61,
            "time-traceable": self.time_traceable,
            "frequency-traceable": self.frequency_traceable,
            "ptp-timescale": self.ptp_timescale,
            "time-source": self.time_source
        }

    def __str__(self):
        return (
            "PtpClockTimePropertiesDs("
            f"current_utc_offset_valid={self.current_utc_offset_valid}, "
            f"current_utc_offset={self.current_utc_offset}, "
            f"leap59={self.leap59}, "
            f"leap61={self.leap61}, "
            f"time_traceable={self.time_traceable}, "
            f"frequency_traceable={self.frequency_traceable}, "
            f"ptp_timescale={self.ptp_timescale}, "
            f"time_source={self.time_source})"
        )


class PtpPortDs(object):
    def __init__(self, port_number=0):
        self._port_number = port_number
        self._port_state = 0

    @property
    def port_number(self):
        return self._port_number

    @port_number.setter
    def port_number(self, value):
        if not isinstance(value, int):
            raise TypeError("port_number must be an integer")
        self._port_number = value

    @property
    def port_state(self):
        return self._port_state

    @port_state.setter
    def port_state(self, value):
        if not isinstance(value, int):
            raise TypeError("port_state must be an integer")
        self._port_state = value

    @property
    def underlying_interface(self):
        return self._underlying_interface

    @underlying_interface.setter
    def underlying_interface(self, value):
        if not isinstance(value, str):
            raise TypeError("underlying_interface must be a string")
        self._underlying_interface = value

    @property
    def log_min_delay_req_interval(self):
        return self._log_min_delay_req_interval

    @log_min_delay_req_interval.setter
    def log_min_delay_req_interval(self, value):
        if not isinstance(value, int):
            raise TypeError("log_min_delay_req_interval must be an integer")
        self._log_min_delay_req_interval = value

    @property
    def peer_mean_path_delay(self):
        return self._peer_mean_path_delay

    @peer_mean_path_delay.setter
    def peer_mean_path_delay(self, value):
        if not isinstance(value, int):
            raise TypeError("peer_mean_path_delay must be an integer")
        self._peer_mean_path_delay = value

    @property
    def log_announce_interval(self):
        return self._log_announce_interval

    @log_announce_interval.setter
    def log_announce_interval(self, value):
        if not isinstance(value, int):
            raise TypeError("log_announce_interval must be an integer")
        self._log_announce_interval = value

    @property
    def announce_receipt_timeout(self):
        return self._announce_receipt_timeout

    @announce_receipt_timeout.setter
    def announce_receipt_timeout(self, value):
        if not isinstance(value, int):
            raise TypeError("announce_receipt_timeout must be an integer")
        self._announce_receipt_timeout = value

    @property
    def log_sync_interval(self):
        return self._log_sync_interval

    @log_sync_interval.setter
    def log_sync_interval(self, value):
        if not isinstance(value, int):
            raise TypeError("log_sync_interval must be an integer")
        self._log_sync_interval = value

    @property
    def delay_mechanism(self):
        return self._delay_mechanism

    @delay_mechanism.setter
    def delay_mechanism(self, value):
        if not isinstance(value, int):
            raise TypeError("delay_mechanism must be an integer")
        self._delay_mechanism = value

    @property
    def log_min_pdelay_req_interval(self):
        return self._log_min_pdelay_req_interval

    @log_min_pdelay_req_interval.setter
    def log_min_pdelay_req_interval(self, value):
        if not isinstance(value, int):
            raise TypeError("log_min_pdelay_req_interval must be an integer")
        self._log_min_pdelay_req_interval = value

    @property
    def version_number(self):
        return self._version_number

    @version_number.setter
    def version_number(self, value):
        if not isinstance(value, int):
            raise TypeError("version_number must be an integer")
        self._version_number = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.PORT_DATA_SET_t)
        self.port_number = data.portIdentity.portNumber
        self.port_state = data.portState
        # self.underlying_interface = data.underlyingInterface
        self.log_min_delay_req_interval = data.logMinDelayReqInterval
        self.peer_mean_path_delay = data.peerMeanPathDelay.getIntervalInt()
        self.log_announce_interval = data.logAnnounceInterval
        self.announce_receipt_timeout = data.announceReceiptTimeout
        self.log_sync_interval = data.logSyncInterval
        self.delay_mechanism = data.delayMechanism
        self.log_min_pdelay_req_interval = data.logMinPdelayReqInterval
        self.version_number = data.versionNumber

    def to_dict(self):
        return {
            "port-number": self.port_number,
            "port-state": self.port_state,
            # "underlying-interface": self.underlying_interface,
            "log-min-delay-req-interval": self.log_min_delay_req_interval,
            "peer-mean-path-delay": self.peer_mean_path_delay,
            "log-announce-interval": self.log_announce_interval,
            "announce-receipt-timeout": self.announce_receipt_timeout,
            "log-sync-interval": self.log_sync_interval,
            "delay-mechanism": self.delay_mechanism,
            "log-min-pdelay-req-interval": self.log_min_pdelay_req_interval,
            "version-number": self.version_number,
        }

    def __str__(self):
        return (
            "PtpPortDs("
            f"port_number={self.port_number}, "
            f"port_state={self.port_state}, "
            # f"underlying_interface={self.underlying_interface}, "
            f"log_min_delay_req_interval={self.log_min_delay_req_interval}, "
            f"peer_mean_path_delay={self.peer_mean_path_delay}, "
            f"log_announce_interval={self.log_announce_interval}, "
            f"announce_receipt_timeout={self.announce_receipt_timeout}, "
            f"log_sync_interval={self.log_sync_interval}, "
            f"delay_mechanism={self.delay_mechanism}, "
            f"log_min_pdelay_req_interval={self.log_min_pdelay_req_interval}, "
            f"version_number={self.version_number}, "
        )


class PtpClock(object):
    DB_TABLE_PREFIX = "PTP_GROUP"
    DB_TABLE_SEPARATOR = "|"
    DB_TABLE_NAME = "PTP_CONFIG"
    DB_KEY_PREFIX = "ptp"

    def __init__(self, instance_number=0):
        self.updated = False
        self._instance_number = instance_number
        self._default_ds = PtpClockDefaultDs()
        self._current_ds = PtpClockCurrentDs()
        self._parent_ds = PtpClockParentDs()
        self._time_properties_ds = PtpClockTimePropertiesDs()
        self._port_ds_list = []

    @property
    def instance_number(self):
        if not hasattr(self, '_instance_number'):
            return 0
        return self._instance_number

    @instance_number.setter
    def instance_number(self, value):
        if not isinstance(value, int):
            raise TypeError("instance_number must be an integer")
        self._instance_number = value

    @property
    def default_ds(self):
        return self._default_ds

    @property
    def current_ds(self):
        return self._current_ds

    @property
    def parent_ds(self):
        return self._parent_ds

    @property
    def time_properties_ds(self):
        return self._time_properties_ds

    @property
    def port_ds_list(self):
        return self._port_ds_list

    def add_port_ds(self, port_ds):
        is_new_port = True
        for p in self._port_ds_list:
            if p.port_number == port_ds.portIdentity.portNumber:
                is_new_port = False
                port_ds_obj = p

        if is_new_port:
            port_ds_obj = PtpPortDs()
            self._port_ds_list.append(port_ds_obj)
        port_ds_obj.populate(port_ds)
        return port_ds_obj

    def to_dict(self):
        return {
            "instance-number": str(self.instance_number),
            "default-ds": self.default_ds.to_dict(),
            "current-ds": self.current_ds.to_dict(),
            "parent-ds": self.parent_ds.to_dict(),
            "time-properties-ds": self.time_properties_ds.to_dict(),
            "port-ds-list": [ x.to_dict() for x in self._port_ds_list ],
        }

    def write_to_state_db(self, state_db):
        separator = self.DB_TABLE_SEPARATOR
        #table_name = separator.join((self.DB_TABLE_PREFIX, self.DB_TABLE_NAME))
        key = separator.join((self.DB_KEY_PREFIX, str(self.instance_number)))
        table = swsscommon.Table(state_db, self.DB_TABLE_NAME)
        # table = swsscommon.Table(state_db, table_name)
        table.set(key, fvs_from_dict(self.to_dict()))

    def __str__(self):
        return (
            "PtpClock("
            f"instance_number={self.instance_number}, "
            f"default_ds={self.default_ds}, "
            f"current_ds={self.current_ds}, "
            f"parent_ds={self.parent_ds}, "
            f"time_properties_ds={self.time_properties_ds})"
        )


class PtpTimeStatus(object):
    DB_TABLE_PREFIX = "PTP_GROUP"
    DB_TABLE_SEPARATOR = "|"
    DB_TABLE_NAME = "PTP_TIME_STATUS"
    DB_KEY_PREFIX = "ptp"

    def __init__(self, instance=0):
        self.updated = False
        self._instance_id = instance
        self._master_offset = 0
        self._ingress_time = 0
        self._cumulative_scaled_rate_offset = 0
        self._scaled_last_gm_phase_change = 0
        self._gm_time_base_indicator = 0
        self._nanoseconds = 0
        self._fractional_nanoseconds = 0
        self._gm_present = False

    @property
    def master_offset(self):
        return self._master_offset

    @master_offset.setter
    def master_offset(self, value):
        if not isinstance(value, int):
            raise TypeError("master_offset must be an integer")
        self._master_offset = value

    @property
    def ingress_time(self):
        return self._ingress_time

    @ingress_time.setter
    def ingress_time(self, value):
        if not isinstance(value, int):
            raise TypeError("ingress_time must be an integer")
        self._ingress_time = value

    @property
    def cumulative_scaled_rate_offset(self):
        return self._cumulative_scaled_rate_offset

    @cumulative_scaled_rate_offset.setter
    def cumulative_scaled_rate_offset(self, value):
        if not isinstance(value, int):
            raise TypeError("cumulative_scaled_rate_offset must be an integer")
        self._cumulative_scaled_rate_offset = value

    @property
    def scaled_last_gm_phase_change(self):
        return self._scaled_last_gm_phase_change

    @scaled_last_gm_phase_change.setter
    def scaled_last_gm_phase_change(self, value):
        if not isinstance(value, int):
            raise TypeError("scaled_last_gm_phase_change must be an integer")
        self._scaled_last_gm_phase_change = value

    @property
    def gm_time_base_indicator(self):
        return self._gm_time_base_indicator

    @gm_time_base_indicator.setter
    def gm_time_base_indicator(self, value):
        if not isinstance(value, int):
            raise TypeError("gm_time_base_indicator must be an integer")
        self._gm_time_base_indicator = value

    @property
    def gm_time_base_indicator(self):
        return self._gm_time_base_indicator

    @gm_time_base_indicator.setter
    def gm_time_base_indicator(self, value):
        if not isinstance(value, int):
            raise TypeError("gm_time_base_indicator must be an integer")
        self._gm_time_base_indicator = value

    @property
    def nanoseconds(self):
        return self._nanoseconds

    @nanoseconds.setter
    def nanoseconds(self, value):
        if not isinstance(value, int):
            raise TypeError("nanoseconds must be an integer")
        self._nanoseconds = value

    @property
    def fractional_nanoseconds(self):
        return self._fractional_nanoseconds

    @fractional_nanoseconds.setter
    def fractional_nanoseconds(self, value):
        if not isinstance(value, int):
            raise TypeError("fractional_nanoseconds must be an integer")
        self._fractional_nanoseconds = value

    @property
    def gm_present(self):
        if not hasattr(self, '_gm_present'):
            return False
        return self._gm_present

    @gm_present.setter
    def gm_present(self, value):
        if not isinstance(value, bool):
            raise TypeError("gm_present must be a boolean")
        self._gm_present = value

    @property
    def grandmaster_identity(self):
        if not hasattr(self, '_grandmaster_identity'):
            return ''
        return self._grandmaster_identity

    @grandmaster_identity.setter
    def grandmaster_identity(self, value):
        self._grandmaster_identity = _snapshot_clock_identity(value)

    def populate(self, data):
        assert isinstance(data, ptpmgmt.TIME_STATUS_NP_t)
        self.master_offset = data.master_offset
        self.ingress_time = data.ingress_time
        self.cumulative_scaled_rate_offset = data.cumulativeScaledRateOffset
        self.scaled_last_gm_phase_change = data.scaledLastGmPhaseChange
        self.gm_time_base_indicator = data.gmTimeBaseIndicator
        # lastGmPhaseChange nanoseconds are an 80-bit signed value.
        nanoseconds = (int(data.nanoseconds_msb) << 64) | int(data.nanoseconds_lsb)
        if nanoseconds & (1 << 79):
            nanoseconds -= 1 << 80
        self.nanoseconds = nanoseconds
        self.fractional_nanoseconds = data.fractional_nanoseconds
        self.gm_present = bool(data.gmPresent)
        self.grandmaster_identity = data.gmIdentity

    def to_dict(self):
        return {
            "master-offset": self.master_offset,
            "ingress-time": self.ingress_time,
            "cumulative-scaled-rate-offset": self.cumulative_scaled_rate_offset,
            "scaled-last-gm-phase-change": self.scaled_last_gm_phase_change,
            "gm-time-base-indicator": self.gm_time_base_indicator,
            "nanoseconds": self.nanoseconds,
            "fractional-nanoseconds": self.fractional_nanoseconds,
        }

    def write_to_state_db(self, state_db):
        separator = self.DB_TABLE_SEPARATOR
        #table_name = separator.join((self.DB_TABLE_PREFIX, self.DB_TABLE_NAME))
        key = separator.join((self.DB_KEY_PREFIX, str(self._instance_id)))
        # table = swsscommon.Table(state_db, table_name)
        table = swsscommon.Table(state_db, self.DB_TABLE_NAME)
        table.set(key, fvs_from_dict(self.to_dict()))

    def __str__(self):
        return (
            "PtpTimeStatus("
            f"master_offset={self.master_offset}, "
            f"ingress_time={self.ingress_time}, "
            f"cumulative_scaled_rate_offset={self.cumulative_scaled_rate_offset}, "
            f"scaled_last_gm_phase_change={self.scaled_last_gm_phase_change}, "
            f"gm_time_base_indicator={self.gm_time_base_indicator}, "
        )


class PtpPortProperties(object):
    def __init__(self):
        self.updated = False

    @property
    def clock_identity(self):
        if not hasattr(self, '_clock_identity'):
            return ''
        return self._clock_identity

    @clock_identity.setter
    def clock_identity(self, value):
        self._clock_identity = _snapshot_clock_identity(value)

    @property
    def port_number(self):
        if not hasattr(self, '_port_number'):
            return None
        return self._port_number

    @port_number.setter
    def port_number(self, value):
        if not isinstance(value, int):
            raise TypeError("port_number must be an integer")
        self._port_number = value

    @property
    def port_state(self):
        if not hasattr(self, '_port_state'):
            return 0
        return self._port_state

    @port_state.setter
    def port_state(self, value):
        if not isinstance(value, int):
            raise TypeError("port_state must be an integer")
        self._port_state = value

    @property
    def timestamping(self):
        if not hasattr(self, '_timestamping'):
            return 0
        return self._timestamping

    @timestamping.setter
    def timestamping(self, value):
        if not isinstance(value, int):
            raise TypeError("timestamping must be an integer")
        self._timestamping = value

    @property
    def interface(self):
        if not hasattr(self, '_interface'):
            return None
        return self._interface

    @interface.setter
    def interface(self, value):
        if not isinstance(value, str):
            raise TypeError("interface must be a string")
        self._interface = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.PORT_PROPERTIES_NP_t)
        port_identity = data.portIdentity
        if isinstance(port_identity, ptpmgmt.PortIdentity_t):
            self.clock_identity = port_identity.clockIdentity
            self.port_number = port_identity.portNumber
        self.port_state = data.portState
        self.timestamping = data.timestamping
        self.interface = data.interface.string()

class PtpPortPacketStatistics(object):
    DB_TABLE_PREFIX = "PTP_GROUP"
    DB_TABLE_NAME = "PTP_PORT"

    def __init__(self, port_ifname: str):
        self.updated = False
        self.port_ifname = port_ifname

    @property
    def port_identity(self):
        if not hasattr(self, '_port_identity'):
            return None
        return self._port_identity

    @port_identity.setter
    def port_identity(self, value):
        self._port_identity = _snapshot_port_identity(value)

    @property
    def rx_sync(self):
        if not hasattr(self, '_rx_sync'):
            return 0
        return self._rx_sync

    @rx_sync.setter
    def rx_sync(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_sync must be an integer")
        self._rx_sync = value

    @property
    def rx_delay_req(self):
        if not hasattr(self, '_rx_delay_req'):
            return 0
        return self._rx_delay_req

    @rx_delay_req.setter
    def rx_delay_req(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_delay_req must be an integer")
        self._rx_delay_req = value

    @property
    def rx_pdelay_req(self):
        if not hasattr(self, '_rx_pdelay_req'):
            return 0
        return self._rx_pdelay_req

    @rx_pdelay_req.setter
    def rx_pdelay_req(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_pdelay_req must be an integer")
        self._rx_pdelay_req = value

    @property
    def rx_pdelay_resp(self):
        if not hasattr(self, '_rx_pdelay_resp'):
            return 0
        return self._rx_pdelay_resp

    @rx_pdelay_resp.setter
    def rx_pdelay_resp(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_pdelay_resp must be an integer")
        self._rx_pdelay_resp = value

    @property
    def rx_follow_up(self):
        if not hasattr(self, '_rx_follow_up'):
            return 0
        return self._rx_follow_up

    @rx_follow_up.setter
    def rx_follow_up(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_follow_up must be an integer")
        self._rx_follow_up = value

    @property
    def rx_delay_resp(self):
        if not hasattr(self, '_rx_delay_resp'):
            return 0
        return self._rx_delay_resp

    @rx_delay_resp.setter
    def rx_delay_resp(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_delay_resp must be an integer")
        self._rx_delay_resp = value

    @property
    def rx_delay_resp_follow_up(self):
        if not hasattr(self, '_rx_delay_resp_follow_up'):
            return 0
        return self._rx_delay_resp_follow_up

    @rx_delay_resp_follow_up.setter
    def rx_delay_resp_follow_up(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_delay_resp_follow_up must be an integer")
        self._rx_delay_resp_follow_up = value

    @property
    def rx_announce(self):
        if not hasattr(self, '_rx_announce'):
            return 0
        return self._rx_announce

    @rx_announce.setter
    def rx_announce(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_announce must be an integer")
        self._rx_announce = value

    @property
    def rx_signaling(self):
        if not hasattr(self, '_rx_signaling'):
            return 0
        return self._rx_signaling

    @rx_signaling.setter
    def rx_signaling(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_signaling must be an integer")
        self._rx_signaling = value

    @property
    def rx_management(self):
        if not hasattr(self, '_rx_management'):
            return 0
        return self._rx_management

    @rx_management.setter
    def rx_management(self, value):
        if not isinstance(value, int):
            raise TypeError("rx_management must be an integer")
        self._rx_management = value

    @property
    def tx_sync(self):
        if not hasattr(self, '_tx_sync'):
            return 0
        return self._tx_sync

    @tx_sync.setter
    def tx_sync(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_sync must be an integer")
        self._tx_sync = value

    @property
    def tx_delay_req(self):
        if not hasattr(self, '_tx_delay_req'):
            return 0
        return self._tx_delay_req

    @tx_delay_req.setter
    def tx_delay_req(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_delay_req must be an integer")

    @property
    def tx_pdelay_req(self):
        if not hasattr(self, '_tx_pdelay_req'):
            return 0
        return self._tx_pdelay_req

    @tx_pdelay_req.setter
    def tx_pdelay_req(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_pdelay_req must be an integer")
        self._tx_pdelay_req = value

    @property
    def tx_pdelay_resp(self):
        if not hasattr(self, '_tx_pdelay_resp'):
            return 0
        return self._tx_pdelay_resp

    @tx_pdelay_resp.setter
    def tx_pdelay_resp(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_pdelay_resp must be an integer")
        self._tx_pdelay_resp = value

    @property
    def tx_follow_up(self):
        if not hasattr(self, '_tx_follow_up'):
            return 0
        return self._tx_follow_up

    @tx_follow_up.setter
    def tx_follow_up(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_follow_up must be an integer")
        self._tx_follow_up = value

    @property
    def tx_delay_resp(self):
        if not hasattr(self, '_tx_delay_resp'):
            return 0
        return self._tx_delay_resp

    @tx_delay_resp.setter
    def tx_delay_resp(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_delay_resp must be an integer")
        self._tx_delay_resp = value

    @property
    def tx_pdelay_req_follow_up(self):
        if not hasattr(self, '_tx_pdelay_req_follow_up'):
            return 0
        return self._tx_pdelay_req_follow_up

    @tx_pdelay_req_follow_up.setter
    def tx_pdelay_req_follow_up(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_pdelay_req_follow_up must be an integer")
        self._tx_pdelay_req_follow_up = value

    @property
    def tx_announce(self):
        if not hasattr(self, '_tx_announce'):
            return 0
        return self._tx_announce

    @tx_announce.setter
    def tx_announce(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_announce must be an integer")
        self._tx_announce = value

    @property
    def tx_signaling(self):
        if not hasattr(self, '_tx_signaling'):
            return 0
        return self._tx_signaling

    @tx_signaling.setter
    def tx_signaling(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_signaling must be an integer")
        self._tx_signaling = value

    @property
    def tx_management(self):
        if not hasattr(self, '_tx_management'):
            return 0
        return self._tx_management

    @tx_management.setter
    def tx_management(self, value):
        if not isinstance(value, int):
            raise TypeError("tx_management must be an integer")
        self._tx_management = value

    def populate(self, data):
        assert isinstance(data, ptpmgmt.PORT_STATS_NP_t)
        self.port_identity = data.portIdentity
        fields = _port_stats_np_tlv_fields(data)
        self.rx_sync = int(fields["rx_Sync"])
        self.rx_delay_req = int(fields["rx_Delay_Req"])
        self.rx_pdelay_req = int(fields["rx_Pdelay_Req"])
        self.rx_pdelay_resp = int(fields["rx_Pdelay_Resp"])
        self.rx_follow_up = int(fields["rx_Follow_Up"])
        self.rx_delay_resp = int(fields["rx_Delay_Resp"])
        self.rx_delay_resp_follow_up = int(fields["rx_Pdelay_Resp_Follow_Up"])
        self.rx_announce = int(fields["rx_Announce"])
        self.rx_signaling = int(fields["rx_Signaling"])
        self.rx_management = int(fields["rx_Management"])
        self.tx_sync = int(fields["tx_Sync"])
        self.tx_delay_req = int(fields["tx_Delay_Req"])
        self.tx_pdelay_req = int(fields["tx_Pdelay_Req"])
        self.tx_pdelay_resp = int(fields["tx_Pdelay_Resp"])
        self.tx_follow_up = int(fields["tx_Follow_Up"])
        self.tx_delay_resp = int(fields["tx_Delay_Resp"])
        self.tx_pdelay_req_follow_up = int(fields["tx_Pdelay_Resp_Follow_Up"])
        self.tx_announce = int(fields["tx_Announce"])
        self.tx_signaling = int(fields["tx_Signaling"])
        self.tx_management = int(fields["tx_Management"])

    def to_dict(self):
        port_identity = self.port_identity
        result = {
            "rx-sync": self.rx_sync,
            "rx-delay-req": self.rx_delay_req,
            "rx-pdelay-req": self.rx_pdelay_req,
            "rx-pdelay-resp": self.rx_pdelay_resp,
            "rx-follow-up": self.rx_follow_up,
            "rx-delay-resp": self.rx_delay_resp,
            "rx-pdelay-resp-follow-up": self.rx_delay_resp_follow_up,
            "rx-announce": self.rx_announce,
            "rx-signaling": self.rx_signaling,
            "rx-management": self.rx_management,
            "tx-sync": self.tx_sync,
            "tx-delay-req": self.tx_delay_req,
            "tx-pdelay-req": self.tx_pdelay_req,
            "tx-pdelay-resp": self.tx_pdelay_resp,
            "tx-follow-up": self.tx_follow_up,
            "tx-delay-resp": self.tx_delay_resp,
            "tx-pdelay-resp-follow-up": self.tx_pdelay_req_follow_up,
            "tx-announce": self.tx_announce,
            "tx-signaling": self.tx_signaling,
            "tx-management": self.tx_management,
        }
        if isinstance(port_identity, _SnapshotPortIdentity):
            result["port-identity"] = {
                "clock-identity": port_identity.clock_identity,
                "port-number": port_identity.port_number,
            }
        return result

    def __str__(self):
        port_identity = self.port_identity
        if isinstance(port_identity, _SnapshotPortIdentity):
            port_identity = (
                f"{port_identity.clock_identity}-{port_identity.port_number}")
        return (
            "PtpPortPacketStatistics("
            f"instance_id={self._instance_id}, "
            f"port_identity={port_identity}, "
            f"rx_sync={self.rx_sync}, "
            f"rx_delay_req={self.rx_delay_req}, "
            f"rx_pdelay_req={self.rx_pdelay_req}, "
            f"rx_pdelay_resp={self.rx_pdelay_resp}, "
            f"rx_follow_up={self.rx_follow_up}, "
            f"rx_delay_resp={self.rx_delay_resp}, "
            f"rx_delay_resp_follow_up={self.rx_delay_resp_follow_up}, "
            f"rx_announce={self.rx_announce}, "
            f"rx_signaling={self.rx_signaling}, "
            f"rx_management={self.rx_management}, "
            f"tx_sync={self.tx_sync}, "
            f"tx_delay_req={self.tx_delay_req}, "
            f"tx_pdelay_req={self.tx_pdelay_req}, "
            f"tx_pdelay_resp={self.tx_pdelay_resp}, "
            f"tx_follow_up={self.tx_follow_up}, "
            f"tx_delay_resp={self.tx_delay_resp}, "
            f"tx_pdelay_req_follow_up={self.tx_pdelay_req_follow_up}, "
            f"tx_announce={self.tx_announce}, "
            f"tx_signaling={self.tx_signaling}, "
            f"tx_management={self.tx_management})"
        )


class PeriodicTaskThread(Thread):
    """
    A thread that periodically calls some function and can be stopped between the calls.
    """
    def __init__(self, period, stop_event):
        """
        :param int period: Period of calling execute() (in seconds).
        :param Event stop_event: The event that can stop execution of this thread.
        """
        super(PeriodicTaskThread, self).__init__()
        self.period = period
        self._stop_event = stop_event

    def execute(self):
        """
        The task to be periodically called.
        """
        raise NotImplementedError()

    def run(self):
        while not self._stop_event.wait(self.period):
            if self._stop_event.is_set():
                break
            if not self.execute():
                self._stop_event.set()
                break


class SubscriptionRenewalThread(PeriodicTaskThread):
    """
    A thread that periodically renews subscription to SUBSCRIBE_EVENTS_NP.
    """
    def __init__(self, watchdog, stop_event):
        """
        :param PtpTelemetryWatchdog watchdog: The watchdog object.
        :param Event stop_event: The event that can stop execution of this thread.
        """
        assert isinstance(watchdog, PtpTelemetryWatchdog)
        super(SubscriptionRenewalThread, self).__init__(watchdog.subscribe_renewal, stop_event)
        self.watchdog = watchdog

    def execute(self):
        try:
            self.watchdog.subscribe_events()
        except BaseException as e:
            if not isinstance(e, KeyboardInterrupt):
                self.watchdog.print_error("Subscribe error (" + str(type(e)) + "): " + str(e))
            if not isinstance(e, Exception):
                return False
        return True

class SubscriptionTimeStatusThread(PeriodicTaskThread):
    """
    A thread that periodically renews subscription to TIME_STATUS_NP.
    """
    def __init__(self, watchdog, stop_event):
        """
        :param PtpTelemetryWatchdog watchdog: The watchdog object.
        :param Event stop_event: The event that can stop execution of this thread.
        """
        assert isinstance(watchdog, PtpTelemetryWatchdog)
        super(SubscriptionTimeStatusThread, self).__init__(watchdog.time_status_period, stop_event)
        self.watchdog = watchdog

    def execute(self):
        try:
            self.watchdog.subscribe_time_status()
        except BaseException as e:
            if not isinstance(e, KeyboardInterrupt):
                self.watchdog.print_error("Subscribe error (" + str(type(e)) + "): " + str(e))
            if not isinstance(e, Exception):
                return False
        return True


class SubscriptionPortStatsThread(PeriodicTaskThread):
    """
    A thread that periodically renews subscription to PORT_STATS_NP.
    """
    def __init__(self, watchdog, stop_event):
        """
        :param PtpTelemetryWatchdog watchdog: The watchdog object.
        :param Event stop_event: The event that can stop execution of this thread.
        """
        assert isinstance(watchdog, PtpTelemetryWatchdog)
        super(SubscriptionPortStatsThread, self).__init__(watchdog.port_stats_period, stop_event)
        self.watchdog = watchdog

    def execute(self):
        try:
            self.watchdog.subscribe_port_stats()
        except BaseException as e:
            if not isinstance(e, KeyboardInterrupt):
                self.watchdog.print_error("Subscribe error (" + str(type(e)) + "): " + str(e))
            if not isinstance(e, Exception):
                return False
        return True




class PtpTelemetryWatchdog:
    REDIS_TIMEOUT_MS = 0
    """
    A watchdog for PTP sync estimation.
    """
    def __init__(self, socket, ptp_config, log_level=3, subscribe_duration=180, subscribe_renewal=60):
        """
        :param ptpmgmt.SockUnix socket: The (already open) UDS socket used for communication.
        :param ptpmgmt.Config ptp_config: The PTP config to apply to the socket and messages.
        :param int subscribe_duration: The duration to be given to SUBSCRIBE_EVENTS_NP (in seconds).
        :param int subscribe_renewal: How often the event subscription should be renewed. This should be lower than
                                      subscribe_duration.
        """
        assert isinstance(socket, ptpmgmt.SockUnix)
        self.sequence = 0
        self._stop_event = Event()

        self.state_db = swsscommon.DBConnector("STATE_DB", self.REDIS_TIMEOUT_MS, False)
        # self.state_db.connect(self.state_db.STATE_DB)
        self.stats_db = swsscommon.DBConnector("COUNTERS_DB", self.REDIS_TIMEOUT_MS, False)
        # self.stats_db_v2 = swsscommon.SonicV2Connector(use_unix_socket_path=True)
        # self.stats_db_v2.connect(self.stats_db_v2.COUNTERS_DB)

        __namespace_id = os.getenv("NAMESPACE_ID")
        if not __namespace_id:
            self.instance_id = 0
        else:
            self.instance_id = int(__namespace_id)

        self.local_clock_id = None
        self.ptp_config = ptp_config
        self.socket = socket
        self.log_level = log_level
        self.message = ptpmgmt.Message()
        self.message_buffer = ptpmgmt.Buf(1000)
        self.message_lock = Lock()

        self.socket.setDefSelfAddress()
        self.socket.init()
        self.socket.setPeerAddress(self.ptp_config)

        self.subscribe_duration = subscribe_duration
        self.subscribe_renewal = subscribe_renewal
        self.time_status_period = 10
        self.port_stats_period = 10

        self.message.useConfig(self.ptp_config)
        self.ptp_clock_cfg = PtpClock(self.instance_id)
        self.time_status = PtpTimeStatus(self.instance_id)
        self.port_mapping = dict()
        self.port_stats_updated = False
        self.port_stats_dict = dict()

        params = self.message.getParams()
        assert isinstance(params, ptpmgmt.MsgParams)
        self_id = params.self_id
        assert isinstance(self_id, ptpmgmt.PortIdentity_t)
        self_id.portNumber = os.getpid() & 0xFFFF  # set PID of the current process as port number
        params.self_id = self_id
        params.boundaryHops = 0  # do not query neighbors
        self.message.updateParams(params)

    def __del__(self):
        self._stop_event.set()
        self.print_info("Exiting")

    def is_stopped(self):
        """
        Whether operation of the watchdog should be stopped.
        :return: Whether operation of the watchdog should be stopped.
        :rtype: bool
        """
        return self._stop_event.is_set()

    def database_sync(self):
        """
        Sync the database with the current state of the watchdog.
        """
        if self.ptp_clock_cfg.updated:            
            self.ptp_clock_cfg.write_to_state_db(self.state_db)
            self.ptp_clock_cfg.updated = False
            self.print_debug(f"Wrote ptp_clock_cfg to state_db: {pprint.pformat(self.ptp_clock_cfg.to_dict())}")

        if self.time_status.updated:
            self.time_status.write_to_state_db(self.state_db)
            self.time_status.updated = False
            self.print_debug(f"Wrote time_status to state_db: {pprint.pformat(self.time_status.to_dict())}")

        if self.port_stats_updated:
            self.write_port_stats_to_counters_db(self.stats_db)
            self.port_stats_updated = False
            self.print_debug(f"Wrote port_stats to counters_db.")

    def write_port_stats_to_counters_db(self, stats_db):
        for number, port in self.port_stats_dict.items():
            separator = ":"
            table_name = "PTP_PORT"
            key = port.port_ifname
            table = swsscommon.Table(stats_db, table_name)
            table.set(key, fvs_from_dict(port.to_dict()))

    def _log_print(self, level, msg):
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        print("{} {}: {}".format(timestamp, level, msg))

    def print_debug(self, msg):
        """
        Print a debug message.
        :param str msg: The message to print.
        """
        if self.log_level < 1:
            self._log_print("DEBUG", msg)

    def print_info(self, msg):
        """
        Print an info message.
        :param str msg: The message to print.
        """
        if self.log_level < 2:
            self._log_print("INFO", msg)

    def print_warn(self, msg):
        """
        Print a warning message.
        :param str msg: The message to print.
        """
        if self.log_level < 3:
            self._log_print("WARN", msg)

    def print_error(self, msg):
        """
        Print an error message.
        :param str msg: The message to print.
        """
        if self.log_level < 4:
            self._log_print("ERROR", msg)

    def _send_message(self):
        """
        Finish sending self.message via self.socket.
        :raises: RuntimeError If something goes wrong with sending or message building.
        :note: Calling code should already hold self.message_lock.
        """
        if self.is_stopped():
            return

        self.sequence += 1
        err = self.message.build(self.message_buffer, self.sequence)
        if err != ptpmgmt.MNG_PARSE_ERROR_OK:
            raise RuntimeError("build error " + ptpmgmt.Message.err2str_c(err))
        if not self.socket.send(self.message_buffer, self.message.getMsgLen()):
            raise RuntimeError("Could not send buffer")

    def send_get_message(self, tlv_id):
        """
        Send a GET message TLV.
        :param int tlv_id: The TLV id.
        """
        with self.message_lock:
            self.message.setAction(ptpmgmt.GET, tlv_id)
            self._send_message()
        self.print_debug("Sent TLV ID " + str(tlv_id))

    def subscribe_time_status(self):
        """
        Subscribe to TIME_STATUS_NP.
        """
        self.send_get_message(ptpmgmt.TIME_STATUS_NP)

    def subscribe_port_stats(self):
        """
        Subscribe to PORT_STATS_NP.
        """
        self.send_get_message(ptpmgmt.PORT_STATS_NP)

    def subscribe_events(self):
        """
        Subscribe to SUBSCRIBE_EVENTS_NP.
        """
        subscribe_message = ptpmgmt.SUBSCRIBE_EVENTS_NP_t()
        subscribe_message.duration = self.subscribe_duration
        subscribe_message.clearAll()
        subscribe_message.setEvent(ptpmgmt.NOTIFY_TIME_SYNC)
        subscribe_message.setEvent(ptpmgmt.NOTIFY_PORT_STATE)
        subscribe_message.setEvent(ptpmgmt.NOTIFY_PARENT_DATA_SET)
        subscribe_message.setEvent(ptpmgmt.NOTIFY_CMLDS)

        with self.message_lock:
            self.message.setAction(ptpmgmt.SET, ptpmgmt.SUBSCRIBE_EVENTS_NP, subscribe_message)
            self._send_message()
            self.message.clearData()

        self.print_debug("Subscribed events for another " + str(self.subscribe_duration) + " seconds")

    def receive(self, num_bytes):
        """
        Receive and process data from self.socket after knowing there are some.
        :param int num_bytes: Number of bytes to read.
        :note: The read values are stored in self.data.
        """
        with self.message_lock:
            if ptpmgmt.MNG_PARSE_ERROR_OK != self.message.parse(self.message_buffer, num_bytes):
                raise RuntimeError("Could not parse message")

            if not self.check_message_recipient():
                self.print_debug("Received message for a different clock")
                return

            self.print_debug("Received TLV ID: " + str(self.message.getTlvId()))

            if self.message.getTlvId() == ptpmgmt.PORT_DATA_SET:
                data = ptpmgmt.conv_PORT_DATA_SET(self.message.getData())
                assert isinstance(data, ptpmgmt.PORT_DATA_SET_t)
                port_ds = self.ptp_clock_cfg.add_port_ds(data)
                self.ptp_clock_cfg.updated = True
                self.print_debug(f"PORT_DATA_SET: {pprint.pformat(port_ds.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.PARENT_DATA_SET:
                data = ptpmgmt.conv_PARENT_DATA_SET(self.message.getData())
                assert isinstance(data, ptpmgmt.PARENT_DATA_SET_t)
                self.ptp_clock_cfg.parent_ds.populate(data)
                self.ptp_clock_cfg.updated = True
                self.print_debug(f"Received PARENT_DATA_SET: {pprint.pformat(self.ptp_clock_cfg.parent_ds.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.DEFAULT_DATA_SET:
                data = ptpmgmt.conv_DEFAULT_DATA_SET(self.message.getData())
                assert isinstance(data, ptpmgmt.DEFAULT_DATA_SET_t)
                self.ptp_clock_cfg.default_ds.populate(data)
                self.ptp_clock_cfg.updated = True
                self.print_debug(f"Received DEFAULT_DATA_SET: {pprint.pformat(self.ptp_clock_cfg.default_ds.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.CURRENT_DATA_SET:
                data = ptpmgmt.conv_CURRENT_DATA_SET(self.message.getData())
                assert isinstance(data, ptpmgmt.CURRENT_DATA_SET_t)
                self.ptp_clock_cfg.current_ds.populate(data)
                self.ptp_clock_cfg.updated = True
                self.print_debug(f"Received CURRENT_DATA_SET: {pprint.pformat(self.ptp_clock_cfg.current_ds.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.TIME_STATUS_NP:
                data = ptpmgmt.conv_TIME_STATUS_NP(self.message.getData())
                assert isinstance(data, ptpmgmt.TIME_STATUS_NP_t)
                self.time_status.populate(data)
                self.time_status.updated = True
                self.print_debug(f"Received TIME_STATUS_NP: {pprint.pformat(self.time_status.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.TIME_PROPERTIES_DATA_SET:
                data = ptpmgmt.conv_TIME_PROPERTIES_DATA_SET(self.message.getData())
                assert isinstance(data, ptpmgmt.TIME_PROPERTIES_DATA_SET_t)
                self.ptp_clock_cfg.time_properties_ds.populate(data)
                self.ptp_clock_cfg.updated = True
                self.print_debug(f"Received TIME_PROPERTIES_DATA_SET: {pprint.pformat(self.ptp_clock_cfg.time_properties_ds.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.PORT_PROPERTIES_NP:
                data = ptpmgmt.conv_PORT_PROPERTIES_NP(self.message.getData())
                assert isinstance(data, ptpmgmt.PORT_PROPERTIES_NP_t)
                port_info = PtpPortProperties()
                port_info.populate(data)
                # self.print_debug(f"Received PORT_PROPERTIES_NP: {pprint.pformat(port_info.to_dict())}")
                self.port_mapping[port_info.port_number] = port_info
                self.print_debug(f"Mapping port {port_info.port_number} to {port_info.interface}")
            elif self.message.getTlvId() == ptpmgmt.PORT_STATS_NP:
                data = ptpmgmt.conv_PORT_STATS_NP(self.message.getData())
                assert isinstance(data, ptpmgmt.PORT_STATS_NP_t)
                port_number = data.portIdentity.portNumber
                if port_number not in self.port_mapping:
                    self.print_debug(f"Received PORT_STATS_NP for unknown port {port_number}")
                    return
                port_stats_obj = self.port_stats_dict.get(port_number, PtpPortPacketStatistics(self.port_mapping[port_number].interface))
                port_stats_obj.populate(data)
                if port_number not in self.port_stats_dict:
                    self.port_stats_dict[port_number] = port_stats_obj
                self.port_stats_updated = True
                self.print_debug(f"Received PORT_STATS_NP: {pprint.pformat(port_stats_obj.to_dict())}")
            elif self.message.getTlvId() == ptpmgmt.SUBSCRIBE_EVENTS_NP:
                data = ptpmgmt.conv_SUBSCRIBE_EVENTS_NP(self.message.getData())
                assert isinstance(data, ptpmgmt.SUBSCRIBE_EVENTS_NP_t)
                self.print_debug("Received SUBSCRIBE_EVENTS_NP: " + repr(data))
            else:
                self.print_debug("Unknown TLV ID " + str(self.message.getTlvId()))

    def check_message_recipient(self):
        """
        Check the clock and port id to verify the message is for us.
        :return: True if the message should be processed, False otherwise.
        :note: Calling code should already hold self.message_lock.
        """
        peer = self.message.getPeer()
        assert isinstance(peer, ptpmgmt.PortIdentity_t)
        clock_id = peer.clockIdentity
        assert isinstance(clock_id, ptpmgmt.ClockIdentity_t)
        if self.local_clock_id is None:
            self.local_clock_id = clock_id
            self.print_info("Attached to clock " + clock_id.string())
        elif not clock_id.eq(self.local_clock_id):
            self.print_debug("Received message for a different clock " + clock_id.string())
            return False
        return True

    def watch(self):
        """
        The main watchdog loop.
        """
        self.send_get_message(ptpmgmt.DEFAULT_DATA_SET)  # clock identity, number of ports, clock quality, priority1, priority2, clock domain, clock slave only
        self.send_get_message(ptpmgmt.PARENT_DATA_SET)  # gm identity
        self.send_get_message(ptpmgmt.CURRENT_DATA_SET)  # offset, path delay
        self.send_get_message(ptpmgmt.TIME_PROPERTIES_DATA_SET)  # master offset
        self.send_get_message(ptpmgmt.PORT_DATA_SET)  # port state
        self.send_get_message(ptpmgmt.PORT_PROPERTIES_NP)  # port properties

        self.subscribe_events()  # offset, port state

        threads = [
            SubscriptionRenewalThread(self, self._stop_event),
            SubscriptionTimeStatusThread(self, self._stop_event),
            SubscriptionPortStatsThread(self, self._stop_event)
        ]

        [t.start() for t in threads]

        while not self.is_stopped():
            try:
                if self.socket.poll(200):
                    num_bytes = self.socket.rcv(self.message_buffer)
                    self.receive(num_bytes)
                else:
                    self.database_sync()
            except BaseException as e:
                if not isinstance(e, KeyboardInterrupt):
                    self.print_error("Poll error (" + str(type(e)) + "): " + str(e))
                if not isinstance(e, Exception):
                    self._stop_event.set()
                    [t.join() for t in threads]

        self._stop_event.set()
        [t.join() for t in threads]


# CLOCK_DESCRIPTION : port
# USER_DESCRIPTION : clock
# DEFAULT_DATA_SET : clock
    # SLAVE_ONLY :clock
    # PRIORITY1 : clock
    # PRIORITY2 : clock
    # DOMAIN : clock
    # CLOCK_ACCURACY : clock
# CURRENT_DATA_SET : clock
# PARENT_DATA_SET : clock
# TIME_PROPERTIES_DATA_SET : clock

# PORT_DATA_SET : port
    # LOG_ANNOUNCE_INTERVAL : port
    # ANNOUNCE_RECEIPT_TIMEOUT : port
    # LOG_SYNC_INTERVAL : port
    # VERSION_NUMBER : port
# PORT_DATA_SET_NP : port
# TIME : unsupported
# TIME_STATUS_NP : clock
# UTC_PROPERTIES : unsupported
# TRACEABILITY_PROPERTIES : clock
# SUBSCRIBE_EVENTS_NP

# PORT_PROPERTIES_NP : port : local port
# GET
# SET
# RESPONSE

CFG = "/etc/ptp4l/ptp4l.conf"


def main():
    config = ptpmgmt.ConfigFile()
    if not config.read_cfg(CFG):
        raise SystemExit("failed to read config")
    else:
        print("Using config file {}", CFG)

    with contextlib.closing(ptpmgmt.SockUnix()) as sock:
        watchdog = PtpTelemetryWatchdog(sock, config)
        watchdog.watch()

if __name__ == "__main__":
    main()