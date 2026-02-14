class MockBootloader(object):

    def __init__(self, enforce=False):
        self.enforce = enforce

    def get_next_image(self):
        return ""

    def set_fips(self, image, enable):
        self.enforce = enable

    def get_fips(self, image):
        return self.enforce
