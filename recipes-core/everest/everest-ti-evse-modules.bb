LICENSE = "Apache-2.0"

LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"


inherit cmake

SRC_URI = "git://git@github.com/PionixInternal/tiramisu-stack.git;protocol=ssh;branch=refactor/new_firmware"

S = "${WORKDIR}/git/everest-module"

SRCREV = "${AUTOREV}"
PV = "0.1+git${SRCPV}"

DEPENDS = " \
    evcli-native \
    everest-core \
    ti-rpmsg-char \
    "

EXTRA_OECMAKE += "-DDISABLE_EDM=ON "

INSANE_SKIP:${PN} = "useless-rpaths"
