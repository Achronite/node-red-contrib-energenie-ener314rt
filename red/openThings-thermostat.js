/*
** Node-red control of Energenie ENER314-RT board for battery powered control & monitor devices
** Author: Achronite, October 2020 - October 2021
**
** v0.5 Beta
**
** File: OpenThings-thermostat.js
** Purpose: Node-Red wrapper for MIHO069 Thermostat devices
**
*/
"use strict";

var ener314rt = require('energenie-ener314rt');

module.exports = function (RED) {

    function OpenThingsThermostatNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var board = RED.nodes.getNode(config.board);
        var deviceId = Number(device.deviceId) || 0;
        var productId = Number(device.productId) || 3;
        var retries = Number(device.retries) || 10;
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
                //  0 = off
                //  1 = thermostatically controlled
                //  2 = on
                //  other number = set target temperature
                switch (typeof msg.payload) {
                    case 'number':
                        var data = msg.payload;
                        if (msg.payload >= 0 && msg.payload <= 2)
                            var cmd = 0xAA; // set thermostat mode                            
                        else
                            var cmd = 0xF4; // set temperature
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
                                node.status({ fill: "grey", shape: "ring", text: "Set Heating ON" });
                        }
                        break;

                    default:  // All other commands
                        node.status({ fill: "grey", shape: "ring", text: `Sent command ${cmd}:${data}` });
                }

                // Set command to be sent, next time the device wakes up
                var res = ener314rt.openThingsCacheCmd(productId, deviceId, cmd, data, retries);
                // ener314rt.openThingsCmd(productId, deviceId, cmd, data, xmits);

                if (res == 0) {
                    // command successfuly cached
                    // Set the node status in the GUI
                    switch (cmd) {
                        case 0xF4:  //TEMP_SET                  Temperature in C
                            node.status({ fill: "grey", shape: "ring", text: `Set Temp to ${data}C` });
                            break;
                        case 0xAA:  //SET_THERMOSTAT_MODE           0,1,2
                            switch (data) {
                                case 0:
                                    node.status({ fill: "grey", shape: "ring", text: "Requesting Always Off Mode" });
                                    break;
                                case 1:
                                    node.status({ fill: "grey", shape: "ring", text: "Requesting Auto Mode" });
                                    break;
                                case 2:
                                    node.status({ fill: "grey", shape: "ring", text: "Requesting Always ON Mode" });
                            }
                            break;
                        case 0xF3:  //SWITCH_STATE
                            node.status({ fill: "grey", shape: "ring", text: "Set switch state ${data}" });
                            break;
                        case 0x00:  //CANCEL
                            node.status({ fill: "grey", shape: "ring", text: "Cancelling command" });
                            break;
                        default:
                            node.error(`Unknown command ${cmd}`);
                    }
                    // TODO: clear message
                } else {
                    node.status({ fill: "red", shape: "dot", text: `Error $res` });
                    node.error(`Error ${res} sending command ${cmd}`);
                }

            });

            board.events.on(deviceId, function (OTmsg) {
                // set node status for Control & Monitor temperature (using the information returned)
                var nodeStatus = { fill: "grey", shape: "ring", text: ""};

                if (typeof (OTmsg.THERMOSTAT_MODE) == 'number') {
                    switch (OTmsg.THERMOSTAT_MODE) {
                        case 0:
                            nodeStatus.text = "Off ";
                            break;
                        case 1:
                            nodeStatus.text = "Auto ";
                            nodeStatus.fill = "green";
                            break;
                        case 2:
                            nodeStatus.text = "On ";
                            nodeStatus.fill = "red";
                    }
                }

                if (typeof (OTmsg.TEMPERATURE) == 'number') {      
                    nodeStatus.text += OTmsg.TEMPERATURE.toFixed(1);
                }              
                if (typeof OTmsg.TARGET_TEMP == 'number') {
                    nodeStatus.text += "(" + OTmsg.TARGET_TEMP + ")";
                }

                if (typeof (OTmsg.SWITCH_STATE) == 'number') {    
                    nodeStatus.shape = "dot";
                    switch (OTmsg.SWITCH_STATE) {
                        case 0:
                            nodeStatus.text+= " [Heating]"
                            break;
                        case 1:
                            nodeStatus.text+= " [OFF]"
                            break;
                    }
                } 

                if (nodeStatus.text.length > 0){
                    node.status(nodeStatus);
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
    RED.nodes.registerType("openThings-thermostat", OpenThingsThermostatNode);
}
