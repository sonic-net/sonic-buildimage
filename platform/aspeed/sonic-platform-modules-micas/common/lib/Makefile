py_install_dir = ${py_sitepkgs_dir}

all: 
	echo "py_install_dir = ${py_install_dir}"

install:
	install -d ${DESTDIR}${py_install_dir}
	cp -r ../lib/* ${DESTDIR}${py_install_dir}/
	rm -rf ${DESTDIR}${py_install_dir}/Makefile

clean:
	echo "do nothing"
