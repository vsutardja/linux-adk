# Linux ADK

## Foreword

This fork of gibsson's linux-adk is specifically geared towards investigating HID over AOAv2 on Windows, and removes some/all of the other features.

## Introduction

This software aims to turn your GNU/Linux machine into an Android Accessory.

It initializes any Android device connected via USB as detailed on Android website:

http://source.android.com/tech/accessories/aoap/aoa.html

## How to use

```
$ ./linux-adk --help
Linux Accessory Development Kit

usage: ./linux-adk [OPTIONS]
OPTIONS:
	-a, --aoa-max-version
		AOA maximum version to be used. Default is no maximum version.
	-d, --device
		USB device product and vendor IDs. Default is "18d1:4e42".
	-D, --description
		accessory description. Default is "Sample Program".
	-m, --manufacturer
		manufacturer's name. Default is "Google, Inc.".
	-M, --model
		model's name. Default is "AccessoryChat".
	-n, --vernumber
		accessory version number. Default is "1.0".
	-N, --no_app
		option that allows to connect without an Android App (AOA v2.0 only, for Audio and HID).
	-s, --serial
		serial numder. Default is "0000000012345678".
	-u, --url
		accessory url. Default is "https://github.com/gibsson".
	-v, --version
		Show program version and exit.
	-V, --verbose
		Sets libusb verbose mode.
	-h, --help
		Show this help and exit.
```
Example:
```
$ ./linux-adk -d 18d1:4ee7 -a 1 -M "DemoKit" -D "Demo ABS2013"
```

## How to build on Linux

First you need to download the dependencies:
```
$ sudo apt install libusb-1.0-0-dev build-essential
```
As of today (alpha v0.4), `autotools` have not been configured.

Therefore the build is done manually as follow:
```
$ make
```
This software requires the use of [`libusb`](https://github.com/libusb/libusb/releases).

For cross-compiling, several environment variables must be set manually:
```
$ export ARCH=arm
$ export CROSS_COMPILE=<custom_toolchain>
$ export CFLAGS="-I<path_to_libusb_headers>
$ export LDFLAGS="-L<path_to_libusb>
$ make
```

For example, to cross-compile from WSL to 64-bit Windows using MinGW:
```
$ export CROSS_COMPILE=x86_64-w64-mingw32-
$ export CFLAGS="-Ilibusb/include/libusb-1.0"
$ export LDFLAGS="-Llibusb/MinGW64/dll"
$ make
```

## How to build on Windows

1) You need install Visual Studio (tested with v2019):

https://visualstudio.microsoft.com/downloads/

2) Download and build `libusb` for Windows using this solution:

https://github.com/libusb/libusb/blob/master/msvc/libusb_2019.sln

3) Download and build `linux-adk` using this solution:

https://github.com/gibsson/linux-adk/blob/master/msvc/linux-adk.sln

Couple of things to note for `linux-adk.exe` to work properly on Windows:
1) First make sure that `adb` is not running, otherwise `linux-adk` won't be able to get the endpoint.

```
$ adb kill-server
```

2) Once the platform switches to Accessory mode, make sure in the *Device Manager* that the device driver is:

    - *Manufacturer*: WinUsb Device
    - *Model*: ADB Device

3) If you're getting `Error -12 claiming interface...`, you may need to use [`Zadig`](https://zadig.akeo.ie/) to install a generic `WinUSB (v6.1.7600.16385)` driver for Interface 0. Uninstalling the driver also seemed to work, so maybe this isn't necessary...?

When first plugging in your phone, Device Manager should list an `Android Composite ADB Interface`. After it switches to accessory mode, I get one `Android Accessory Interface` under the `Universal Serial Bus devices` group (interface 3) and one under the `Other Devices` group (interface 0).

## License

linux-adk is distributed under the term of the General Public License version 2 (GPLv2)

See gpl-2.0.txt for the full license content

## Changelog

v0.4: Add verbose option to enable libusb debug logs; Add Windows support; Switch to AccessoryChat app by default
v0.3: Remove Audio hack as not supposed to be (more on https://code.google.com/p/android/issues/detail?id=56549)
v0.2: Audio exception when launched in no-app
v0.1: Original commit for ABS2013 demo

## Author

Gary Bisson <bisson.gary@gmail.com>
