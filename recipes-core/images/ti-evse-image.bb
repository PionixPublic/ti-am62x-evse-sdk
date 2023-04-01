require recipes-core/images/everest-basecamp.bb

SUMMARY = "EVerest image for TI AC charging development kit"

LICENSE = "MIT"

CORE_IMAGE_EXTRA_INSTALL += "\
	nodered-flows-ti \
	everest-ti-evse-modules \
	everest-ti-evse-config \
        wl18xx-fw \
        python3-pip \
        python3-ply \
        python3-cffi \
        python3-asyncio-glib \
        python3-cryptography \
        python3-psutil \
        python3-netifaces \
        python3-dateutil \
        python3-iso15118 \
	"

# missing python3 packages:
# pip3 install environs pydantic aiofile py4j

#IMAGE_ROOTFS_SIZE ?= "8192"
#IMAGE_ROOTFS_EXTRA_SPACE_append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "" ,d)}"
IMAGE_ROOTFS_EXTRA_SPACE= "1000000"
