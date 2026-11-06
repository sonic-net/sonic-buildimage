localbindir="/usr/local/bin"

all: 
	echo "localbindir = ${localbindir}"

install:
	install -d ${DESTDIR}${localbindir}
	install -m 755 *.py ${DESTDIR}${localbindir}/
	install -m 755 *.sh ${DESTDIR}${localbindir}/
	install -m 755 ./mce_info/event_*.py ${DESTDIR}${localbindir}/
	install -m 755 ./mce_info/*_event_notify_cfg.py ${DESTDIR}${localbindir}/
	install -m 755 ./mce_info/dbus_*.py ${DESTDIR}${localbindir}/
	ln -sf /usr/bin/python3 ${DESTDIR}/usr/bin/python_nos

clean:
	echo "do nothing"
