# Configuring OpenThread Radio Co-processor on nRF52840 Dongle

The article describes how to build and program
[OpenThread Radio Co-Processor](https://openthread.io/platforms/co-processor)
(RCP) firmware onto a Nordic Semiconductor
[nRF52840 Dongle](https://www.nordicsemi.com/Software-and-tools/Development-Kits/nRF52840-Dongle).

OpenThread Radio Co-processor design allows to add Thread network capabilities
to devices which don't support Thread natively. Read
[Configuring Thread network on a Linux machine](linux_thread_connectivity.md) to
learn how to add Thread support to a Linux machine.

## Building and programming the RCP firmware onto an nRF52840 Dongle

Run the following commands to build and program the RCP firmware onto an
nRF52840 Dongle:

1.  Clone the OpenThread repository into the current directory:

        $ git clone https://github.com/openthread/openthread.git

2.  Enter the _openthread_ directory:

        $ cd openthread

3.  Install OpenThread dependencies:

        $ ./script/bootstrap

4.  Set up the build environment:

        $ ./bootstrap

5.  Build OpenThread for the nRF52840 Dongle:

         $ make -f examples/Makefile-nrf52840 BOOTLOADER=USB USB=1

    This creates an RCP image at `output/nrf52840/bin/ot-rcp`.

6.  Convert the RCP image to HEX format:

        $ arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-rcp output/nrf52840/bin/ot-rcp.hex

7.  Install
    [nRF Util](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Util):

        $ python3 -m pip install -U nrfutil

8.  Generate the RCP firmware package:

        $ nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
            --application output/nrf52840/bin/ot-rcp.hex \
            --application-version 1 output/nrf52840/bin/ot-rcp.zip

9.  Connect the nRF52840 Dongle to a USB port.

10. Press **Reset** button on the dongle to put it into DFU mode. Red LED on the
    dongle starts blinking.

11. To install the RCP firmware package onto the dongle, run the following
    command with _/dev/ttyACM0_ replaced with the device node name of your
    nRF52840 Dongle:

        $ nrfutil dfu usb-serial -pkg output/nrf52840/bin/ot-rcp.zip -p /dev/ttyACM0
