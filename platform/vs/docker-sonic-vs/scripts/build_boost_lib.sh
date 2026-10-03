#!/bin/bash

# Run script with desired library and version, e.g. to get libboost-serialization1.71.0, run:
# build_boost_lib.sh serialization 1.71.0

script=$(basename "$0")
if [[ -z "$1" || -z "$2" || "$1" == "-h" || "$1" == "--help" ]]; then
    echo "Usage:"
    echo "  ${script} <boost library name> <boost version>"
    echo
    echo "Example:"
    echo "  ${script} serialization 1.71.0"
    echo
    echo "Parameters:"
    echo "  -h,--help   Print this help message"
    exit
fi

libname="$1"
boost_ver="$2"
boost_underscore=$(echo "${boost_ver}" | sed 's/\./_/g')
boostdir="boost_${boost_underscore}"

export DEBVERSION=${boost_ver}
export PACKAGE_NAME="libboost-${libname}"

if [ ! -d ${boostdir}  ]; then
    boost_zip_file="boost_${boost_underscore}.zip" 
    rm -f ${boost_zip_file}
    wget "https://archives.boost.io/release/${boost_ver}/source/${boost_zip_file}"
    unzip ${boost_zip_file}
fi

cd ${boostdir}
rm -rf debian
mkdir -p debian
dch --create -v $DEBVERSION --package $PACKAGE_NAME ""
touch debian

cat > debian/control <<EOF
Source: ${PACKAGE_NAME}
Maintainer: None <none@example.com>
Section: misc
Priority: optional
Standards-Version: 3.9.2
Build-Depends: debhelper (>= 8), cdbs, zlib1g-dev

Package: ${PACKAGE_NAME}
Architecture: amd64
Depends: \${shlibs:Depends}, \${misc:Depends}, ${PACKAGE_NAME}  (= ${DEBVERSION})
Description: ${libname} library for C++

Package: ${PACKAGE_NAME}-dev
Architecture: amd64
Depends: ${PACKAGE_NAME} (= $DEBVERSION)
Description: ${libname} development files
EOF

cat > debian/rules <<EOF
#!/usr/bin/make -f
%:
	dh \$@
override_dh_auto_configure:
	./bootstrap.sh
override_dh_auto_build:
	./b2 --no-cmake-config --with-${libname} link=static,shared -j 1 --prefix=$(pwd)/debian/${PACKAGE_NAME}/usr/
override_dh_auto_install:
	mkdir -p debian/${PACKAGE_NAME}/usr debian/${PACKAGE_NAME}-dev/usr
	./b2 --no-cmake-config --with-${libname} link=static,shared --prefix=$(pwd)/debian/${PACKAGE_NAME}/usr/ install
	mv debian/${PACKAGE_NAME}/usr/include debian/${PACKAGE_NAME}-dev/usr
EOF
echo "9" > debian/compat
debuild -b
