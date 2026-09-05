import json


def extra_filters():
    """ Declare some custom filters.

        Returns: dict(name = function)
    """
    return dict(
        json=json.dumps,
    )

