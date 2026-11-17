import math


try:
    integer_types = (int, long)
except NameError:
    integer_types = (int,)


class FanConversionError(ValueError):
    """Raised when declarative fan conversion data or its result is invalid."""


class FanConversion(object):
    """Apply a fixed numerical pipeline described by fan configuration."""

    REQUIRED_FIELDS = (
        'input_offset',
        'multiplier',
        'pre_divide_offset',
        'divisor',
        'output_offset',
        'divide_before_multiply',
        'force_float',
    )
    MAX_ABS_NUMBER = 10 ** 12

    def __init__(self, config):
        if not isinstance(config, dict):
            raise FanConversionError('Fan conversion must be an object')

        unknown_fields = set(config) - set(self.REQUIRED_FIELDS)
        missing_fields = set(self.REQUIRED_FIELDS) - set(config)
        if missing_fields:
            raise FanConversionError(
                'Fan conversion is missing fields: %s' %
                ', '.join(sorted(missing_fields))
            )
        if unknown_fields:
            raise FanConversionError(
                'Fan conversion contains unknown fields: %s' %
                ', '.join(sorted(unknown_fields))
            )

        self.input_offset = self._validate_integer(
            config['input_offset'], 'input_offset'
        )
        self.multiplier = self._validate_integer(
            config['multiplier'], 'multiplier'
        )
        self.pre_divide_offset = self._validate_integer(
            config['pre_divide_offset'], 'pre_divide_offset'
        )
        self.divisor = self._validate_integer(config['divisor'], 'divisor')
        if self.divisor <= 0:
            raise FanConversionError('Fan conversion divisor must be positive')
        self.output_offset = self._validate_integer(
            config['output_offset'], 'output_offset'
        )
        self.divide_before_multiply = self._validate_boolean(
            config['divide_before_multiply'], 'divide_before_multiply'
        )
        self.force_float = self._validate_boolean(
            config['force_float'], 'force_float'
        )
        if self.divide_before_multiply and self.pre_divide_offset:
            raise FanConversionError(
                'Fan conversion pre_divide_offset requires multiplication first'
            )

    @classmethod
    def _validate_integer(cls, value, field):
        if isinstance(value, bool) or not isinstance(value, integer_types):
            raise FanConversionError('Fan conversion %s must be an integer' % field)
        if abs(value) > cls.MAX_ABS_NUMBER:
            raise FanConversionError('Fan conversion %s is out of range' % field)
        return value

    @staticmethod
    def _validate_boolean(value, field):
        if not isinstance(value, bool):
            raise FanConversionError('Fan conversion %s must be a boolean' % field)
        return value

    @classmethod
    def _validate_number(cls, value, value_type):
        if isinstance(value, bool) or not isinstance(value, integer_types + (float,)):
            raise FanConversionError('Fan conversion %s must be numeric' % value_type)
        if isinstance(value, float) and (math.isnan(value) or math.isinf(value)):
            raise FanConversionError('Fan conversion %s must be finite' % value_type)
        if abs(value) > cls.MAX_ABS_NUMBER:
            raise FanConversionError('Fan conversion %s is out of range' % value_type)
        return value

    def convert(self, value):
        value = self._validate_number(value, 'input')
        if self.force_float:
            value = float(value)

        try:
            value += self.input_offset
            if self.divide_before_multiply:
                result = value / self.divisor * self.multiplier
            else:
                result = (
                    value * self.multiplier + self.pre_divide_offset
                ) / self.divisor
            result += self.output_offset
        except (ArithmeticError, OverflowError) as error:
            raise FanConversionError('Fan conversion failed: %s' % error)
        return self._validate_number(result, 'result')


class DeferredFanConversion(object):
    """Hold a fan conversion configuration error until a conversion is attempted.

    Constructing a fan must not fail because of conversion configuration, since
    that would prevent the whole platform chassis from being created. Reporting
    the error when the conversion is used keeps the failure local to fan speed
    operations.
    """

    def __init__(self, field, reason):
        self.field = field
        self.reason = reason

    def convert(self, value):
        raise FanConversionError(
            'FAN.%s is not usable: %s' % (self.field, self.reason)
        )


def build_fan_conversion(fan_plugin_data, field):
    """Return a conversion for field, deferring configuration errors."""
    if not isinstance(fan_plugin_data, dict) or field not in fan_plugin_data:
        return DeferredFanConversion(field, 'not configured')
    try:
        return FanConversion(fan_plugin_data[field])
    except FanConversionError as error:
        return DeferredFanConversion(field, error)
