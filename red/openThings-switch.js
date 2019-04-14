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
var libradio = require('./libradio');

module.exports = function (RED) {

    function OpenThingsSwitchNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var board = RED.nodes.getNode(config.board);
        var deviceId = Number(device.deviceId) || 0;
        var productId = Number(device.productId) || 2;

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
                });

                // dont send any payload for the input messages, as we are also a monitor node
            });

            board.events.on('monitor', function (OTmsg) {
                if (OTmsg.deviceId == deviceId) {
                    // received event for me

                    // set node status for confirmed switch status
                    if (OTmsg.SWITCH_STATE) {
                        node.status({ fill: "green", shape: "dot", text: "ON" });
                    } else {
                        node.status({ fill: "red", shape: "dot", text: "OFF" });
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
