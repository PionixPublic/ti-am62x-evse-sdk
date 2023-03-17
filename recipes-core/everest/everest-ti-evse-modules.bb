LICENSE = "Apache-2.0"

LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit cmake

SRC_URI = "git://github.com/PionixPublic/ti-am62x-evse-sdk.git;protocol=https;branch=main"

S = "${WORKDIR}/git/everest-module"

SRCREV = "0f4a9116d47c8c7bfadbb01278bf3cffee5351ee"
PV = "0.1+git${SRCPV}"

DEPENDS = " \
    evcli-native \
    everest-core \
    ti-rpmsg-char \
    "

EXTRA_OECMAKE += "-DDISABLE_EDM=ON "

INSANE_SKIP:${PN} = "useless-rpaths"
