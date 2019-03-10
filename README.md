# node-red-contrib-energenie-ener314rt
A node-red module to control the Energenie line of products via the ENER314-RT add-on board for the Raspberry Pi.

https://energenie4u.co.uk/


## Purpose

You can use this node-red module to turn on and off the Energenie smart devices such as adapters, sockets, lights and relays 
on a Raspberry Pi with an ENER314-RT board and node-red.

Currently only switching of devices is supported.  'Control' and 'Control & Monitor' devices are supported from the legacy and MiHome range.

There are 2 nodes for switching devices, one is based on the OOK transmission standard which covers all 'Control only' energenie devices, the other
supports switching of FSK/OpenThings 'Control & Monitor' devices (see below for full list).

The number of individual devices this node can control is over 4 million, so it should be suitable for most installations!

NOTE: This module does not currently support the older 'Pi-mote' board or the MiHome hub (see below for the full supported list).


## Getting Started

1) Plug in your ENER314-RT-VER01 board from Energenie onto the 26 pin or 40 pin connector of your Raspberry Pi.

2) Install this module as you would any node-red module using the 'Manage palette' option in Node-Red GUI or by using npm.

3) Perform one-time only setup to teach your 'Control' devices to operate with your selected zone code(s) switch number(s) combinations (if applicable): 

* Create a simple test flow in node-red.  The simplest way is to wire an inject node with payload of boolean 'true' as an input to your new 'ook-switch' node, for example:
```
[
    {
        "id": "448bf248.0f7efc",
        "type": "inject",
        "z": "c914364f.3616e",
        "name": "",
        "topic": "",
        "payload": "true",
        "payloadType": "bool",
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "x": 170,
        "y": 440,
        "wires": [
            [
                "dbacbd8.6f4254"
            ]
        ]
    },
    {
        "id": "dbacbd8.6f4254",
        "type": "ook-switch",
        "z": "c914364f.3616e",
        "name": "My Switch",
        "zone": "123456",
        "switchNum": 1,
        "x": 340,
        "y": 440,
        "wires": [
            []
        ]
    }
]
```
* Set the parameters in the ook-switch node to uniquely reference your device (following the rules below).
* Deploy your flow.
* Hold the button on your device until it starts to flash. 
* Press the button on the inject node to send the 'switch-on' (true) payload.
* The device should then learn the zone code being broadcast by your flow, all subsequent calls using the same zone/switch number will cause your device to switch.

TIP: If you already know the house/zone code assigned, for example to an RF hand controller, you can use that in your node to make the device work with both.

4) Perform one-time only setup to learn your 'Control & Monitor' devices (if applicable)
* TBD


## 'Control Only' OOK Devices - Zone Rules

* Each Energenie 'Control only' or OOK based device can be assigned to a specifc zone (or house code) and a switch number.
* Each zone is encoded as a 20-bit address (1-1048575 decimal).
* Each zone can contain up to 6 switches (1-6) - NOTE: officially energenie state this is only 4 devices (1-4)
* All devices within the same zone can be switched at the same time using a switch number of '0'.
* A default zone '0' can be used to use Energenie's default zone (0x6C6C6).


## Supported Devices

This node works with all switchable (or 'Control') devices, including devices in the OOK & FSK (OpenThings) ranges.

Here is a list of what I believe this node **should** be compatible with:

OOK Node:
* ENER002 Green Button Socket (tested)
* ENER010 4 Gang Extension lead
* MIHO002 Smart Plug with Blue Text
* MIHO007 Double Wall Socket White (tested)
* MIHO008 Single Light Switch White
* MIHO014 In-line Controller
* MIHO015 In-line Relay
* MIHO021 Double Wall Socket Nickel
* MIHO022 Double Wall Socket Chrome
* MIHO023 Double Wall Socket Brushed Steel
* MIHO024 Single Light Switch Nickel
* MIHO025 Single Light Switch Chrome
* MIHO026 Single Light Switch Steel

OpenThings Node:
* NEW: MIHO005: MiHome Adapter Plus (switching only) (tested)

## Built With

* [NodeJS](https://nodejs.org/dist/latest-v6.x/docs/api/) - JavaScript runtime built on Chrome's V8 JavaScript engine.
* [Node-RED](http://nodered.org/docs/creating-nodes/) - for wiring together hardware devices, APIs and online services.

## Authors

* **Energenie** - *Radio C library* - [Energenie](https://github.com/Energenie)
* **David Whale** - *Python implementation* - [whaleygeek](https://github.com/whaleygeek/pyenergenie)
* **Achronite** - *Node-Red wrappers, javascript and OOK-Send C code* - [Achronite](https://github.com/Achronite/node-red-contrib-energenie-ener314)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Bugs and Future Work

The ENER314-RT board is a full receive/transmit radio that is programmable from the SPI interface of the
Raspberry Pi. The board and the underlying 'C' code that this modules uses is capable of supporting all 
of the MiHome monitor devices but nodes for these have yet to be added.

Please raise any bugs, questions or queries using the github issues link below:

https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues


@Achronite - January 2019
