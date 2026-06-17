"Assertion helpers for tar archives"

load("@bazel_lib//lib:diff_test.bzl", "diff_test")

def assert_tar(name, tar, expected, inconsistent_sizes = []):
    """Assert that a tar archive contains the expected file listing.

    Args:
        name: name of the test target
        tar: label of the tar archive to check
        expected: label of a file containing the expected listing (one path per line)
                  Note that the contents of this line must have been created and sorted with `bsdtar -tvf | LC_ALL=C sort --key=9`.
        inconsistent_sizes: list of file paths whose sizes may vary across builds.
                  For these files, the size field (column 5) is replaced with `???` and the
                  line is reformatted with single-space separators. Both the actual tar listing
                  and the expected file are normalized, so the expected file can use the original
                  bsdtar format.
    """
    listing = "{}_listing".format(name)

    if inconsistent_sizes:
        awk_condition = " || ".join(['$$9 == "' + f + '"' for f in inconsistent_sizes])

        # This will transform the string to remove all extraneous spaces. However, it's much easier to
        # reason about than the alternatives, like `gensub`.
        normalize_cmd = " | awk '{if (" + awk_condition + ') {$$5 = "???"} print}' + "'"
    else:
        normalize_cmd = ""

    native.genrule(
        name = listing,
        srcs = [tar],
        testonly = True,
        outs = ["{}.listing".format(name)],
        cmd = "$(BSDTAR_BIN) --verbose --list --file $(execpath {}){} | LC_ALL=C sort --key=9 >$@".format(tar, normalize_cmd),
        toolchains = ["@tar.bzl//tar/toolchain:type"],
    )

    if inconsistent_sizes:
        normalized_expected = "{}_expected_normalized".format(name)
        native.genrule(
            name = normalized_expected,
            srcs = [expected],
            testonly = True,
            outs = ["{}.expected_normalized".format(name)],
            cmd = "cat $(execpath {}){} >$@".format(expected, normalize_cmd),
        )
        expected = normalized_expected

    diff_test(
        name = name,
        file1 = listing,
        file2 = expected,
        timeout = "short",
    )
