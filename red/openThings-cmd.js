/*
** Node-red control of Energenie ENER314-RT board for control & monitor devices
** Author: Achronite, October 2020
**
** v0.4 Beta
**
** File: OpenThings-cmd.js
** Purpose: Node-Red wrapper for call to Control & Monitor node for ENER314-RT Control & Monitor device
**
** NOTE: This node has been deprecated as all purple devices now have specific nodes createc for them
**
*/
"use strict";

var ener314rt = require('energenie-ener314rt');

module.exports = function (RED) {

    function OpenThingsCmdNode(config) {
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
                //  0 = switch off
                //  1 = switch on
                //  other number = set target temperature
                switch (typeof msg.payload) {
                    case 'number':
                        var data = msg.payload;
                        if (msg.payload >= 0 && msg.payload <= 1)
                            var cmd = 0xF3; // set switch state                            
                        else
                            var cmd = 0xF4; // set temperature
                        break;
                    case 'boolean':
                        // assume set switch state
                        var cmd = 0xF3; // set valve state
                        var data = msg.payload ? 0 : 1;
                        break;
                    case 'object':
                        // Assume Command mode
                        if (typeof msg.payload.command == 'number') {
                            var cmd = msg.payload.command;
                            if (typeof msg.payload.data == 'number') {
                                var data = msg.payload.data;
                            } else {
                                var data = 0;
                            }
                        } else {
                            this.error(`Invalid payload object: ${msg.payload}`);
                            return false;
                        }
                        break;
                    default:
                        this.error(`Invalid payload: ${msg.payload}`);
                        return false;
                }

                // Check xmit times (advanced), 26ms per payload transmission
                var xmits = Number(msg.payload.repeat) || 20;

                // Set the node status in the GUI (before sending)
                switch (cmd) {
                    case 0xF4:  //TEMP_SET                  Temperature in C
                        node.status({ fill: "grey", shape: "ring", text: `Set Temp to ${data}C` });
                        break;
                    case 0xAA:  //SET_THERMOSTAT_MODE for MIHO069 0,1,2  (guessing at 0xAA as report is 0x2a)
                        switch (data) {
                            case 0:
                                node.status({ fill: "grey", shape: "ring", text: "Set Heating Off" });
                                break;
                            case 1:
                                node.status({ fill: "grey", shape: "ring", text: "Set Temp Controlled" });
                                break;
                            case 2:
                                node.status({ fill: "grey", shape: "ring", text: "Set Always ON" });
                        }
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
                    case 0xF3:   // SWITCH_STATE
                        switch (data) {
                            case 0:
                                node.status({ fill: "red", shape: "ring", text: "OFF sent" });
                                break;
                            case 1:
                                node.status({ fill: "green", shape: "ring", text: "ON sent" });
                                break;
                            default:
                                node.status({ fill: "grey", shape: "ring", text: `Sent command ${cmd}:${data}` });
                        }
                        break;

                    default:  // All other commands
                        node.status({ fill: "grey", shape: "ring", text: `Sent command ${cmd}:${data}` });
                }

                // Send command to device immediately
                var res = ener314rt.openThingsCmd(productId, deviceId, cmd, data, xmits);

                if (res == 0) {
                    // TODO: clear message
                } else {
                    node.status({ fill: "red", shape: "dot", text: `Error $res` });
                    node.error(`Error ${res} sending command ${cmd}`);
                }

            });

            board.events.on(deviceId, function (OTmsg) {
                // set node status for Control & Monitor temperature
                if (typeof (OTmsg.TEMPERATURE) == 'number') {
                    node.status({ fill: "grey", shape: "ring", text: "Temp " + OTmsg.TEMPERATURE });
                }

                // send on decoded OpenThings message as is
                //console.log(`sending payload for ${deviceId} ts=${OTmsg.timestamp}`);
                node.send({ 'payload': OTmsg });
            });

            board.events.on('error', function (err) {
                node.error(`Board event error ${err}`);
            });
        }

    }
    RED.nodes.registerType("openThings-cmd", OpenThingsCmdNode);
}
