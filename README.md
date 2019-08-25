# node-red-contrib-energenie-ener314rt
A node-red module to control the Energenie line of products via the ENER314-RT add-on board for the Raspberry Pi.

https://energenie4u.co.uk/


[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-brightgreen.svg)](https://github.com/Achronite/node-red-contrib-energenie-ener314/graphs/commit-activity)
[![Downloads](https://img.shields.io/npm/dm/node-red-contrib-energenie-ener314rt.svg)](https://www.npmjs.com/package/node-red-contrib-energenie-ener314rt)
[![HitCount](http://hits.dwyl.io/achronite/node-red-contrib-energenie-ener314rt.svg)](http://hits.dwyl.io/achronite/node-red-contrib-energenie-ener314rt)
[![Dependencies Status](https://img.shields.io/david/Achronite/node-red-contrib-energenie-ener314rt.svg)](https://david-dm.org/Achronite/node-red-contrib-energenie-ener314rt)
[![Release](https://img.shields.io/github/release-pre/achronite/node-red-contrib-energenie-ener314rt.svg)](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/releases)
[![NPM](https://nodei.co/npm/node-red-contrib-energenie-ener314rt.png)](https://nodei.co/npm/node-red-contrib-energenie-ener314rt/)


## Purpose

You can use this node-red module to control and monitor the Energenie MiHome radio based smart devices such as adapters, sockets, lights and relays 
on a Raspberry Pi with an **ENER314-RT** board installed using node-red (see below for full list).  This is *instead* of operating the devices using a MiHome Gateway, so it works without an internet connection.

**'Control'**, **'Monitor'** and **'Control & Monitor'** radio based devices are supported from the legacy and MiHome range.

There are 5 nodes in total:
* **Blue** for switching '**Control**' (OOK) based devices
* **Pink** for monitoring MiHome **'Monitor'** devices
* **Purple Switch** for monitoring and switching **'Control & Monitor'** devices
* **Purple eTRV** for monitoring and controlling **'Control & Monitor'** MiHome Thermostatic Radiator values (eTRV)
* **Green** for sending any OOK or FSK raw byte array (Advanced node)

The number of individual devices this node can control is over 4 million, so it should be suitable for most installations!

>NOTE: This module does not currently support the older boards (ENER314/Pi-Mote), the Energenie Wifi sockets or the MiHome Gateway (see below for the full supported list).


## Getting Started

1) Plug in your ENER314-RT-VER01 board from Energenie onto the 26 pin or 40 pin connector of your Raspberry Pi.

2) Install this module as you would any node-red module using the 'Manage palette' option in Node-Red GUI or by using npm.

3) Perform one-time only setup to **teach** your **'Control'** devices to operate with your selected zone code(s) switch number(s) combinations (if applicable): 

    * Drag a blue node onto the canvas
    * Open the node by double clicking
    * If required, add a board config (only done once for all devices), by clicking pencil next to board, and then clicking 'add'
    * Set the parameters in the Control node that you wish to uniquely reference your device (following the zone rules below).
    * You can test the zone/device combination by clicking the learn button to switch on the device, or continue these steps to 'teach' your device
    * Hold the button on your device until it starts to flash. 
    * Click the teach (mortar board) icon in the control node properties.
    * The device should learn the zone code being broadcast by the teach request, the light should stop flashing when successful.
    * All subsequent calls using the same zone/switch number will cause your device to switch. Pressing the teach button again will cause the device to switch **on**.

> TIP: If you already know the house/zone code assigned, for example to an RF hand controller, you can use that in your node to make the device work with both.

4) Perform one-time only setup to **discover** your **'Monitor'** and **'Control & Monitor'** devices (if applicable)

    * Drag one of the pink or purple nodes onto the canvas
    * Open the node by double clicking
    * If required, add a board config (only done once for all devices), by clicking pencil next to board, and then clicking 'add'
    * Click 'pencil' icon to add a new device
    * The device config node will open and perform an auto-scan; after 10 seconds it returns a list of devices that it has found. If your device is not found you get it to 'join' the network by holding the button.  You can click the search button if the device is not found.  If you find that it is still not working, please raise a bug request.
    * Select the device in the 'Devices' drop down
    * The Product name, ID and type will populate
    * (Optional) Change the 'Name' of the device if desired
    * Click 'Add' to finish configuration of the device
    * Click 'Done'


## 'Control Only' OOK Zone Rules

* Each Energenie **'Control'** or OOK based device can be assigned to a specifc zone (or house code) and a switch number.
* Each zone is encoded as a 20-bit address (1-1048575 decimal).
* Each zone can contain up to 6 switches (1-6) - NOTE: officially energenie state this is only 4 devices (1-4)
* All devices within the **same** zone can be switched **at the same time** using a switch number of '0'.
* A default zone '0' can be used to use Energenie's default zone (0x6C6C6).


## Supported Devices

This nodes works with all radio devices. It was designed to work with all on/off switchable devices, including devices in the OOK & FSK (OpenThings) ranges.

I've tested the nodes with all devices that I currently own.  Here is a table showing what each node *should* support, and a tag showing if it has been tested (please let me know of any succesful tests, and I'll update the table):

| Device | Description | Control Node (Blue)|Monitor Node (Pink)|Control+Monitor Node (Purple)|Tested OK|
|---|---|:---:|:---:|:---:|:---:|
||**Node Protocol / Type**|*OOK*|*FSK*|*FSK*
|ENER002|Green Button Adapter|x|||x
|ENER010|MiHome 4 gang Multiplug|x
|MIHO002|MiHome Adapter (Blue)|x
|MIHO004|MiHome Energy Monitor (Pink)||x
|MIHO005|MiHome Adapter Plus (Purple)| | x | x|x
|MIHO006|MiHome House Monitor| | x
|MIHO007|MiHome Socket (White)| x|||x
|MIHO008|MiHome Light Switch (White)| x
|MIHO013|MiHome Radiator Valve| | x | use eTRV node | beta
|MIHO014|Single Pole Relay (inline)| x
|MIHO015|MiHome Relay| x
|MIHO021|MiHome Socket (Nickel)|x|||White
|MIHO022|MiHome Socket (Chrome)|x|||White
|MIHO023|MiHome Socket (Steel)|x|||White
|MIHO024|MiHome Light Switch (Nickel)| x
|MIHO025|MiHome Light Switch (Chrome)| x
|MIHO026|MiHome Light Switch (Steel)| x
|MIHO032|MiHome Motion sensor| | x
|MIHO033|MiHome Open Sensor| | x
|MIHO069|MiHome Heating Thermostat | | x | ?
|MIHO089|MiHome Click - Smart Button||x


### NOT SUPPORTED:
Specific nodes are required to send the correct control signals to other **'control & monitor'** devices.  This version now has basic support for the MiHome Heating thermostatic radiator valve (eTRV), but it is a bit temperamental on receiving instruction signals (see issues).  I believe this is caused by timing issues with the receive window on these devices.


## Processing Monitor Messages

The **'Monitor'**, **'Control & Monitor'** & **'eTRV'  nodes receive monitoring information from the devices and emit the received parameter values on their output.  These messages conform to the OpenThings parameter standard.
All OpenThings parameters received from the device are decoded and returned in the ```msg.payload```.  I use the returned *SWITCH_STATE* parameter to set the *node.status* of the C&M nodes to say if it is 'ON' or 'OFF', and the *TEMPERATURE* value is used on the eTRV node to show the current temperature.

For example the 'Adapter Plus' returns the following parameters in the ```msg.payload```:
```
timestamp: <numeric 'epoch based' timestamp, of when message was read>
REAL_POWER: <power in Watts being consumed>
REACTIVE_POWER: <Power in volt-ampere reactive (VAR)>
VOLTAGE: <Power in Volts>            
FREQUENCY: <Radio Frequency in Hz>
SWITCH_STATE: <Device State, 0 = off, 1 = on
```
Other devices will return other parameters which you can use. I have provided parameter name and type mapping for the known values for received messages.
Connect up a debug node to see what your specific devices output.

A full parameter list can be found in C/src/achronite/openThings.c if required.

## Change History
| Version | Change details
|---|---|
0.1.0|Initial Release
0.2.0|Full NPM & node-red catalogue release
0.3.0|Switched to use node.js Native API (N-API) for calling C functions.  Added new node to support MiHome Radiator Valve, but it does not always process commands (see [issue](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/4)).


## Built With

* [NodeJS](https://nodejs.org/dist/latest-v6.x/docs/api/) - JavaScript runtime built on Chrome's V8 JavaScript engine.
* [Node-RED](http://nodered.org/docs/creating-nodes/) - for wiring together hardware devices, APIs and online services.
* [N-API](https://nodejs.org/docs/latest-v10.x/api/n-api.html)- *NEW in v0.3* - Used to wrap C code as a native node.js Addon. N-API is maintained as part of Node.js itself, and produces Application Binary Interface (ABI) stable across all versions of Node.js.

## Authors

* **Achronite** - *Node-Red wrappers, javascript and additional C code for switching, monitoring and locking* - [Achronite](https://github.com/Achronite/node-red-contrib-energenie-ener314)
* **David Whale** - *Radio C library and python implementation* - [whaleygeek](https://github.com/whaleygeek/pyenergenie)
* **Energenie** - *Original C code base* - [Energenie](https://github.com/Energenie)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Bugs and Future Work

Future work is detailed on the [github issues page](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues). Please raise any bugs, questions, queries or enhancements you have using this page.

https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues



@Achronite - August 2019 - v0.3.0 Beta