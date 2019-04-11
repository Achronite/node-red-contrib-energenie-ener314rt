# node-red-contrib-energenie-ener314rt
A node-red module to control the Energenie line of products via the ENER314-RT add-on board for the Raspberry Pi.

https://energenie4u.co.uk/


## Purpose

You can use this node-red module to turn on and off the Energenie smart devices such as adapters, sockets, lights and relays 
on a Raspberry Pi with an ENER314-RT board and node-red.

'Control', 'Monitor' and 'Control & Monitor' radio based devices are supported from the legacy and MiHome range.

There are 2 nodes for switching devices, one is based on the OOK transmission standard which covers all 'Control only' energenie devices, the other
supports switching of FSK/OpenThings 'Control & Monitor' devices (see below for full list).

The number of individual devices this node can control is over 4 million, so it should be suitable for most installations!

NOTE: This module does not currently support the older boards (ENER314/Pi-Mote), the Energenie Wifi sockets or the MiHome hub (see below for the full supported list).


## Getting Started

1) Plug in your ENER314-RT-VER01 board from Energenie onto the 26 pin or 40 pin connector of your Raspberry Pi.

2) Install this module as you would any node-red module using the 'Manage palette' option in Node-Red GUI or by using npm.

3) Perform one-time only setup to **teach** your 'Control' devices to operate with your selected zone code(s) switch number(s) combinations (if applicable): 

* Drag a blue node onto the canvas
* Open the node by double clicking
* If required, add a board config (only done once for all devices), by clicking pencil next to board, and then clicking 'add'
* Set the parameters in the Control node that you wish to uniquely reference your device (following the zone rules below).
* Hold the button on your device until it starts to flash. 
* Click the teach (mortar board) icon in the control node properties.
* The device should learn the zone code being broadcast by the teach request, the light should stop flashing when successful.
* All subsequent calls using the same zone/switch number will cause your device to switch. Pressing the teach button again will cause the device to switch on.

TIP: If you already know the house/zone code assigned, for example to an RF hand controller, you can use that in your node to make the device work with both.

4) Perform one-time only setup to **discover** your 'Monitor' and 'Control & Monitor' devices (if applicable)

* Drag the pink or purple nodes onto the canvas
* Open the node by double clicking
* If required, add a board config (only done once for all devices), by clicking pencil next to board, and then clicking 'add'
* Click 'pencil' icon to add a new device
* The device config node will open and perform an auto-scan; after 10 seconds it returns a list of devices that it has found. I do not believe that you need to get the device to 'join' the network by holding the button.  You can click the search button if the device is not found.  If you find that it is still not working, please raise a bug request.
* Select the device in the 'Devices' drop down
* The Product name, ID and type will populate
* (Optional) Change the 'Name' of the device if desired
* Click 'Add' to finish configuration of the device
* Click 'Done'


## 'Control Only' Zone Rules

* Each Energenie 'Control only' or OOK based device can be assigned to a specifc zone (or house code) and a switch number.
* Each zone is encoded as a 20-bit address (1-1048575 decimal).
* Each zone can contain up to 6 switches (1-6) - NOTE: officially energenie state this is only 4 devices (1-4)
* All devices within the same zone can be switched at the same time using a switch number of '0'.
* A default zone '0' can be used to use Energenie's default zone (0x6C6C6).


## Supported Devices

This node works with all radio devices. It was designed to work with all switchable devices, including devices in the OOK & FSK (OpenThings) ranges.

I have a small selection of energenie devices, so have only tested it with these.  Here is a list of what I believe each node **should** support:

###Control Node (Blue):
* ENER002: Green Button Socket **(tested)**
* ENER010: 4 Gang Extension lead
* MIHO002: Smart Plug with Blue Text
* MIHO007: Double Wall Socket White **(tested)**
* MIHO008: Single Light Switch White
* MIHO014: In-line Controller
* MIHO015: In-line Relay
* MIHO021: Double Wall Socket Nickel
* MIHO022: Double Wall Socket Chrome
* MIHO023: Double Wall Socket Brushed Steel
* MIHO024: Single Light Switch Nickel
* MIHO025: Single Light Switch Chrome
* MIHO026: Single Light Switch Steel

###Control & Monitor Switch Node (Purple):
* MIHO005: Purple MiHome Adapter Plus (switching and monitoring) **(tested)**

###Monitor Node (Pink):
* MIHO004: Pink MiHome Monitor Adapter
* MIHO005: MiHome Adapter Plus (monitoring only) **(tested)**
* MIHO006: House Monitor
* MIHO013: MiHome Heating TRV (monitoring only)
* MIHO032: Motion sensor
* MIHO033: Open sensor

###NOT SUPPORTED:
Specific nodes will be required to control the other 'control & monitor' devices such as the MiHome Heating TRV.  I do not own any of these devices so it is difficult to create code for them.  It *may* be possible to switch the TRV with the Control & Monitor Switch node, but this is untested.


## Processing Monitor Messages

The 'Control' & 'Control & Monitor' nodes emit messages that conform to the OpenThings standard.
I use the *SWITCH_STATE* parameter to set the *node.status* of the node, but there are also other parameters that you can use.

For example the 'Adapter Plus' returns the following parameters in the *msg.payload*:
```
timestamp: <numeric 'epoch based' timestamp, of when message was read>
REAL_POWER: <power in Watts being consumed>
REACTIVE_POWER: <Power in volt-ampere reactive (VAR)>
VOLTAGE: <Power in Volts>            
FREQUENCY: <Radio Frequency in Hz>
SWITCH_STATE: <Device State, 0 = off, 1 = on
```
Other devices may return other parameters which you can use. I have provided generic name and parameter mapping for the known values for received messages.
Connect up a debug node to see what your specific devices output.

A full parameter list can be found in C/src/achronite/openThings.c if required.


## Built With

* [NodeJS](https://nodejs.org/dist/latest-v6.x/docs/api/) - JavaScript runtime built on Chrome's V8 JavaScript engine.
* [Node-RED](http://nodered.org/docs/creating-nodes/) - for wiring together hardware devices, APIs and online services.

## Authors

* **Achronite** - *Node-Red wrappers, javascript and OOK-Send C code* - [Achronite](https://github.com/Achronite/node-red-contrib-energenie-ener314)
* **Energenie** - *Radio C library* - [Energenie](https://github.com/Energenie)
* **David Whale** - *Python implementation* - [whaleygeek](https://github.com/whaleygeek/pyenergenie)


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Bugs and Future Work

The ENER314-RT board is a full receive/transmit radio that is programmable from the SPI interface of the
Raspberry Pi. The board and the underlying 'C' code that this modules uses is capable of supporting all 
of the MiHome monitor devices.

Please raise any bugs, questions or queries using the github issues link below:

https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues


@Achronite - April 2019
