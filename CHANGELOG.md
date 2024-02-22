# node-red-contrib-eneregenie-ener314rt Change Log

## [Unreleased]

* [#75](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/75) Support for MiHome Click
* [#74](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/74) Thermostat icon disappears when dragged to flow

Also see [Issues](https://github.com/Achronite/energenie-ener314rt/issues) for issues relating to dependent module.

## [0.7.0] 2024-02-22

### IMPORTANT: This release requires the following updates that will need to be manually installed

* `gpiod` & `libgpiod`: New dependencies that need to be installed  (e.g raspbian: `sudo apt-get gpiod libgpiod`)

### Added

* Support added for MiHome Thermostat (MIHO069), including auto-messaging to obtain telemetry and additional commands [#36](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/36)
* Support added for MiHome Click (MIHO089) [#75](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/75)
* Setting target temperature now caters for 0.5 increments (previously integer)
* Support raspberry pi 5 (alpha)

### Fixed

* Fixed workspace default name for 'PIR sensor' 
* Fixed icons missing from when dragged to canvas [#74](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/74)

### Changed

* **BREAKING DEPENDENCIES**: The version of `energenie-ener314rt` needed is v0.7.x [#72](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/72).  This uses a newer GPIO library that is compatible with the pi5. `gpiod` and `libgpiod` will need to be installed first
* Pretty printed all device JSON files
* Submitting a cached command will now replace the existing cached command for the device
* Remove deprecated 'Control & Monitor' node [#76](https://github.com/Achronite/node-red-contrib-energenie-ener314rt/issues/76).  Please use the specific purple node created for your device instead.
* Migrated change history from README to here


See also: https://github.com/Achronite/energenie-ener314rt/releases/tag/v0.7.1  - Notably pi5 support and GPIO driver changes


## Migrated Change Log for older versions

| Version | Date | Change details |
|---|---|---|
0.1.0|27 Apr 19|Initial Release|
0.2.0|08 May 19|Full NPM & node-red catalogue release|
0.3.0|10 Jan 20|Major change - Switched to use node.js Native API (N-API) for calling C functions, and split off new node module.  Added a new node to support MiHome Radiator Valve, along with a separate thread for monitoring that implements caching and dynamic polling.  This version requires node.js v10+.|
0.3.2|17 Jan 20|Added node v10+ dependency (via 'engines').  Fixed issue with teaching OOK devices, and added 'off' button. Added troubleshooting section to docs.|
0.3.4|22 Jan 20|Fixed zone 0 switch all devices. Tested Energenie 4-way gang. Updates to GUI tip shown for eTRV. Made emit monitor device specific to improve performance.|
0.3.5|02 Feb 20|Improve error handling for board failure.|
0.3.6|02 Feb 20|Added compile error to README. Removed console.log for eTRV Rx (left in by mistake).|
0.3.7|09 Feb 20|Fixed raw tx node for v0.3.x|
0.3.8|01 Mar 20|Fixed passing of switchNum into OOK node. Fixed node.status showing ERROR for OOK node when there is a message in Rx buffer. Added support for payload.state and payload.unit as alternative parameters in OOK node. README updates|
0.3.9|11 Nov 20|Fix the dependent version of energenie-ener314rt to 0.3.4 to allow version 0.4.0 (alpha) testing without impacting node-red code. README updates, including example monitor messages and success tests for 3 more devices from AdamCMC.|
0.4.0|19 Feb 21|Added new C&M node that immediately sends commands (designed for MIHO069 Thermostat). Added MIHO069 thermostat params & icon. Added support for UNKNOWN commands (this assumes a uint as data-type for .data). Added specific nodes for MIHO032 Motion Sensor and MIHO033 Open Sensor. Updated Energenie device names. Renamed old C&M node to be 'Smart Plug+'. Readme updates.|
0.4.1|bugfix|Reduced internal efficiency 'sleep' from 5s to 0.5s (for non-eTRV send mode) to reduce risk of losing a message (Issue #14). Fix crash when using over 6 devices (Issue #15).
0.4.2|05 May 21|Added MiHome Dimmer node. Made ON/OFF status messages consistent across node types. Bug fix for issue #49. Only stop monitoring during close if has been started. README updates.|
0.5.0|19 Apr 22|Added specific node for MIHO069 MiHome Thermostat, deprecating the generic Control & Monitor node (as no other C&M devices exist at present).
0.5.1|Sep 22|Increased support for MiHome House Monitor issue #57 (added apparent_power to node status & new node icon), Fixed Zone 0 (all) for Control Node (Issue #61)
0.5.2|Sep 22|Added node-red version to package.json
0.6.0|23 Jan 23|Updated for v0.6.0 of dependency [energenie-ener314rt](https://github.com/Achronite/energenie-ener314rt), which contains multiple fixes and improvements. Highlights: <br>Hardware driver support added using spidev, which falls back to software driver if unavailable.<br>Renamed TARGET_C to TARGET_TEMP for eTRV.<br>Add capability for cached/pre-cached commands to be cleared with command=0.