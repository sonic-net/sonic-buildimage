from jinja2 import TemplateNotFound


class BufferProfileLookup(object):
    """Check static (speed, cable length) profiles; None means unknown."""

    def __init__(self, environment):
        self.environment = environment
        self.profiles = None
        self.loaded = False

    def __call__(self, speed, cable, required=False):
        if not speed or not cable:
            return None

        if not self.loaded:
            try:
                source, _, _ = self.environment.loader.get_source(
                    self.environment, 'pg_profile_lookup.ini'
                )
            except TemplateNotFound:
                source = ''
            profiles = set()
            for line in source.splitlines():
                fields = line.partition('#')[0].split()
                if len(fields) >= 6 and fields[0].isdigit():
                    profiles.add((fields[0], fields[1]))
            self.profiles = profiles or None
            self.loaded = True

        if self.profiles is None:
            return None
        available = (str(speed), cable) in self.profiles
        if required and not available:
            raise ValueError(
                'No buffer profile configured for speed {} and cable length {}'.format(
                    speed, cable
                )
            )
        return available
