import pytest


class TestBufferProfile:
    @pytest.mark.parametrize("action", ["drop", "trim"])
    def test_valid_data_lossy(self, yang_model, action):
        data = {
            "sonic-buffer-pool:sonic-buffer-pool": {
                "sonic-buffer-pool:BUFFER_POOL": {
                    "BUFFER_POOL_LIST": [
                        {
                            "name": "egress_lossy_pool",
                            "type": "egress",
                            "mode": "dynamic"
                        }
                    ]
                }
            },
            "sonic-buffer-profile:sonic-buffer-profile": {
                "sonic-buffer-profile:BUFFER_PROFILE": {
                    "BUFFER_PROFILE_LIST": [
                        {
                            "name": "q_lossy_trim_profile",
                            "pool": "egress_lossy_pool",
                            "dynamic_th": "0",
                            "size": "0",
                            "packet_discard_action": action
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "action,error_message", [
            pytest.param('INVALID_VALUE', 'Invalid value', id="invalid-value")
        ]
    )
    def test_neg_packet_discard_action(self, yang_model, action, error_message):
        data = {
            "sonic-buffer-pool:sonic-buffer-pool": {
                "sonic-buffer-pool:BUFFER_POOL": {
                    "BUFFER_POOL_LIST": [
                        {
                            "name": "egress_lossy_pool",
                            "type": "egress",
                            "mode": "dynamic"
                        }
                    ]
                }
            },
            "sonic-buffer-profile:sonic-buffer-profile": {
                "sonic-buffer-profile:BUFFER_PROFILE": {
                    "BUFFER_PROFILE_LIST": [
                        {
                            "name": "q_lossy_trim_profile",
                            "pool": "egress_lossy_pool",
                            "dynamic_th": "0",
                            "size": "0",
                            "packet_discard_action": action
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)
