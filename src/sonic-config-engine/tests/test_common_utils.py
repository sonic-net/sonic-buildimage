import os

import tests.common_utils as utils


def test_sample_output_file_falls_back_to_legacy_path(tmpdir):
    test_dir = str(tmpdir)

    assert utils.get_sample_output_file(test_dir, "output.json") == os.path.join(
        test_dir, "sample_output", "output.json"
    )


def test_sample_output_file_prefers_versioned_path(tmpdir):
    test_dir = str(tmpdir)
    versioned_dir = os.path.join(
        test_dir, "sample_output", utils.PYvX_DIR
    )
    os.makedirs(versioned_dir)
    versioned = os.path.join(versioned_dir, "output.json")
    with open(versioned, "w"):
        pass

    assert utils.get_sample_output_file(test_dir, "output.json") == versioned


def test_sample_output_file_prefers_custom_path(tmpdir):
    test_dir = str(tmpdir)
    custom_dir = os.path.join(
        test_dir, "sample_output", "custom", utils.PYvX_DIR
    )
    versioned_dir = os.path.join(
        test_dir, "sample_output", utils.PYvX_DIR
    )
    os.makedirs(custom_dir)
    os.makedirs(versioned_dir)
    custom = os.path.join(custom_dir, "output.json")
    versioned = os.path.join(versioned_dir, "output.json")
    with open(custom, "w"):
        pass
    with open(versioned, "w"):
        pass

    assert utils.get_sample_output_file(test_dir, "output.json") == custom
