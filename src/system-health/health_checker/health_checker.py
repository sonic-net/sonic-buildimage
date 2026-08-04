class HealthChecker(object):
    """
    Base class for health checker. A checker is an object that performs system health check for a particular category,
    it collects and stores information after the check.
    """
    INFO_FIELD_OBJECT_TYPE = 'type'
    INFO_FIELD_OBJECT_STATUS = 'status'
    INFO_FIELD_OBJECT_MSG = 'message'

    STATUS_OK = 'OK'
    STATUS_NOT_OK = 'Not OK'

    summary = STATUS_OK

    def __init__(self):
        self._info = {}

    def reset(self):
        """
        Reset the status of the checker. Called every time before the check.
        :return:
        """
        pass

    def get_category(self):
        """
        Get category of the checker.
        :return: String category
        """
        pass

    def get_info(self):
        """
        Get information of the checker. A checker usually checks a few objects and each object status will be put to
        self._info.
        :return: Check result.
        """
        return self._info

    def check(self, config):
        """
        Perform the check.
        :param config: Health checker configuration.
        :return:
        """
        pass

    def __str__(self):
        return self.__class__.__name__

    def add_info(self, object_name, key, value):
        """
        Add check result for an object.
        :param object_name: Object name.
        :param key: Object attribute name.
        :param value: Object attribute value.
        :return:
        """
        if object_name not in self._info:
            self._info[object_name] = {}

        self._info[object_name][key] = value

    def set_object_not_ok(self, object_type, object_name, message):
        """
        Set that an object is not OK.
        :param object_type: Object type.
        :param object_name: Object name.
        :param message: A message to describe what is wrong with the object.
        :return:
        """
        self.add_info(object_name, self.INFO_FIELD_OBJECT_TYPE, object_type)
        self.add_info(object_name, self.INFO_FIELD_OBJECT_MSG, message)
        self.add_info(object_name, self.INFO_FIELD_OBJECT_STATUS, self.STATUS_NOT_OK)
        HealthChecker.summary = HealthChecker.STATUS_NOT_OK

    def set_object_ok(self, object_type, object_name):
        """
        Set that an object is in good state.
        :param object_type: Object type.
        :param object_name: Object name.
        :return:
        """
        self.add_info(object_name, self.INFO_FIELD_OBJECT_TYPE, object_type)
        self.add_info(object_name, self.INFO_FIELD_OBJECT_MSG, '')
        self.add_info(object_name, self.INFO_FIELD_OBJECT_STATUS, self.STATUS_OK)
