#inherit phygittag
#inherit buildinfo
#include linux-common.inc
DESCRIPTION = "Linux Kernel Phytec common base"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"

inherit kernel
inherit kconfig

LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

GIT_URL = "git://github.com/phytec/linux-phytec-ti.git;protocol=https"
SRC_URI = "${GIT_URL};branch=v5.10.120-phy \
           file://0001-Enable-framebuffer-rotation.patch \
           file://0002-Add-cpu-spi0-muxing-and-qca7k.patch \
           file://0003-Set-mcu_spi-to-reserved-in-device-tree.patch \
           "

PR = "r0.0"

# NOTE: PV must be in the format "x.y.z-.*". It cannot begin with a 'v'.
# NOTE: Keep version in filename in sync with commit id!
SRCREV = "dfa4f01de4f77a6abb39097c49e44ed7e555f9bf"
S = "${WORKDIR}/git"

DEPENDS += "lzop-native lz4-native"

# Pull in the devicetree files into the rootfs
RDEPENDS:${KERNEL_PACKAGE_NAME}-base:append = "\
    kernel-devicetree \
"

# Special configuration for remoteproc/rpmsg IPC modules
module_conf_rpmsg_client_sample = "blacklist rpmsg_client_sample"
module_conf_ti_k3_r5_remoteproc = "softdep ti_k3_r5_remoteproc pre: virtio_rpmsg_bus"
module_conf_ti_k3_dsp_remoteproc = "softdep ti_k3_dsp_remoteproc pre: virtio_rpmsg_bus"
KERNEL_MODULE_PROBECONF += "rpmsg_client_sample ti_k3_r5_remoteproc ti_k3_dsp_remoteproc"

EXTRA_DTC_ARGS += "DTC_FLAGS=-@"
KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT} \
                      ${EXTRA_DTC_ARGS}"

FILES:${KERNEL_PACKAGE_NAME}-devicetree += "/${KERNEL_IMAGEDEST}/*.itb"

INTREE_DEFCONFIG = "phytec_ti_defconfig phytec_ti_platform.config"

COMPATIBLE_MACHINE =  "phyboard-lyra-am62xx"
