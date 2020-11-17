# CHIP nRF Connect nRF52840 Lock Example Application

An example application showing the use
[CHIP](https://github.com/project-chip/connectedhomeip) on the Nordic nRF52840.

<hr>

-   [CHIP nRF52840 Lock Example Application](#chip-nrf52840-lock-example-application)
    -   [Introduction](#introduction)
    -   [Device UI](#device-ui)
    -   [Setting up environment](#setting-up-environment)
        -   [Using Docker container](#using-docker-container)
        -   [Using Native shell](#using-native-shell)
    -   [Building](#building)
    -   [Flashing and debugging](#flashing-and-debugging)
    -   [Accessing the command line](#accessing-the-command-line)

<hr>

<a name="intro"></a>

## Introduction

![nrf52840 DK](../../platform/nrfconnect/doc/images/nrf52840-dk.jpg)

The nRF52840 lock example application provides a working demonstration of a
connected door lock device, built using CHIP, and the Nordic nRF Connect. The
example supports remote access and control of a simulated door lock over a
low-power, 802.15.4 Thread network. It is capable of being paired into an
existing CHIP network along with other CHIP-enabled devices. The example targets
the
[Nordic nRF52840 DK](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK)
development kit, but is readily adaptable to other nRF52840-based hardware.

The lock example is intended to serve both as a means to explore the workings of
CHIP, as well as a template for creating real products based on the Nordic
platform.

The example makes use of the CMake build system to generate the ninja build
script. The build system takes care of invoking the CHIP library build with all
necessary flags exported from the Zephyr environment.

<a name="device-ui"></a>

## Device UI

The example application provides a simple UI that depicts the state of the
device and offers basic user control. This UI is implemented via the
general-purpose LEDs and buttons built in to the nRF52840 DK dev board.

**LED #1** shows the overall state of the device and its connectivity. Four
states are depicted:

-   _Short Flash On (50ms on/950ms off)_ &mdash; The device is in an
    unprovisioned (unpaired) state and is waiting for a commissioning
    application to connect.

*   _Rapid Even Flashing (100ms on/100ms off)_ &mdash; The device is in an
    unprovisioned state and a commissioning application is connected via BLE.

-   _Short Flash Off (950ms on/50ms off)_ &mdash; The device is fully
    provisioned, but does not yet have full network (Thread) or service
    connectivity.

*   _Solid On_ &mdash; The device is fully provisioned and has full network and
    service connectivity.

**Button #1** can be used to initiate a OTA software update as well as to reset
the device to a default state.

Pressing and holding Button #1 for 6 seconds initiates a factory reset. After an
initial period of 3 seconds, all four LED will flash in unison to signal the
pending reset. Holding the button past 6 seconds will cause the device to reset
its persistent configuration and initiate a reboot. The reset action can be
cancelled by releasing the button at any point before the 6 second limit.

**LED #2** shows the state of the simulated lock bolt. When the LED is lit the
bolt is extended (i.e. door locked); when not lit, the bolt is retracted (door
unlocked). The LED will flash whenever the simulated bolt is in motion from one
position to another.

**Button #2** can be used to change the state of the simulated bolt. This can be
used to mimick a user manually operating the lock. The button behaves as a
toggle, swapping the state every time it is pressed.

**Button #3** can be used to start Thread networking using default configuration
which was selected to match OpenThread Border Router default settings and
network credentials.

**Button #4** can be used to start BLE advertisement, which is disabled by
default.

The remaining two LEDs (#3 and #4) are unused.

**NFC** can be used to scan shared tag and get information about device
necessary to perform rendezvous and network provisioning operation.

Tag can be read by bringing NFC poller e.g. smartphone supporting NFC close to
the nRF52840's NFC antenna. Moreover in the reaction on sensing field from the
smartphone CHIP device will start BLE advertisement, what is an alternative way
to trigger this from manually pressing Button 4.

## Setting up environment

First, checkout CHIP repository and sync submodules using the following command:

        $ git submodule update --init

The example requires nRF Connect SDK v1.4.0. You can either install it along
with related tools directly on your system or use a Docker image which comes
with the tools pre-installed.

Note that if you're a macOS user, you won't be able to use the Docker container
to flash the application onto a Nordic board due to
[certain limitations of Docker for macOS](https://docs.docker.com/docker-for-mac/faqs/#can-i-pass-through-a-usb-device-to-a-container),
so it's recommended that you skip to the
[Using native shell](#using-native-shell) instruction.

### Using Docker container

If you don't have nRF Connect SDK installed yet, create a directory where it
should be placed:

        $ mkdir ~/nrfconnect

Download the latest `nordicsemi/nrfconnect-chip` Docker image:

        $ docker pull nordicsemi/nrfconnect-chip

The next command will start a Docker container using the image acquired in the
previous step. Please read the below explanation for all specified arguments
before proceeding.

        $ docker run --rm -it -e RUNAS=$(id -u) -v ~/nrfconnect:/var/ncs -v ~/connectedhomeip:/var/chip \
            -v /dev/bus/usb:/dev/bus/usb --device-cgroup-rule "c 189:* rmw" nordicsemi/nrfconnect-chip

> **Note**:
>
> -   `~/connectedhomeip` should be replaced with an absolute path to CHIP
>     source directory.
> -   `~/nrfconnect` can be replaced with an absolute path to nRF Connect SDK
>     directory in case you have it already installed.
> -   `-v /dev/bus/usb:/dev/bus/usb --device-cgroup-rule 'c 189:* rmw'`
>     parameters can be omitted if you're not planning to flash the example onto
>     hardware. The parameters give the container access to USB devices
>     connected to your computer such as Nordic development kits.
> -   `-e RUNAS=$(id -u)` is needed to start the container session as the
>     current user instead of root.

To checkout or update nRF Connect SDK to the recommended `1.4.0` version, run:

        $ setup --ncs v1.4.0
        /var/ncs repository is empty. Do you wish to check out nRF Connect SDK sources [v1.4.0]? [Y/N] y
        ...
        /var/chip repository is initialized, skipping...

Proceed with the [Building](#building) instruction.

### Using native shell

Follow the
[guide](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_assistant.html#)
to install nRF Connect SDK v1.4.0. Since further in the text we use command-line
tools to build the example, installing SEGGER Embedded Studio is not required.

In case you have an older version of the SDK installed, use the following
commands to update it to the recommended version. Replace `<nrfconnect-dir>`
with a path to the nRF Connect SDK installation directory.

        $ cd <nrfconnect-dir>/nrf
        $ git fetch origin
        $ git checkout v1.4.0
        $ west update

Download and install the
[nRF Command Line Tools](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools).

Download and install [GN meta-build system](https://gn.googlesource.com/gn/).

Initialize environment variables referred to by CHIP and nRF Connect SDK build
scripts. Replace `<nrfconnect-dir>` with a path to the nRF Connect SDK
installation directory. Also replace `<toolchain-dir>` with a path to GNU Arm
Embedded Toolchain.

        $ source <nrfconnect-dir>/zephyr/zephyr-env.sh
        $ export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
        $ export GNUARMEMB_TOOLCHAIN_PATH=<toolchain-dir>

## Building

Navigate to the example's directory:

        $ cd examples/lock-app/nrfconnect

Run the following command to build the example. Replace `<board-name>` with name
of the Nordic board you own, for example `nrf52840dk_nrf52840`.

        $ west build -b <board-name>

You only need to specify the board name on the first build, then it's enough to
run:

        $ west build

To remove all build artifacts:

        $ rm -r build

To build the example with release configuration which disables diagnostic
features like logs and command-line interface:

        $ west build -b <board-name> -- -DOVERLAY_CONFIG=third_party/connectedhomeip/config/nrfconnect/release.conf

<a name="configuring"></a>

## Configuring the example

The Zephyr ecosystem is higly configurable and allows the user to modify many
aspects of the application. The configuration system is based on `Kconfig` and
the settings can be modified using the `menuconfig` utility.

To open the configuration menu, do the following:

        $ cd <example-dir>
        # First time build
        $ west build -b nrf52840dk_nrf52840 -t menuconfig

        # Any subsequent build
        $ west build -t menuconfig

        # Running menuconfig with ninja
        $ cd <example-dir>/build
        $ ninja menuconfig

Changes done with `menuconfig` will be lost, if the `build` directory is
deleted. To make them persistent, save the configuration options in `prj.conf`
file.

<a name="flashing"></a>

## Flashing and debugging

To flash the application onto the device, run the command below in the example's
directory:

        $ west flash

If you have multiple Nordic devices connected, `west` will prompt you to pick
the correct one.

To start the application with a debugger attached to your board, run:

        $ west debug

<a name="accessing-the-command-line"></a>

## Accessing the command line

The application includes a command line interface with support for logs and the
OpenThread commands.

To access it, use any serial terminal program you like, for example `minicom` or
`GNU screen`.

The UART interface is configured for `115200` baud rate.

All OpenThread commands must be prefixed with `ot`, for example
`ot thread start`.
