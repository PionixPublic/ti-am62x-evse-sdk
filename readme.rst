⚡️ TI AM62x EVSE SDK 🔌
========================

Organization of this repository
-------------------------------

The repository contains multiple independent branches

* **main**:

  this branch contains the software for the AM62x *MCU* in the `firmware
  <https://github.com/PionixPublic/ti-am62x-evse-sdk/tree/main/firmware>`_
  folder and its board support driver for the `EVerest framework
  <https://github.com/EVerest/EVerest>`_ in the `everest-module
  <https://github.com/PionixPublic/ti-am62x-evse-sdk/tree/main/everest-module>`_
  folder

* **kirkstone**:

  this branch contains the OpenEmbedded layer, which is necessary for building
  a system image containing all the necessary software and drivers in order to
  run a charging station

* **repo-manifest**:

  this branch contains a manifest for the `repo
  <https://gerrit.googlesource.com/git-repo/>`_ and specifies other necessary
  layer dependencies


Building the system image
-------------------------

This section assumes, that you have minimal experience using the `OpenEmbedded
<https://www.openembedded.org>`_ framework.

* install ``repo`` (see https://gerrit.googlesource.com/git-repo/)
* use repo to fetch all layers::

    repo init -u https://github.com/PionixPublic/ti-am62x-evse-sdk.git -b repo-manifest

    repo sync
    # will fetch all repositories

* source the open-embedded environment script::

    # do this in the folder, where you ran repo init
    . poky/oe-init-build-env
    #^- note the space

* to setup the neccessary bitbake layers, replace the file
  ``build/conf/bblayers.conf`` with the following content::

    # POKY_BBLAYERS_CONF_VERSION is increased each time build/conf/bblayers.conf
    # changes incompatibly
    POKY_BBLAYERS_CONF_VERSION = "2"

    BBPATH = "${TOPDIR}"
    BBFILES ?= ""

    BSPDIR := "${@os.path.abspath(os.path.dirname(d.getVar('FILE', True)) + '/../..')}"

    BBLAYERS ?= " \
      ${BSPDIR}/poky/meta \
      ${BSPDIR}/poky/meta-poky \
      ${BSPDIR}/poky/meta-yocto-bsp \
      ${BSPDIR}/meta-arm/meta-arm \
      ${BSPDIR}/meta-arm/meta-arm-toolchain \
      ${BSPDIR}/meta-ti/meta-ti-bsp \
      ${BSPDIR}/meta-ti/meta-ti-extras \
      ${BSPDIR}/meta-ti-evse \
      ${BSPDIR}/meta-everest \
      ${BSPDIR}/meta-flutter \
      ${BSPDIR}/meta-openembedded/meta-oe \
      ${BSPDIR}/meta-openembedded/meta-python \
      ${BSPDIR}/meta-openembedded/meta-networking \
      ${BSPDIR}/meta-clang \
      "

* edit the file ``build/conf/local.conf`` and replace *MACHINE* setting with::

    MACHINE ??= "phyboard-lyra-am62xx"

  and append the following lines at the end::

    DISTRO_FEATURES:append = " systemd"
    DISTRO_FEATURES_BACKFILL_CONSIDERED += "sysvinit"
    VIRTUAL-RUNTIME_init_manager = "systemd"
    VIRTUAL-RUNTIME_initscripts = "systemd-compat-units"

* finally build the *ti-evse-image*::

    bitbake ti-evse-image

* after the image is complete, you can write it to a sd-card::

    xzcat build/tmp/deploy/images/phyboard-lyra-am62xx/ti-evse-image-phyboard-lyra-am62xx.wic.xz | sudo dd of=/dev/sdX bs=4M status=progress; sudo sync

* now you are ready to insert the sd card into the AM62x EVM and boot it as
  described `here <https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/08_03_00_19/exports/docs/linux/How_to_Guides/Hardware_Setup_with_CCS/AM62x_EVM_Hardware_Setup.html>`_
  
If you have a complete Charging development kit you should now be ready to 
charge a car!
By default Basic PWM charging is enabled. EVerest can be configured with the 
config files found in /etc/everest to e.g. change the maximum current,
enable OCPP or enable ISO15118-2 AC charging.

Contact Pionix at contact at pionix.com to learn more about EVerest and ISO15118-2 options without Java!


Building the *MCU* firmware
---------------------------

Note: this is only necessary if you want to modify the MCU firmware.  A
*recent* binary of the firmware is already included in the image.

1.
  Install `CCStudio
  <https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/08_03_00_19/exports/docs/linux/How_to_Guides/Hardware_Setup_with_CCS/AM62x_EVM_Hardware_Setup.html>`_
  and follow its setup instructions.  Furthermore you'll need to install the
  *AM62x MCU+ SDK*.  Look `here
  <https://software-dl.ti.com/mcu-plus-sdk/esd/AM62X/latest/exports/docs/api_guide_am62x/GETTING_STARTED.html>`_
  for instructions on how to do that.

2. Checkout out the MCU firmware from this repository::

    git clone https://github.com/PionixPublic/ti-am62x-evse-sdk.git

3. Start *CCStudio* and import the ``firmware`` folder from the checkout via
   ``File->Import...`` (choose *CCS Projects*)

4. Now you should be ready to modify and compile the firmware.  For
   instructions on how to load and unload the firmware, you might find what you
   are looking for `here
   <https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/08_03_00_19/exports/docs/linux/Foundational_Components_IPC62x.html>`_.
