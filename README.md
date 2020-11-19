# node-red-contrib-energenie-ener314rt
A node-red module to control the Energenie MiHome line of products via the ENER314-RT add-on board for the Raspberry Pi.

[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-brightgreen.svg)](https://github.com/Achronite/node-red-contrib-energenie-ener314/graphs/commit-activity)
[![Downloads](https://img.shields.io/npm/dm/node-red-contrib-energenie-ener314rt.svg)](https://www.npmjs.com/package/node-red-contrib-energenie-ener314rt)
[![HitCount](http://hits.dwyl.io/achronite/node-red-contrib-energenie-ener314rt.svg)](http://hits.dwyl.io/achronite/node-red-contrib-energenie-ener314rt)
[![Dependencies Status](https://img.shields.io/david/Achronite/node-red-contrib-energenie-ener314rt.svg)](https://david-dm.org/Achronite/node-red-contrib-energenie-ener314rt)
![node](https://img.shields.io/node/v/node-red-contrib-energenie-ener314rt)
[![Release](https://img.shields.io/github/release-pre/achronite/node-red-contrib-energenie-ener314rt.svg)](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/releases)
[![NPM](https://nodei.co/npm/node-red-contrib-energenie-ener314rt.png)](https://nodei.co/npm/node-red-contrib-energenie-ener314rt/)


[node-red module](https://flows.nodered.org/node/node-red-contrib-energenie-ener314rt)

## Purpose

You can use this node-red module to control and monitor the [Energenie](https://energenie4u.co.uk/) MiHome radio based smart devices such as adapters, sockets, lights, thermostats and relays 
on a Raspberry Pi with an [ENER314-RT](https://energenie4u.co.uk/catalogue/product/ENER314-RT) board installed using node-red (see below for full list).  This is *instead* of operating the devices using a MiHome Gateway, so this node does not require an internet connection.

**'Control'**, **'Monitor'** and **'Control & Monitor'** radio based devices are supported from the legacy and MiHome range.

There are 6 nodes in total:
* **Blue** for switching '**Control**' (OOK) based devices
* **Pink** for monitoring MiHome **'Monitor'** devices
* **Purple Control & Monitor** for monitoring and switching **'Control & Monitor'** OpenThings devices
* **Purple Smart Plug+** for monitoring and switching **'Control & Monitor'** MiHome Smart Plug+ devices
* **Purple eTRV** for monitoring and controlling **'Control & Monitor'** MiHome Thermostatic Radiator valves (eTRV) using command caching
* **Green** for sending any OOK or FSK raw byte array (Advanced node)

The number of individual devices this node can control is over 4 million, so it should be suitable for most installations!

>NOTE: This module does not currently support the older boards (ENER314/Pi-Mote), the Energenie Wifi sockets or the MiHome Gateway (see below for the full supported list).


## Getting Started

1) Plug in your ENER314-RT-VER01 board from Energenie onto the 26 pin or 40 pin connector of your Raspberry Pi.

2) Install this module as you would any node-red module using the 'Manage palette' option in Node-Red GUI or by using npm.

3) If you have any **'Control'** only devices, perform a one-time only setup to **teach** your  devices to operate with your selected zone code(s) switch number(s) combinations: 

* Drag a blue node onto the canvas
* Open the node by double clicking
* If required, add a board config (only done once for all devices), by clicking pencil next to board, and then clicking 'add'
* Set the parameters in the Control node that you wish to uniquely reference your device (following the zone rules below).
* You can test the zone/device combination by clicking the learn button to switch on the device, or continue these steps to 'teach' your device
* Hold the button on your device until it starts to flash. 
* Click the teach (mortar board) icon in the control node properties.
* The device should learn the zone code being broadcast by the teach request, the light should stop flashing when successful.
* All subsequent calls using the same zone/switch number will cause your device to switch.
* Pressing the teach button again will cause the device to switch **on**, and pressing the 'power' button will cause the same device to switch **off**.


> TIP: If you already know the house/zone code assigned, for example to an RF hand controller, you can use that in your node to make the device work with both.

4)  If you have any **'Monitor'** or **'Control & Monitor'** devices perform one-time only setup to **discover** the devices

* Drag the appropriate pink or purple node onto the canvas (see Supported Devices table below, if you are unsure which one to use)
* Open the node by double clicking
* If required, add a board config (only done once for all devices), by clicking pencil next to board, and then clicking 'add'
* Click 'pencil' icon to add a new device
* The device config node will open and perform an auto-scan; after 10 seconds it returns a list of devices that it has found. If your device is not found you can force it to transmit a 'join' request by holding the button on the device for 5 seconds.  You can click the search button as many times as you like if the device is not found.  If you find that it is still not working, please raise a bug request.
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
* If you have a MiHome 4 gang Multiplug, the same zone must be used for controlling all 4 switches


## Supported Devices

These nodes are designed for energenie RF radio devices in the OOK & FSK (OpenThings) ranges.

Here is a table showing what each node *should* support, and a tag showing if it has been tested (please let me know of any succesful tests, and I'll update the table):

| Device | Description | Control Node (Blue)|Monitor Node (Pink)|Control+Monitor Node (Purple)|Tested OK|
|---|---|:---:|:---:|:---:|:---:|
||**Protocol / Type**|*OOK*|*FSK*|*FSK*||
|ENER002|Green Button Adapter|x|||x|
|ENER010|MiHome 4 gang Multiplug|x|||x|
|MIHO002|MiHome Smart Plug (Blue)|x||||
|MIHO004|MiHome Smart Monitor Plug (Pink)||x|||
|MIHO005|MiHome Smart Plug+ (Purple)||x|use Smart Plug+ node|x|
|MIHO006|MiHome House Monitor||x||x|
|MIHO007|MiHome Socket (White)|x|||x|
|MIHO008|MiHome Light Switch (White)|x||||
|MIHO013|MiHome Radiator Valve||x|use eTRV node|x|
|MIHO014|Single Pole Relay (inline)|x||||
|MIHO015|MiHome Relay|x||||
|MIHO021|MiHome Socket (Nickel)|x|||White|
|MIHO022|MiHome Socket (Chrome)|x|||White|
|MIHO023|MiHome Socket (Steel)|x|||White|
|MIHO024|MiHome Light Switch (Nickel)|x|
|MIHO025|MiHome Light Switch (Chrome)|x|
|MIHO026|MiHome Light Switch (Steel)|x|
|MIHO032|MiHome Motion sensor||x||x|
|MIHO033|MiHome Open Sensor||x||x|
|MIHO069|MiHome Heating Thermostat||x|x|o|
|MIHO089|MiHome Click - Smart Button||x|||


### NOT SUPPORTED:
Specific nodes are required to send the correct control signals to some **'control & monitor'** devices.  This version has specific nodes for the MiHome Heating thermostatic radiator valve (eTRV, MIHO013), and the MiHome Smart Plug+ (MIHO004) see below.

For other **mains-powered** devices (for example the MIHO069 Heating Thermostat) you should be able to send any OpenThings Commands to the Control & Monitor device using the **'Control & Monitor'** node.  Please let me know, via [github](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues), if you identify any 'unknown' commands or parameters.


## Processing Monitor Messages

The **'Monitor'**, **'Control & Monitor'**, **'eTRV'** & **'Smart Plug+'**  nodes receive monitoring information from the devices and emit the received parameter values on their output.  These messages conform to the OpenThings parameter standard.
All OpenThings parameters received from the device are decoded and returned in the ```msg.payload```.  I use the returned *SWITCH_STATE* parameter to set the *node.status* of the C&M nodes to say if it is 'ON' or 'OFF', and the *TEMPERATURE* value is used on the eTRV node to show the current temperature.

Some example ```msg.payload```s are shown below. I have provided parameter name and type mapping for the known values for received messages. Connect up a debug node to see what your specific devices output.

A full parameter list can be found in C/src/achronite/openThings.c if required.

### Example msg.payload - Smart Plug+ (MIHO005)
Every 10 seconds:
```
deviceId: <device number>
mfrId: 4
productId: 2
timestamp: <numeric 'epoch based' timestamp, of when message was read>
REAL_POWER: <power in Watts being consumed>
REACTIVE_POWER: <Power in volt-ampere reactive (VAR)>
VOLTAGE: <Power in Volts>            
FREQUENCY: <Radio Frequency in Hz>
SWITCH_STATE: <Device State, 0 = off, 1 = on
```
### Example msg.payload - Whole House Energy Monitor (MIHO006)
```
deviceId: <device number>
mfrId: 4
productId: 5
timestamp: <numeric 'epoch based' timestamp, of when message was read>
APPARENT_POWER: 612
VOLTAGE: 4.566406
CURRENT: 2.179688
```
### Example msg.payload - Motion Sensor (MIHO032)
```
deviceId: <device number>
mfrId: 4
productId: 12
timestamp: <numeric 'epoch based' timestamp, of when message was read>
MOTION_DETECTOR: <Sensor state, 0 = no motion, 1 = motion>
```
### Example msg.payload - Door Sensor (MIHO033)
```
deviceId: <device number>
mfrId: 4
productId: 13
timestamp: <numeric 'epoch based' timestamp, of when message was read>
DOOR_SENSOR: <Sensor state, 0 = open, 1 = closed>
```
### Example msg.payload - Heating Thermostat (MIHO069)
```
deviceId: <device number>
mfrId: 4
productId: 18
timestamp: <numeric 'epoch based' timestamp, of when message was read>
TEMPERATURE: <current temperature in celcius>
REL_HUMIDITY: <Humidity as a percentage>
THERMOSTAT_MODE: <Thermostat mode, 0 = off, 1 = temp controlled, 2= always on>
```

## MiHome Radiator Valve (eTRV) Support

v0.3+ now supports the MiHome Thermostatic Radiator valve (eTRV).
> WARNING: Due to the way the eTRV works there may be a delay from when a command is sent to it being processed by the device. See **eTRV Command Caching** below

### eTRV Commands
The MiHome Thermostatic Radiator valve (eTRV) can accept commands to perform operations, provide diagnostics or perform self tests.  The documented commands are provided in the table below.

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

#### Sending eTRV Commands
Single commands should be sent as a numeric value within a JSON request, for example to Request Diagnostics you can use a template node (Output as Parsed JSON) to send the following ```msg.payload```:
```
{
    "command": 226,
    "data": 0
}
```
Example for setting temperature to 20C using command mode:
```
{
    "command": 244,
    "data": 20
}
```

#### eTRV Command Caching
The eTRV reports its temperature at the *SET_REPORTING_INTERVAL* (default 5 minutes). The receiver is activated after each *TEMPERATURE* report to listen for commands. The receiver only remains active for 200ms or until a message is received.

To cater for this hardware limitation the **'eTRV node'** uses command caching and dynamic polling. Any command sent using the eTRV node will be held until a TEMPERATURE report is received; at this point the most recent cached message (only 1 is supported) will be sent to the eTRV.  Messages will continue to be resent until they have been succesfully received or until the number of Retries has reached 0.

The reason that a command may be resent multiple times is due to reporting issues. The eTRV devices, unfortunately, do not send acknowledgement for every command type (indicated by a 'No' in the *Response* column in the above table).  This includes the *TEMP_SET* command!  So these commands are always resent for the full number of retries.

> **NOTE:** The performance of node-red may decrease when an eTRV command is cached due to dynamic polling. The frequency that the radio device is polled by the monitor thread automatically increases by a factor of 200 when a command is cached (it goes from checking every 5 seconds to every 25 milliseconds) this dramatically increases the chance of a message being correctly received sooner.

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

## Troubleshooting
If you have any issues with the code, particularly if your board is not initialising, please try [ener314rt-debug](https://github.com/Achronite/ener314rt-debug), which has been created as a standalone node.js application with full debug enabled.  Node-red is not required to execute this test.

* *Unable to initialise Energenie ENER314-RT board error: -n*: Check that your card is installed correctly, and that you **do not** have hardware SPI enabled.  On raspbian if the hardware SPI driver was loaded, you will see the device `/dev/spidev0.0`.  If you see this, you will need to switch hardware SPI OFF.

* *Compile errors during install: 'unknown type/function napi_...'*:  This node module requires node.js v10 or above to work, upgrade your node.js version and retry.

# Package Details

## Change History
| Version | Date | Change details
|---|---|---|
0.1.0|27 Apr 19|Initial Release
0.2.0|08 May 19|Full NPM & node-red catalogue release
0.3.0|10 Jan 20|Major change - Switched to use node.js Native API (N-API) for calling C functions, and split off new node module.  Added a new node to support MiHome Radiator Valve, along with a separate thread for monitoring that implements caching and dynamic polling.  This version requires node.js v10+.
0.3.2|17 Jan 20|Added node v10+ dependency (via 'engines').  Fixed issue with teaching OOK devices, and added 'off' button. Added troubleshooting section to docs.
0.3.4|22 Jan 20|Fixed zone 0 switch all devices. Tested Energenie 4-way gang. Updates to GUI tip shown for eTRV. Made emit monitor device specific to improve performance.
0.3.5|02 Feb 20|Improve error handling for board failure.
0.3.6|02 Feb 20|Added compile error to README. Removed console.log for eTRV Rx (left in by mistake).
0.3.7|09 Feb 20|Fixed raw tx node for v0.3.x
0.3.8|01 Mar 20|Fixed passing of switchNum into OOK node. Fixed node.status showing ERROR for OOK node when there is a message in Rx buffer. Added support for payload.state and payload.unit as alternative parameters in OOK node. README updates
0.3.9|11 Nov 20|Fix the dependent version of energenie-ener314rt to 0.3.4 to allow version 0.4.0 (alpha) testing without impacting node-red code. README updates, including example monitor messages and success tests for 3 more devices from AdamCMC.
0.4.0|TBD|Added new C&M node that immediately sends commands (designed for MIHO069 Thermostat). Added MIHO069 thermostat params & icon. Added support for UNKNOWN commands (this assumes a uint as datatype for .data). Increased support for MIHO032 Motion Sensor (icon & node status). Updated Energenie device names. Renamed old C&M node to be 'Smart Plug+'. Readme updates.

## Dependencies

* [energenie-ener314rt](https://github.com/Achronite/energenie-ener314rt) -  Node module (by same author) used to perform all radio interaction, split from original code base in version 0.3.0

## Built With

* [Node-RED](http://nodered.org/docs/creating-nodes/) - for wiring together hardware devices, APIs and online services.

## Authors

* **Achronite** - *Node-Red wrappers, and dependent node module* - [Achronite](https://github.com/Achronite/node-red-contrib-energenie-ener314)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Bugs and Future Work

Future work is detailed on the [github issues page](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues). Please raise any bugs, questions, queries or enhancements you have using this page.

https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues


@Achronite - November 2020 - v0.4.0 Beta