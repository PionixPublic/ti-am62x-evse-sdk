DESCRIPTION = "Install node-red flows for TI EVSE"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"
PR = "r1"

SRC_URI = "file://flows.json \
           file://LICENSE \
           file://node_modules.tgz"

S = "${WORKDIR}"

# FIXME (aw): remove absolute paths
FILES:${PN} += "/home/root/.node-red/flows.json \
               /home/root/.node-red/node_modules/*"

do_install() {
        mkdir -p ${D}/home/root/.node-red/
        cp ${S}/flows.json ${D}/home/root/.node-red/
        cp -r ${S}/node_modules ${D}/home/root/.node-red/
        rm -rf ${D}/home/root/.node-red/node_modules/.bin
}
