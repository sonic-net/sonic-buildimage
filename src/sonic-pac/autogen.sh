#!/bin/bash

libtoolize --force --copy &&
autoreconf --force --install -I m4
rm -Rf autom4te.cache

