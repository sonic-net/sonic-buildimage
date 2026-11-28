import pytest

from .base_image_script_util import build_script_env


@pytest.fixture
def script_env(tmp_path):
    return build_script_env(tmp_path)
