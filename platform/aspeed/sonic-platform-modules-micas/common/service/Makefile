
all: 
	echo "systemd_system_unitdir = ${systemd_system_unitdir}"

install:
	install -d ${DESTDIR}${systemd_system_unitdir}
	install -m 0644 *.service ${DESTDIR}${systemd_system_unitdir}

clean:
	echo "do nothing"
