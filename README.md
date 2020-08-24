# Scan for BLE devices broadcasting a particular UUID

This utility scans for BLE devices. In particular it scans for "Exposure Notification" packets. If you know how to build stuff, it is just:

	./lescan_covid19 -c

## Dependencies

Very few dependencies are required. The Bluez library is used directly (not communicating over dbus).

* Bluez
* libblepp 

You can install bluez via your package manager, apt for example.

To install libblepp:

	sudo apt install libboost-dev bluez
	git clone https://github.com/edrosten/libblepp
	cd libblepp
	mkdir build
	cd build
	cmake ..
	make
	sudo make install

If you can't install, it's just a `libble++.so` symlink and a `libble++.so.5` file you will need. You might just be able to copy it to your system and be fine.

## Installation

The installation is straightforward

	git clone https://github.com/mrquincle/corona-scanner
	cd corona-scanner
	mkdir build
	cd build
	cmake ..
	make

## Usage

The use is very simple, there is one obligated option (assuming build directory):

	./lescan_covid19 -c

This scans for UUIDs `FD6F`.

## Limitations

The application shows advertisements in the following form:


	Active scanning
	2020-08-24 16:06:24.055802 77:0b:5a:7a:a2:b2 FD6F -67 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:06:24.248835 1f:b9:96:21:03:ed FD6F -94 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:06:24.314686 77:0b:5a:7a:a2:b2 FD6F -73 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:06:24.572803 77:0b:5a:7a:a2:b2 FD6F -83 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:06:24.830676 77:0b:5a:7a:a2:b2 FD6F -91 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:06:25.094867 77:0b:5a:7a:a2:b2 FD6F -82 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:06:25.356697 77:0b:5a:7a:a2:b2 FD6F -81 dBm PLAIN NONCONNECTABLE

You see the RSSI signal. The MAC address is changing so now and then as according to the protocol.

	2020-08-24 16:09:16.576105 4e:4f:5e:fd:16:85 FD6F -64 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:09:16.840269 4e:4f:5e:fd:16:85 FD6F -65 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:09:17.049210 1f:b9:96:21:03:ed FD6F -92 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:09:17.095163 4e:4f:5e:fd:16:85 FD6F -68 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:09:17.615100 4e:4f:5e:fd:16:85 FD6F -67 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:09:18.141078 4e:4f:5e:fd:16:85 FD6F -68 dBm PLAIN NONCONNECTABLE
	2020-08-24 16:09:18.401841 4e:4f:5e:fd:16:85 FD6F -69 dBm PLAIN NONCONNECTABLE

I did hold my phone closer here (signal strengths are now between -70 and -60 dBm) You also see that the one further away, starting with the temporary `1f`, did not yet rotate.

Summarized:

* a date including time up to milliseconds
* a MAC address
* the UUID (3333)
* the RSSI (-29 dBm)
* the type of advertisement (directed, undirected, scannable, or unconnectable)
* how it is found, as "normal" UUID (PLAIN) or in the "overflow" area (OVERFLOW)

It is not possible to know which advertisement channel they have used. You will have to use an Ubertooth device for that. Then you can select a particular channel to listen on. Perhaps you can also hop over channels and display the channel index (37,38,39). If you have three Ubertooth devices you can listen on all channels and not miss in any advertisement.

## Errors

It might be that you will get an error like:

	error 1531465825.596375: Error obtaining HCI device ID
	terminate called after throwing an instance of 'BLEPP::HCIScanner::HCIError'
	  what():  Error obtaining HCI device ID

This means that there is no BLE device available. There are some instructions after the error message about how to reinsert a kernel module, unblock bluetooth via rfkill, and bring up the interface.

    sudo rmmod btusb
    sudo modprobe btusb
    sudo rfkill unblock bluetooth
    sudo hciconfig hci0 up

Another error might be the following:

	error 1531466395.775174: Setting scan parameters: Operation not permitted
	terminate called after throwing an instance of 'BLEPP::HCIScanner::IOError'
	  what():  Setting scan parameters: Operation not permitted

This means that the capabilities of the binary are not such that it is allowed to scan for BLE devices. It will give some instructions again, namely to either use sudo or manually add the capabilities (you will have to do this each time you compile it).

    sudo setcap cap_net_raw+ep lescan_covid19

## Copyright

* Author: Anne van Rossum
* Date: August 24, 2020
* License: Commercial
* Copyrights: Anne van Rossum
