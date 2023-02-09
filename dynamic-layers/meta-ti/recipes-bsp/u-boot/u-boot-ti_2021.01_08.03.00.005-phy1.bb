require recipes-bsp/u-boot/u-boot-ti.inc

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=5a7450c57ffe5ae63fd732446b988025"

GIT_URL = "git://github.com/phytec/u-boot-phytec-ti.git;protocol=https"
BRANCH = "v2021.01_08.03.00.005-phy"
SRC_URI = "${GIT_URL};branch=${BRANCH} \
           file://0002-Add-dtb-overlays-and-console-rotate.patch \
           "

FILESEXTRAPATHS:prepend := "${THISDIR}/u-boot:"
SRC_URI:append:phyboard-lyra-am62xx = "\
    file://0001-HACK-board-phytec-phycore_am62x-Enable-OLDI0-AUDIO_R.patch \
"
SRC_URI:append:phyboard-lyra-am62xx-k3r5 = "\
    file://0001-HACK-board-phytec-phycore_am62x-Enable-OLDI0-AUDIO_R.patch \
"

PR = "r0"
SRCREV = "71d3014ea58a4efdb847dc6820076880407fb6a3"

SPL_UART_BINARY_k3r5 = "u-boot-spl.bin"

do_deploy:append:k3r5 () {
	mv ${DEPLOYDIR}/tiboot3.bin ${DEPLOYDIR}/tiboot3-r5spl.bin || true
	mv ${DEPLOYDIR}/u-boot-spl.bin ${DEPLOYDIR}/u-boot-spl-r5spl.bin || true
}

COMPATIBLE_MACHINE = "^("
COMPATIBLE_MACHINE .= "phyboard-lyra-am62xx"
COMPATIBLE_MACHINE .= "|phyboard-lyra-am62xx-k3r5"
COMPATIBLE_MACHINE .= ")$"
