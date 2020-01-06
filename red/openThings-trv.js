/*
** Node-red control of Energenie ENER314-RT board for remote control of radiator valves
** Author: Achronite, August - September 2019
**
** v0.3 Alpha
**
** File: OpenThings-trv.js
** Purpose: Node-Red wrapper for call to eTRV node for ENER314-RT eTRV device (Control & Monitor)
**
*/
"use strict";

var path = require('path');

// TODO: Separate out as npm module
var ener314rt = require(path.join(__dirname, '../build/Release/ener314rt'));

module.exports = function (RED) {

    function OpenThingsTrvNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var board = RED.nodes.getNode(config.board);
        var deviceId = Number(device.deviceId) || 0;
        var productId = Number(device.productId) || 3;
        var retry = device.retry || true;
        var sentState = -1;

        // CHECK CONFIG
        if (!deviceId || device == null || !board || board == null) {
            this.status({ fill: "red", shape: "ring", text: "Not configured" });
            return false;
        } else {
            node.on('input', function (msg) {
                // Check all variables before we cache the message to transmit, msg.payload overrides any defaults set in node

                // Check OpenThings deviceId
                if (deviceId == 0 || isNaN(deviceId)) {
                    this.error("DeviceId err: " + deviceId + " (" + typeof (deviceId) + ")");
                }

                // Check the command type by numeric passed in for now
                //  0 = valve open
                //  1 = valve closed
                //  2 = valve controlled by temperature
                //  other number = set target temperature of radiator
                switch (typeof msg.payload) {
                    case 'number':
                        var data = msg.payload;
                        if (msg.payload >= 0 && msg.payload <= 2)
                            var cmd = 0xA5; // set valve state                            
                        else
                            var cmd = 0xF4; // set temperature
                        break;
                    case 'boolean':
                        // assume set valve state
                        var cmd = 0xA5; // set valve state
                        var data = msg.payload ? 0 : 1;
                        break;
                    case 'object':
                        // Assume Command mode
                        if (typeof msg.payload.command == 'number' && typeof msg.payload.data == 'number') {
                            var cmd = msg.payload.command;
                            var data = msg.payload.data;
                        } else {
                            this.error(`Invalid payload object: ${msg.payload}`);
                            return false;
                        }
                        break;
                    default:
                        this.error(`Invalid payload: ${msg.payload}`);
                        return false;
                }

                // Set command to be sent, next time radiator wakes up
                var res = ener314rt.openThingsCacheCmd(deviceId, cmd, data);

                if (res == 0) {
                    // command successfuly cached
                    // Set the node status in the GUI
                    switch (cmd) {
                        case 0xF4:  //TEMP_SET                  Temperature in C
                            node.status({ fill: "grey", shape: "ring", text: `Set Temp to ${data}C` });
                            break;
                        case 0xA5:  //SET_VALVE_STATE           0,1,2
                            switch (data) {
                                case 0:
                                    node.status({ fill: "grey", shape: "ring", text: "Opening Valve" });
                                    break;
                                case 1:
                                    node.status({ fill: "grey", shape: "ring", text: "Closing Valve" });
                                    break;
                                case 2:
                                    node.status({ fill: "grey", shape: "ring", text: "Temp Controlled" });
                            }
                            break;
                        case 0xA3:  //EXERCISE_VALVE
                            node.status({ fill: "grey", shape: "ring", text: "Exercising Valve" });
                            break;
                        case 0xA4:  //SET_LOW_POWER_MODE        0,1
                            if (data) {
                                node.status({ fill: "grey", shape: "ring", text: "Set Low power mode on" });
                            } else {
                                node.status({ fill: "grey", shape: "ring", text: "Set Low power mode off" });
                            }
                            break;
                        case 0xA6:  //REQUEST_DIAGNOTICS
                            node.status({ fill: "grey", shape: "ring", text: "Requesting Diagnostics" });
                            break;
                        case 0xBF:  //IDENTIFY
                            node.status({ fill: "grey", shape: "ring", text: "Identifying Valve" });
                            break;
                        case 0xD2:  //SET_REPORTING_INTERVAL    Time in seconds (300-3600, default=300=5mins)
                            node.status({ fill: "grey", shape: "ring", text: `Set Reporting interval to ${data}secs` });
                            break;
                        case 0xE2:  //REQUEST_VOLTAGE
                            node.status({ fill: "grey", shape: "ring", text: "Requesting Voltage" });
                            break;
                        default:
                            node.error(`Unknown command ${cmd}`);
                    }

                } else {
                    node.status({ fill: "red", shape: "dot", text: "Device Unknown" });
                    node.error(`Device currently unknown, retry later`);
                }
                // dont send any payload for the input messages, as we are also a monitor node

            });

            board.events.on('monitor', function (OTmsg) {
                if (OTmsg.deviceId == deviceId) {
                    // received event for me
                    // cached commands now handled in C

                    // set node status for eTrv temperature
                    if (typeof(OTmsg.TEMPERATURE) == 'number') {
                        node.status({ fill: "grey", shape: "ring", text: "Temp " + OTmsg.TEMPERATURE });
                    }

                    // send on decoded OpenThings message as is
                    node.send({ 'payload': OTmsg });
                }
            });

            board.events.on('error', function (err) {
                node.error(`Board event error ${err}`);
            });
        }

    }
    RED.nodes.registerType("openThings-trv", OpenThingsTrvNode);
}
