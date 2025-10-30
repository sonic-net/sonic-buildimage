import pytest


class TestTrimming:
    @pytest.mark.parametrize(
        "queue", [
            pytest.param('6', id="queue-static"),
            pytest.param('dynamic', id="queue-dynamic")
        ]
    )
    @pytest.mark.parametrize(
        "dscp", [
            pytest.param('48', id="dscp-symmetric"),
            pytest.param('from-tc', id="dscp-asymmetric")
        ]
    )
    def test_valid_data(self, yang_model, dscp, queue):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "size": "128",
                        "dscp_value": dscp,
                        "tc_value": "6",
                        "queue_index": queue
                    }
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "size,error_message", [
            pytest.param('-1', 'out of type uint32 min/max bounds', id="min-1"),
            pytest.param('4294967296', 'out of type uint32 min/max bounds', id="max+1")
        ]
    )
    def test_neg_size(self, yang_model, size, error_message):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "size": size
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "dscp,error_message", [
            pytest.param('-1', 'Invalid union value', id="min-1"),
            pytest.param('64', 'Invalid union value', id="max+1")
        ]
    )
    def test_neg_dscp_value(self, yang_model, dscp, error_message):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "dscp_value": dscp
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "tc,error_message", [
            pytest.param('-1', 'uint8 min/max bounds', id="min-1"),
            pytest.param('256', 'uint8 min/max bounds', id="max+1")
        ]
    )
    def test_neg_tc_value(self, yang_model, tc, error_message):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "tc_value": tc
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "queue,error_message", [
            pytest.param('-1', 'Invalid union value', id="min-1"),
            pytest.param('256', 'Invalid union value', id="max+1")
        ]
    )
    def test_neg_queue_index(self, yang_model, queue, error_message):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "queue_index": queue
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)
