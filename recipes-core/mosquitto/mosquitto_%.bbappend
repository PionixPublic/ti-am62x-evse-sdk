FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "file://mosquitto.conf"

FILES:${PN} += "${sysconfdir}/mosquitto/mosquitto.conf"

do_install:append() {
    install -m 0644 ${WORKDIR}/mosquitto.conf ${D}${sysconfdir}/mosquitto/mosquitto.conf
}
