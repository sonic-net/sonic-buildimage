import pytest


class TestTrimming:
    @pytest.mark.parametrize(
        "index", [
            pytest.param('6', id="static"),
            pytest.param('dynamic', id="dynamic")
        ]
    )
    def test_valid_data(self, yang_model, index):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "size": "128",
                        "dscp_value": "48",
                        "queue_index": index
                    }
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "size,error_message", [
            pytest.param('-1', 'Invalid value', id="min-1"),
            pytest.param('4294967296', 'Invalid value', id="max+1")
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
            pytest.param('-1', 'Invalid value', id="min-1"),
            pytest.param('64', 'Invalid DSCP value', id="max+1")
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
        "index,error_message", [
            pytest.param('-1', 'Invalid value', id="min-1"),
            pytest.param('256', 'Invalid value', id="max+1")
        ]
    )
    def test_neg_queue_index(self, yang_model, index, error_message):
        data = {
            "sonic-trimming:sonic-trimming": {
                "sonic-trimming:SWITCH_TRIMMING": {
                    "GLOBAL": {
                        "queue_index": index
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)
