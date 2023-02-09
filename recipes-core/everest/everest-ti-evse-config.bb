DESCRIPTION = "Install TI config/firmware and systemd service"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

PR = "r1"

INSANE_SKIP:${PN} += "arch"

SRC_URI = "file://config-ti.yaml \
           file://logging.ini \
           file://LICENSE \
           file://am62-mcu-m4f0_0-fw \
           file://rise_v2g.tgz \
           file://everest.service"

S = "${WORKDIR}"

FILES:${PN} += "/etc/everest/config-ti.yaml \
               /etc/everest/logging.ini \
               /lib/firmware/am62-mcu-m4f0_0-fw \
               /usr/libexec/everest/3rd_party \
               ${systemd_system_unitdir}/everest.service"

inherit systemd

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_PACKAGES = "${PN}"

SYSTEMD_SERVICE:${PN} = "everest.service"

do_install() {
        mkdir -p ${D}/etc/everest/
        mkdir -p ${D}/etc/systemd/system
        mkdir -p ${D}/usr/libexec/everest/3rd_party
        cp -r ${S}/rise_v2g ${D}/usr/libexec/everest/3rd_party
        cp ${S}/config-ti.yaml ${D}/etc/everest/config-ti.yaml
        cp ${S}/logging.ini ${D}/etc/everest/logging.ini
        install -d ${D}/lib/firmware
        cp ${S}/am62-mcu-m4f0_0-fw ${D}/lib/firmware
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${WORKDIR}/everest.service ${D}${systemd_system_unitdir}
}
