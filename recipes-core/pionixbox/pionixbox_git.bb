SUMMARY = "Flutter PIONIX box application"
DESCRIPTION = "Flutter PIONIX box application"
AUTHOR = "PIONIX"
HOMEPAGE = "https://github.com/PionixInternal/pionixbox"
BUGTRACKER = "https://github.com/PionixInternal/pionixbox/issues"
SECTION = "graphics"

LICENSE = "CLOSED"

SRCREV = "4a422eaf4c64cde2ac1c4c7f11056bb99e5ae35a"
SRC_URI = "git://git@github.com/PionixInternal/pionixbox.git;protocol=ssh;branch=kh-ui-fixes \
           file://display-app.service"

S = "${WORKDIR}/git"

inherit systemd

SYSTEMD_AUTO_ENABLE_${PN} = "enable"
SYSTEMD_PACKAGES = "${PN}"

SYSTEMD_SERVICE_${PN} = "display-app.service"

PUBSPEC_APPNAME = "pionixbox"
FLUTTER_APPLICATION_INSTALL_PREFIX = "/flutter"

FLUTTER_BUILD_ARGS = "bundle"

inherit flutter-app

do_install:append() {
    mv ${D}${FLUTTER_INSTALL_DIR}/data/flutter_assets/* ${D}${FLUTTER_INSTALL_DIR}
    mv ${D}${FLUTTER_INSTALL_DIR}/lib/* ${D}${FLUTTER_INSTALL_DIR}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/display-app.service ${D}${systemd_system_unitdir}
}

FILES_${PN} += "${systemd_system_unitdir}/display-app.service"

REQUIRED_DISTRO_FEATURES= " systemd"
