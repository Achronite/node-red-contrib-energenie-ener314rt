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
Specific nodes are required to send the correct control signals to other **'control & monitor'** devices.  This version now supports the MiHome Heating thermostatic radiator valve (eTRV), see below.


## Processing Monitor Messages

The **'Monitor'**, **'Control & Monitor'** & **'eTRV'**  nodes receive monitoring information from the devices and emit the received parameter values on their output.  These messages conform to the OpenThings parameter standard.
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

## MiHome Radiator Valve (eTRV) Support

v0.3+ now supports the MiHome Thermostaic Radiator valve (eTRV).
> WARNING: Due to the way the eTRV works there may be a delay from when a command is sent to it being processed by the device. See **eTRV Command Caching** below

### eTRV Commands

| Command | # | Description | .data | Response Msg |
|---|:---:|---|---|:---:|
|EXERCISE_VALVE|163|Send exercise valve command, recommended once a week to calibrate eTRV||DIAGNOSTICS|
|SET_LOW_POWER_MODE|164|This is used to enhance battery life by limiting the hunting of the actuator, ie it limits small adjustments to degree of opening, when the room temperature is close to the *TEMP_SET* point. A consequence of the Low Power mode is that it may cause larger errors in controlling room temperature to the set temperature.|0=Off<br>1=On|No*|
|SET_VALVE_STATE|165|Set valve state|0=Open<br>1=Closed<br>2=Auto (default)|No|
|REQUEST_DIAGNOTICS|166|Request diagnostic data from device, if all is OK it will return 0. Otherwise see additional monitored values for status messages||DIAGNOSTICS|
|IDENTIFY|191|Identify the device by making the green light flash on the selected eTRV for 60 seconds||No|
|SET_REPORTING_INTERVAL|210|Update reporting interval to requested value|300-3600 seconds|No|
|REQUEST_VOLTAGE|226|Report current voltage of the batteries||VOLTAGE|
|TEMP_SET|244|Send new target temperature for eTRV.<br>NOTE: The VALVE_STATE must be set to 'Auto' for this to work.|int|No|

> \* Although this will not auto-report, a subsequent call to *REQUEST_DIAGNOTICS* will confirm the *LOW_POWER_MODE* setting

### eTRV Command Caching
The eTRV reports its temperature at the *SET_REPORTING_INTERVAL* (default 5 minutes). The receiver is activated after each *TEMPERATURE* report to listen for commands. The receiver only remains active for 200ms or until a message is received.

To cater for this hardware limitation the **'eTRV node'** uses command caching. Any command sent using the eTRV node will be held until a TEMPERATURE report is received; at this point the most recent cached message (only 1 is supported) will be sent to the eTRV.  Messages will continue to be resent until they have been succesfully received (indicated by the *Response* command in the above table) or until the number of Retries has reached 0.

The reason that a command may be resent multiple times is due to timing issues.  The ENER314-RT monitoring feature is is due to the polling frequency

The eTRV, unfortunately, has no way of checking that certain commands have been received by the device (indicated by a 'No' in the *Response* column in the above table).  This includes the *TEMP_SET* command!  Any commands that  so this is retried the full number of times.

There is a trade-off between how often the ENER314-RT board should be polled for new messages and the performance of node-red.  Having a short polling frequency will increase the chance of a message being received sooner, but could potentially slow down node-red.

### eTRV Monitor Messages

To support the MiHome Radiator Valve (MIHO013) aka **'eTRV'** in v0.3 and above, additional code has been added to cache the monitor information for these devices.  An example of the values is shown below, only 'known' values are returned when the eTRV regularly reports the TEMPERATURE.  See table for types and determining when field was last updated:
```
{
    "deviceId":3989,
    "mfrId":4,
    "productId":3,
    "timestamp":1567932119,
    "TEMPERATURE":19.7,
    "EXERCISE_VALVE":"success",
    "VALVE_TS":1567927343,
    "DIAGNOSTICS":512,
    "DIAGNOSTICS_TS":1567927343,
    "LOW_POWER_MODE":false,
    "TARGET_C": 10,
    "VOLTAGE": 3.19,
    "VOLTAGE_TS": 1568036414,
    "ERRORS": true,
    "ERROR_TEXT": ...
}
```

|Parameter|Description|Data Type|Update time|
|---|---|---|---|
|command|number of current command being set to eTRV|int|timestamp|
|retries|The number of remaining retries for 'command' to be sent to eTRV>|int|timestamp|
|DIAGNOSTICS|Numeric diagnostic code, see "ERRORS" for interpretation|int|DIAGNOSTIC_TS|
|DIAGNOSTICS_TS|timestamp of when diagnostics were last received|epoch|DIAGNOSTIC_TS|
|ERRORS|true if an error condition has been detected|boolean|DIAGNOSTIC_TS|
|ERROR_TEXT|error information|string|DIAGNOSTIC_TS|
|EXERCISE_VALVE|The result of the *EXERCISE_VALVE* command| success or fail|DIAGNOSTIC_TS|
|LOW_POWER_MODE|eTRV is in low power mode state>|boolean|DIAGNOSTIC_TS|
|TARGET_C|Target temperature in celcius|int|TEMP_SET command|
|TEMPERATURE|The current temperature in celcius|float|timestamp|
|VALVE_STATE|Current valve mode/state| open, closed, auto, error|VALVE_STATE command *or* DIAGNOSTIC_TS on error|
|VALVE_TS|timestamp of when last *EXERCISE_VALVE* took place|epoch|DIAGNOSTIC_TS|
|VOLTAGE|Current battery voltage|float|VOLTAGE_TS|
|VOLTAGE_TS|Tmestamp of when battery voltage was last received|epoch|VOLTAGE_TS|

## Change History
| Version | Change details
|---|---|
0.1.0|Initial Release
0.2.0|Full NPM & node-red catalogue release
0.3.0|Switched to use node.js Native API (N-API) for calling C functions.  Added new node to support MiHome Radiator Valve, but please take into account the 200ms window for receiving commands, which could mean that the valve may not respond to messages immediately (see [issue](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/4)).


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