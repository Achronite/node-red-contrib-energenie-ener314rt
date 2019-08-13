/*
** Node-red control of Energenie ENER314-RT board for remote control of radio sockets
** Author: Achronite, December 2018 - January 2019
**
** v0.1 Alpha
**
** File: OpenThings-switch.js
** Purpose: Node-Red wrapper for call to switch node for ENER314-RT FSK/OpenThings (aka Monitor) devices
**          Primarily intended for MiHome Adapter Plus - Control & Monitor
**
*/
"use strict";

var libradio = require('./libradio');

module.exports = function (RED) {

    function OpenThingsSwitchNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var board = RED.nodes.getNode(config.board);
        var deviceId = Number(device.deviceId) || 0;
        var productId = Number(device.productId) || 2;
        var retry = device.retry || true;
        var sentState = -1;

        // CHECK CONFIG
        if (!deviceId || device == null || !board || board == null) {
            this.status({ fill: "red", shape: "ring", text: "Not configured" });
            return false;
        } else {
            node.on('input', function (msg) {
                // Check all variables before we call the C routine to transmit, msg.payload overrides any defaults set in node

                // Check OpenThings deviceId
                if (deviceId == 0 || isNaN(deviceId)) {
                    this.error("DeviceId err: " + deviceId + " (" + typeof (deviceId) + ")");
                }

                // Check Switch State (default to off=0)
                var switchState = 0;
                if (typeof msg.payload == typeof true)
                    switchState = msg.payload ? 1 : 0;
                else if (typeof msg.payload.powerOn == typeof true)
                    switchState = msg.payload.powerOn ? 1 : 0;
                else if (msg.payload === "on" || msg.payload.powerOn === "on")
                    switchState = 1;

                // Set the node status in the GUI
                switch (switchState) {
                    case 1:
                        node.status({ fill: "green", shape: "ring", text: "ON sent" });
                        break;
                    case 0:
                        node.status({ fill: "red", shape: "ring", text: "OFF sent" });
                        break;
                }

                // Check xmit times (advanced), 26ms per payload transmission
                var xmits = Number(msg.payload.repeat) || 20;

                // Send payload (async)
                libradio.openThings_switch.async(productId, deviceId, switchState, xmits, function (err, res) {
                    // callback
                    if (err) {
                        node.error("openThings_switch err: " + err);
                        node.status({ fill: "red", shape: "dot", text: "ERROR" });
                    }
                    if (res == 0 && retry) {
                        // radio send successful, check guaranteed delivery through monitoring
                        //console.log(`Checking switch to ${switchState}`);
                        sentState = switchState;
                    }
                });

                // dont send any payload for the input messages, as we are also a monitor node
            });

            board.events.on('monitor', function (OTmsg) {
                if (OTmsg.deviceId == deviceId) {
                    // received event for me

                    // check if we have any confirmations outstanding in retry mode
                    if (retry) {
                        if (sentState == OTmsg.SWITCH_STATE) {
                            // switch confirmed
                            //console.log(`confirmed switch to ${sentState}`);
                            sentState = -1;

                        } else if (sentState >= 0) {
                            // oh dear, switch didn't actually switch!
                            node.warn(`openThings_switch: device ${deviceId} did not switch to ${sentState}, retrying`);

                            //Resend switch message (non-async) overriding xmits
                            let res = libradio.openThings_switch(productId, deviceId, sentState, 20);
                        }
                    }

                    // set node status for Rx switch status or temperature
                    if (OTmsg.SWITCH_STATE) {
                        node.status({ fill: "green", shape: "dot", text: "on" });
                    } else if (OTmsg.SWITCH_STATE != null) {   // also checks for undefined, assume 0=off
                        node.status({ fill: "red", shape: "ring", text: "off" });
                    } else if (OTmsg.TEMPERATURE){
                        node.status({ fill: "grey", shape: "ring", text: "Temp " + OTmsg.TEMPERATURE });
                    }

                    // send on decoded OpenThings message as is
                    node.send({ 'payload': OTmsg });
                }
            });

            board.events.on('error', function () { node.error("Board event error") });

            this.on('close', function () {
                // tidy up state
            });
        }

    }
    RED.nodes.registerType("openThings-switch", OpenThingsSwitchNode);
}
