import os


def init_env():
    # Just make sure there is an environment variable CONTAINER_NAME
    # The value of this environment variable is not important
    os.environ["CONTAINER_NAME"] = "some_container"
