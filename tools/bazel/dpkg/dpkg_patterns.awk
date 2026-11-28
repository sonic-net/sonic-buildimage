# Pull dpkg's path filters out of a dpkg.cfg.d file, into a format that bsdtar can read.
#
# dpkg writes absolute paths, but a tar stores its entries as "./usr/..." and
# bsdtar matches a pattern against the name as stored, so emit both spellings.

BEGIN {
    printf "" > excludes
    printf "" > includes
}

function emit(file,   pattern) {
    pattern = $2
    # Emit `./usr/lib/...`
    print "." pattern > file
    sub(/^\//, "", pattern)
    # Emit `usr/lib/...`
    print pattern > file
}

$1 == "path-exclude" { emit(excludes) }
$1 == "path-include" { emit(includes) }
