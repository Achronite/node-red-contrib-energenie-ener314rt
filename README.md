# node-red-contrib-energenie-ener314rt
A node-red module to control the Energenie MiHome line of products via the ENER314-RT add-on board for the Raspberry Pi.

[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-brightgreen.svg)](https://github.com/Achronite/node-red-contrib-energenie-ener314/graphs/commit-activity)
[![Downloads](https://img.shields.io/npm/dm/node-red-contrib-energenie-ener314rt.svg)](https://www.npmjs.com/package/node-red-contrib-energenie-ener314rt)
![node](https://img.shields.io/node/v/node-red-contrib-energenie-ener314rt)
[![Release](https://img.shields.io/github/release-pre/achronite/node-red-contrib-energenie-ener314rt.svg)](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/releases)
[![NPM](https://nodei.co/npm/node-red-contrib-energenie-ener314rt.png)](https://nodei.co/npm/node-red-contrib-energenie-ener314rt/)


[node-red module](https://flows.nodered.org/node/node-red-contrib-energenie-ener314rt)

## IMPORTANT: UPGRADING FROM PREVIOUS RELEASE

**v0.7.x requires additional software dependencies that must be manually installed first.**

If you are upgrading from version 0.6.x or below, please ensure that you install node.js v18.2+, `gpiod` and `libgpiod` first (see Getting Started below).


## Purpose

You can use this node-red module to control and monitor the [Energenie](https://energenie4u.co.uk/) MiHome radio based smart devices such as adapters, sockets, lights, thermostats and relays 
on a Raspberry Pi with an [ENER314-RT](https://energenie4u.co.uk/catalogue/product/ENER314-RT) board installed using node-red (see below for full list).  This is *instead* of operating the devices using a MiHome Gateway, so this node does not require an internet connection.

**'Control'**, **'Monitor'** and **'Control & Monitor'** radio based devices are supported from the legacy and MiHome range.

There are 4 types of node to match the colour coding of the Energenie MiHome devices:
* **Blue** for switching '**Control**' OOK based devices
* **Pink** for monitoring MiHome **'Monitor'** FSK/OpenThings devices
* **Purple** for monitoring and controlling **'Control & Monitor'** FSK/OpenThings devices
* **Green** for sending any OOK or FSK raw byte array (Advanced node)

Within the 4 types there are these nodes available to use:

| Node | Created For |
|---|---|
|![Blue Control](doc-images/B-Control.png?raw=true)|All Blue, Green & Legacy (OOK) Energenie Devices|
|![Blue Dimmer](doc-images/B-Dimmer.png?raw=true)|MIHO010 - MiHome Dimmer|
|![Pink Monitor](doc-images/P-Monitor.png?raw=true)|All Pink Monitor Only Devices|
|![Pink PIR Sensor](doc-images/P-PIR.png?raw=true)|MIHO032 - MiHome Motion sensor|
|![Pink Open Sensor](doc-images/P-Sensor.png?raw=true)|MIHO033 - MiHome Open Door/Window sensor|
|![Pink MiHome Click](doc-images/P-Click.png?raw=true)|MIHO089 - MiHome Click|
|![Purple eTRV](doc-images/C-TRV.png?raw=true)|MIHO013 - MiHome Radiator Valve|
|![Purple Smart Plug+](doc-images/C-Adaptor.png?raw=true)|MIHO005 - MiHome Smart Plug+ / Adaptor+|
|![Purple Thermostat](doc-images/C-Thermostat.png?raw=true)|MIHO069 - MiHome Thermostat|
|![Green Raw Transmit](doc-images/G-Raw.png?raw=true)|Non-Energenie 433Mhz Radio Controlled Devices|

The number of individual devices this node can control is over 4 million, so it should be suitable for most installations!

>NOTE: This module does not currently support the older boards (ENER314/Pi-Mote), the Energenie WiFi sockets or the MiHome Gateway (see below for the full supported list).

## Supported Devices

These nodes are designed for energenie RF radio devices in the OOK & FSK (OpenThings) ranges.

Here is a table showing which node is recommended for each energenie device, and a tick showing if it has been tested (please let me know of any successful tests, and I'll update the table):


| Device | Description | Type | Recommend Node | Tested OK |
|---|---|:---:|---|:---:|
|ENER002|Green Button Adapter|OOK|Blue: Control| &#10003; |
|ENER010|MiHome 4 gang Multi-plug|OOK|Blue: Control| &#10003; |
|MIHO002|MiHome Smart Plug|OOK|Blue: Control|&#10003;|
|MIHO004|MiHome Smart Monitor Plug|FSK|Pink: Monitor| &#10003; |
|MIHO005|MiHome Smart Plug+|FSK|Purple: Smart Plug+| &#10003; |
|MIHO006|MiHome House Monitor|FSK|Pink: Monitor| &#10003; |
|MIHO007|MiHome Socket|OOK|Blue: Control| &#10003; |
|MIHO008|MiHome Single Light|OOK|Blue: Control||
|MIHO009|Double Gang MiHome Light|OOK|Blue: Control||
|MIHO010|MiHome Dimmer Switch|OOK|Blue: Dimmer||
|MIHO013|MiHome Radiator Valve|FSK Cached|Purple: eTRV| &#10003; |
|MIHO014|Single Pole Relay (inline)|OOK|Blue: Control||
|MIHO015|MiHome Relay|OOK|Blue: Control||
|MIHO021<br />MIHO022<br />MIHO023|MiHome Double Socket|OOK|Blue: Control| &#10003; |
|MIHO024<br />MIHO025<br />MIHO026|MiHome Single Light|OOK|Blue: Control||
|MIHO032|MiHome Motion sensor|FSK|Pink: PIR Sensor| &#10003; |
|MIHO033|MiHome Open Sensor|FSK|Pink: Open Sensor||
|MIHO069|MiHome Heating Thermostat|FSK|Purple: Thermostat|&#10003;|
|MIHO071<br />MIHO072<br />MIHO073|Double Gang MiHome Light|OOK|Blue: Control||
|MIHO076<br />MIHO077<br />MIHO087|MiHome Dimmer Switch|OOK|Blue: Dimmer||
|MIHO089|MiHome Click - Smart Button|FSK|Pink: MiHome Click|&#10003;|


### NOT SUPPORTED:
Specific nodes may be required for new energenie devices.  Please let me know, via [github](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues), if you identify any 'unknown' devices, commands or parameters.

The use of these nodes within the Node-RED add-on for [Home Assistant](https://www.home-assistant.io/) (aka hassio) is [not supported](https://community.home-assistant.io/t/accessing-gpio-spi-from-custom-node-red-node-node-red-contrib-energenie-ener314rt/170002).  A new MQTT / Home Assistant integration is now available at [mqtt-energenie-ener314rt](https://github.com/Achronite/mqtt-energenie-ener314rt), please use this instead.


## Getting Started

1) Plug in your ENER314-RT-VER01 board from Energenie onto the 26 pin or 40 pin connector of your Raspberry Pi.

2) Install `gpiod` and `libgpiod` dependencies (as of v0.7.x) :
For example (Debian):
```
sudo apt-get install gpiod libgpiod-dev
```

3) Install this module as you would any node-red module using the 'Manage palette' option in Node-Red GUI or by using npm.

4) If you have any **'Control'** only devices, perform a one-time only setup to **teach** your  devices to operate with your selected zone code(s) switch number(s) combinations: 

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

> TIP: Usually each energenie **'Control'** only devices can be assigned 2 separate zone codes, so you could operate them across this and another system (e.g RF hand controller). There is one downside though, in that the on/off state will not be reflected on the other system for these devices.

5)  If you have any **'Monitor'** or **'Control & Monitor'** devices perform one-time only setup to **discover** the devices

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

## Hardware based SPI driver - *NEW* In Version 0.6
To increase reliability a new hardware SPI driver has been added.  The hardware SPI driver version can be enabled using `sudo raspi-config` choosing `Interface Options` and `SPI` to enable the hardware SPI mode, do this whilst this software is not running.  The module tries to use the hardware driver on start-up, if it has not been enabled it falls back to using the software SPI driver.

## 'Control Only' OOK Zone Rules

* Each Energenie **'Control'** or OOK based device can be assigned to a specific zone (or house code) and a switch number.
* Each zone is encoded as a 20-bit address (1-1048575 decimal).
* Each zone can contain up to 6 switches (1-6) - NOTE: officially energenie state this is only 4 devices (1-4)
* All devices within the **same** zone can be switched **at the same time** using a switch number of '0'.
* A default zone '0' can be used to use Energenie's default zone (0x6C6C6).
* If you have a MiHome 4 gang Multi-plug, the same zone must be used for controlling all 4 switches, use switch #0 to control all, 1-4 for each socket
* If you have a MiHome 2 gang socket or light switch, the same zone must be used for controlling the 2 switches


## Light Dimmer Support

Each Energenie Light Dimmer requires a dedicated OOK zone allocating to it, as internally it uses the switch numbers to set the brightness level of the dimmer. This node works slightly differently to the standard **'Control'** node. The `payload` determines the light level required as follows:

|payload|Brightness Level|Action|
|:---:|:---:|---|
|0 OR false|OFF|Switch light off, remembering light level|
|1 OR true|last|Switch on at the previous light level set|
|2 or 20|20%|Switch light on at 20%|
|3 or 30|30%|Switch light on at 30%|
|4 or 40|40%|Switch light on at 40%|
|5 or 60|60%|Switch light on at 60%|
|6 or 80|80%|Switch light on at 80%|
|7 or 100|100%|Switch light on at 100%|


## Processing Monitor Messages

The Pink and Purple nodes receive monitoring information from the devices and emit the received parameter values on their output.  These messages conform to the OpenThings parameter standard.

All OpenThings parameters received from the device are decoded and returned in the ```msg.payload```.  Some of the values are used to set the *node.status* for the specific nodes, for example *SWITCH_STATE* is used to indicate if a device is 'ON' or 'OFF', and the *TEMPERATURE* value is used on the eTRV node to show the current temperature.

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
DOOR_SENSOR: <Sensor state, 0 = closed, 1 = open>
```
### Example msg.payload - Heating Thermostat (MIHO069)
```
deviceId: <device number>
mfrId: 4
productId: 18
timestamp: <numeric 'epoch based' timestamp, of when message was read>
TEMPERATURE: 23.06
REL_HUMIDITY: 72
BATTERY_LEVEL: 3.08
MOTION_DETECTOR: <Motion detector state, 0 = no motion, 1 = motion>
THERMOSTAT_MODE: <Thermostat mode, 0 = off, 1 = temp controlled, 2= always on>
TARGET_TEMP: <Target set temperature>
SWITCH_STATE: <Current state of the heating 0 = off, 1 = heating (when pressed)>
```
### Example msg.payload - MiHome Click (MIHO089)
```
deviceId: <device number>
mfrId: 4
productId: 19
timestamp: <numeric 'epoch based' timestamp, of when message was read>
VOLTAGE: <battery voltage>
BUTTON: <1=Single press, 2=Double press, 255=long press>
```

## MiHome Heating Device Support
The MiHome Thermostatic Radiator valve (eTRV) and MiHome Thermostat are battery powered devices that do not constantly listen for commands. This required specific code to be written to 'cache' commands for these devices.  As a result there may be a delay from when a command is sent to it being processed by the device. See **Command Caching** below.

### Sending Commands (purple nodes)
Single commands are sent as a numeric value within a JSON request as shown in the tables below.  For example to Request Diagnostics you can use a template node (Output as Parsed JSON) to send the following ```msg.payload```:
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
#### eTRV Commands
The MiHome Thermostatic Radiator valve (eTRV) accepts commands to perform operations, provide diagnostics or perform self tests.  The documented commands are provided in the table below, there may also be other undocumented commands.

| Command | # | Description | .data | Response Msg |
|---|:---:|---|---|:---:|
|CANCEL|0|Cancel existing cached command (set retries to 0)||Yes|
|EXERCISE_VALVE|163|Send exercise valve command, recommended once a week to calibrate eTRV||DIAGNOSTICS|
|SET_LOW_POWER_MODE|164|This is used to enhance battery life by limiting the hunting of the actuator, ie it limits small adjustments to degree of opening, when the room temperature is close to the *TEMP_SET* point. A consequence of the Low Power mode is that it may cause larger errors in controlling room temperature to the set temperature.|0=Off<br>1=On|No*|
|SET_VALVE_STATE|165|Set valve state|0=Open<br>1=Closed<br>2=Auto (default)|No|
|REQUEST_DIAGNOSTICS|166|Request diagnostic data from device, if all is OK it will return 0. Otherwise see additional monitored values for status messages||DIAGNOSTICS|
|IDENTIFY|191|Identify the device by making the green light flash on the selected eTRV for 60 seconds||No|
|SET_REPORTING_INTERVAL|210|Update reporting interval to requested value|300-3600 seconds|No|
|REQUEST_VOLTAGE|226|Report current voltage of the batteries||VOLTAGE|
|TARGET_TEMP|244|Send new target temperature for eTRV in 0.5 increments.<br>NOTE: The VALVE_STATE must be set to 'Auto' for this to work.|int|No|

> \* Although this will not auto-report, a subsequent call to *REQUEST_DIAGNOSTICS* will confirm the *LOW_POWER_MODE* setting

#### Thermostat Commands

The MiHome Thermostat accepts the following commands to perform operations.

> WARNING: If you are using a MiHome gateway to control your thermostat command clash may occur by issuing command within node-red.

| Command | # | Description | .data | Tested |
|---|:---:|---|---|:---:|
|CANCEL|0|Cancel existing cached command (set retries to 0)||Yes|
|THERMOSTAT_MODE|170|Change mode of thermostat where<br>0 = OFF<br>1 = Temp Controlled<br>2 = ON|0-2|Yes|
|RELAY_POLARITY|171|Polarity of the boiler relay|0=Normally Open,1=Normally Closed|Yes|
|HUMID_OFFSET|186|Humidity Calibration|-20 to 20|Yes|
|TEMP_OFFSET|189|Temperature Calibration|-20.0 to 20.0|Yes||TARGET_TEMP|244|Send new target temperature for thermostat (0-30) in 0.5 increments.<br>NOTE: The THERMOSTAT_MODE must be set to '1' for this to work.|5-30|Yes|
|HYSTERESIS|254|The difference between the current temperature and target temperature before the thermostat triggers|0.5-10|Yes|


In order for the Thermostat to provide updates for it's telemetry data when used **without a MiHome gateway**, auto messaging has been enabled within this module.  To start this auto-messaging you will need to send a  command that returns the `THERMOSTAT_MODE` to the application (a `THERMOSTAT_MODE` command will do).  The result of the most recent `THERMOSTAT_MODE` value will be stored and periodically replayed (until a restart) to prompt the thermostat into providing it's telemetry data.

### Command Caching
Battery powered energenie devices, such as the eTRV or Room Thermostat do not constantly listen for commands.  For example, the eTRV reports its temperature at the *SET_REPORTING_INTERVAL* (default 5 minutes) after which the receiver is then activated to listen for commands. The receiver only remains active for 200ms or until a message is received.

To cater for these hardware limitations the **'eTRV'** and **'Thermostat'** nodes use command caching and dynamic polling. Any command sent using these nodes will be held until a TEMPERATURE (for eTRV) or WAKEUP (for Thermostat) message is received; at this point the most recent cached message (only 1 is supported) will be sent to the device.  Messages will continue to be resent until they have been successful received or until the number of retries has reached 0.

Sometimes a specific command may be resent multiple times. This is particularly a problem for the eTRV devices, as they do not send an acknowledgement for every command type (indicated by a 'No' in the *Response* column in the above table).  This includes the *TEMP_SET* command!  So these commands are always resent for the full number of retries.  ** NEW v0.6.x ** - When a device *has* acknowledged a command the 'command' and 'retries' topics are reset to 0.

Be careful when sending multiple commands to the same device, as the most recent command will overwrite any existing cached command for that device (check the retries is 0 first).

> **NOTE:** The performance of node-red may decrease when a command is cached due to dynamic polling. The frequency that the radio device is polled by the monitor thread automatically increases by a factor of 200 when a command is cached (it goes from checking every 0.5 seconds to every 25 milliseconds) this dramatically increases the chance of a message being correctly received by the device sooner.

### eTRV Monitor Messages

To support the MiHome Radiator Valve (MIHO013) aka **'eTRV'** in v0.3 and above, additional code has been added to store the received and set information for these devices.  An example of the values are shown below, only 'known' values are returned when the eTRV regularly reports the TEMPERATURE.  See table for types and determining when field was last updated:
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
    "TARGET_TEMP": 10,
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
|TARGET_TEMP|Target temperature in celcius|int|TEMP_SET command|
|TEMPERATURE|The current temperature in celcius|float|timestamp|
|VALVE_STATE|Current valve mode/state| open, closed, auto, error|VALVE_STATE command *or* DIAGNOSTIC_TS on error|
|VALVE_TS|timestamp of when last *EXERCISE_VALVE* took place|epoch|DIAGNOSTIC_TS|
|VOLTAGE|Current battery voltage|float|VOLTAGE_TS|
|VOLTAGE_TS|Timestamp of when battery voltage was last received|epoch|VOLTAGE_TS|

>TIP: To get up-to-date information for a specific eTRV parameter you will need to request the device for an update by sending the appropriate command (see [eTRV Commands](#-eTRV-Commands) above),

## Changing the Icons
The icon as displayed on the node within the flows can be changed using the 'Icon' dropdown in the 'Appearance' tab in the node properties. I have created icons for a few of the energenie devices. Use the search button and enter 'ener' to find them.

## Troubleshooting
If you have any issues with the code, particularly if your board is not initialising, please try [ener314rt-debug](https://github.com/Achronite/ener314rt-debug), which has been created as a standalone node.js application with full debug enabled.  Node-red is not required to execute these tests.

* *Unable to initialise Energenie ENER314-RT board error: -n*: Check that your card is installed correctly.

* *Compile errors during install: 'unknown type/function napi_...'*:  This node module requires node.js v10 or above to work, upgrade your node.js version and retry.

# Package Details

## Change History
See [CHANGELOG.md](./CHANGELOG.md)


## Dependencies

* [energenie-ener314rt](https://github.com/Achronite/energenie-ener314rt) -  Node module (by same author) used to perform all radio interaction

## Built With

* [Node-RED](http://nodered.org/docs/creating-nodes/) - for wiring together hardware devices, APIs and online services.

## Authors

* **Achronite** - *Node-Red wrappers, and dependent node module* - [Achronite](https://github.com/Achronite/node-red-contrib-energenie-ener314)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Bugs and Future Work

Future work is detailed on the [github issues page](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues). Please raise any bugs, questions, queries or enhancements you have using this page.

I am currently working on a new node.js implementation of ENER314-RT that uses MQTT for Home Assistant - [mqtt-energenie-ener314rt](https://github.com/Achronite/mqtt-energenie-ener314rt).

https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues


@Achronite - February 2024