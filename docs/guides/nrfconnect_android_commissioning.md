# Commissioning nRF Connect Accessory using Android CHIPTool

This article describes how to use [CHIPTool](../../src/android/CHIPTool/README.md) for Android smartphones to commission a Nordic Semiconductor nRF52840 DK running
[nRF Connect Lock Example Application](../../examples/lock-app/nrfconnect/README.md) onto a CHIP-enabled Thread network. The instructions are also valid for [nRF Connect Lighting Example Application](../../examples/lighting-app/nrfconnect/README.md).

Prerequisites:
- 2x nRF52840 DK (PCA10056)
  > :information_source: One nRF52840 DK can be replaced with another [OpenThread Radio Co-Processor](https://openthread.io/platforms/co-processor) compatible device like nRF52840 Dongle
- 1x smartphone with Android 8+
- 1x PC with Ubuntu 20.04 and Docker installed, with a spare Wi-Fi card.<br>
  > :information_source: While the article uses Ubuntu 20.04 as a reference, all the presented steps should be achievable using other popular operating systems.

The essence of the commissioning process is composed of the following pieces:
1. CHIPTool discovers the accessory device over BLE.
2. CHIPTool establishes a secure channel to the device over BLE, sends CHIP operational credentials and Thread provisioning data.
3. The accessory device joins a Thread network.

BLE is only used during the commissioning phase and afterwards IP connectivity between the smartphone and the accessory device is needed to send operational messages. Since a typical smartphone doesn't have Thread radio built-in, extra effort is needed to prepare full-fledged test environment. The article describes how to turn a PC with spare Wi-Fi card and nRF52840 DK into a Thread border router forwarding Wi-Fi packets to the Thread network the accessory device operates in. The below diagram shows connectivity between network components necessary to allow communication between devices running CHIPTool and Lock applications:

```
               +--------------------+
               |    Smartphone      |
     +---------|  Android CHIPTool  |---------+
     |         +--------------------+         |
     |                                        | Wi-Fi
     |                                        |
     |                                +---------------+  Ethernet  +----------+
     |                                |      PC       |------------| Internet |
     |                                +---------------+            +----------+
     |                                        |
 BLE |                                        | USB
     |                                        |
     |                                +---------------+
     |                                |  nRF52840 DK  |
     |                                | OpenThread RCP|
     |                                +---------------+
     |                                        |
     |         +--------------------+         | Thread
     |         |    nRF52840 DK     |         |
     +---------|  Lock Application  |---------+
               +--------------------+
```
## Building and flashing OpenThread RCP firmware

<details>
<summary>Click here to expand</summary>
<br>

> :information_source: In case you want to use nRF52840 Dongle as an OpenThread Radio Co-Processor read [nRF52840 dongle support](https://github.com/openthread/openthread/blob/master/examples/platforms/nrf528xx/nrf52840/README.md#nrf52840-dongle-support-pca10059) and adjust the steps below accordingly.

OpenThread RCP firmware is required to allow a PC to communicate with Thread devices. Complete the following steps to build and flash the RCP firmware onto a nRF52840 DK:

1. Download and install the
[nRF Command Line Tools](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools).
2. Clone OpenThread repository into the current directory:

        $ git clone https://github.com/openthread/openthread.git
        
3. Install OpenThread dependencies:

        $ cd openthread
        $ ./script/bootstrap
4. Set up build environment:

        $ ./bootstrap
5. Build OpenThread for the nRF52840 DK:

        $ make -f examples/Makefile-nrf52840

6. Convert the resulting RCP image to HEX format:

        $ arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-rcp output/nrf52840/bin/ot-rcp.hex
7. Flash the RCP firmware:

        $ nrfjprog --chiperase --program output/nrf52840/bin/ot-rcp.hex --reset
8. Disable Mass Storage feature on the device so that it doesn't [interfere](https://github.com/openthread/openthread/blob/master/examples/platforms/nrf528xx/nrf52840/README.md#mass-storage-device-known-issue) with core RCP functionalities. Note that power-cycle of the device is required to apply the changes. The setting will remain valid even if you flash another firmware onto the device.

        $ JLinkExe
        J-Link>MSDDisable
        Probe configured successfully.
        J-Link>exit


</details>

## Configuring PC as a Thread border router

<details>
<summary>Click here to expand</summary>
<br>
To make your PC work as a Thread border router, you need to complete the following tasks which can be further broken down into smaller steps:

1. Form a Thread network using OpenThread Border Router Docker image and configure IPv6 packet routing to the network.
2. Form a Wi-Fi network.

To form a Thread network, perform the following steps:

1. Create an IPv6 network for an OTBR Docker container:

        $ docker network create --ipv6 --subnet 2001:db8:1::/64 -o com.docker.network.bridge.name=otbr0 otbr
2. Start an OTBR Docker container using the command below with _rcp-dk_ in the last line replaced with the device node name of the nRF52840 DK running the RCP firmware, e.g. _/dev/ttyACM0_:

        $ docker run -it --rm --privileged --network otbr -p 8080:80 \
                --sysctl "net.ipv6.conf.all.disable_ipv6=0 net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1" \
                --volume rcp-dk:/dev/ttyACM0 openthread/otbr --radio-url spinel+hdlc+uart:///dev/ttyACM0
3. Open `http://localhost:8080/` address in a web browser.
4. Click __Form__ button on the left menu.
5. Notice that On-Mesh Prefix is `fd11:22::`. It will be used later to configure IPv6 packet routing.
6. Click __Form__ button at the bottom to form a new Thread network using the default settings.
7. Check the output from `docker network inspect otbr` to learn which IPv6 address was assigned to the OTBR Docker container. Most likely, it will be `2001:db8:1::2`.
8. Run the following command, with `2001:db8:1::2` replaced with the actual address obtained in the previous step, to ensure that packets targeting Thread network nodes are routed via the OTBR Docker container:

        $ sudo ip -6 route add fd11:22::/64 dev otbr0 via 2001:db8:1::2

<br>
To form a Wi-Fi network, perform the following steps:

1. Open Ubuntu settings widget, by running the command:

        $ gnome-control-center
2. Navigate to Wi-Fi settings.
3. Click the 3-dot icon at the title bar and select __Turn On Wi-Fi Hotspot...__ option.
4. Enter your network name and password and click __Turn On__.
5. Run the following command to assign a well-known IPv6 address to the hotspot interface:

        $ nmcli connection modify Hotspot ipv6.addresses 2001:db8:2::1/64
6. Install Routing Advertisement Daemon to enable IPv6 auto-configuration of devices connecting to the hotspot:

        $ sudo apt install radvd
7. To learn the hotspot interface name:

        $ nmcli connection show Hotspot | grep interface-name  
8. Put the following content with _wlo1_ replaced with the hotspot interface name in `/etc/radvd.conf`:

        interface wlo1
        {
          MinRtrAdvInterval 3;
          MaxRtrAdvInterval 4;
          AdvSendAdvert on;
          AdvManagedFlag on;
          prefix 2001:db8:2::/64
          { 
             AdvValidLifetime 14300; 
             AdvPreferredLifetime 14200; 
          };
        };
9. Start `radvd` service:

        $ systemctl start radvd
</details>

## Building and flashing nRF Connect Lock Example Application

Read [nRF Connect Lock Example Application README](../../examples/lock-app/nrfconnect/README.md) to learn how to build and flash the lock firmware onto nRF52840 DK.

## Building and installing Android CHIPTool

<details>
<summary>Click here to expand</summary>
<br>

1. Read [Android CHIPTool README](../../src/android/CHIPTool/README.md) to learn how to build the application for your smartphone.
2. Install `android-tools-adb` package:

        $ sudo apt install android-tools-adb
3. Enable **USB debugging** on the smartphone by following the [article](https://developer.android.com/studio/debug/dev-options). If **Install via USB** option is present on your Android version, turn it on, too.
4. Plug the smartphone into a USB port on your PC.
5. Run the following command with _chip-dir_ replaced with CHIP source directory to install the application:

        $ adb install -r chip-dir/src/android/CHIPTool/app/build/outputs/apk/debug/app-debug.apk
6. Navigate to Android Settings and grant __Camera__ and __Location__ permissions to CHIPTool.
</details>

## Commissioning

<details>
<summary>Click here to expand</summary>
<br>
To prepare the accessory device for commissioning:

1. Attach to the device [CLI](nrfconnect_examples_cli.md) to get access to application logs.
2. Hold **Button 1** for more than 6s to trigger factory reset of the device.
3. Find a message similar to the one below in application logs:

        I: 666[SVR] Copy/paste the below URL in a browser to see the QR Code:
                https://dhrishi.github.io/connectedhomeip/qrcode.html?data=CH%3AI34DV%2A-00%200C9SS0
4. Open the URL in a web browser to have onboarding QR code generated.
5. Push **Button 4** on the device to start BLE advertising.

To commission the accessory device onto the Thread network created in [Configuring PC as a Thread border router](#Configuring-PC-as-a-Thread-border-router) chapter:

1. Enable **Bluetooth** and **Location** services on your smartphone.
2. Connect the smartphone to the Wi-Fi Hotspot created in [Configuring PC as a Thread border router](#Configuring-PC-as-a-Thread-border-router) chapter.
3. Open CHIPTool application.
4. Tap **SCAN QR CODE** button and scan the onboarding QR code.
5. Tap **Rendezvous over BLE** button to initiate the commissioning procedure. You should see a few pop-up messages appearing as the commissioning progresses. A message "Secure channel established. Provisioning" indicates that a secure channel has been established successfully and the accessory device has been sent Thread provisioning data.
</details>

## Sending ZCL commands

<details>
<summary>Click here to expand</summary>
<br>

1. Go back to the main application screen.
2. Tap **LIGHT ON/OFF CLUSTER** button.
3. Verify that the textbox on the next screen is filled in with IP address of the accessory device.
4. Tap **ON** or **OFF** button to lock or unlock the door. Observe that the second LED on the device gets turned on and off as state of the lock changes.

</details>
