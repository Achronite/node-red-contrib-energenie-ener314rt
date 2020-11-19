/*
** Node-red node for monitor functions of Energenie ENER314-RT board
** Author: Achronite, March 2019
**
** v0.1 Alpha
**
** File: OpenThings-monitor.js
** Purpose: Node-Red wrapper for call to monitor only node for ENER314-RT FSK/OpenThings (aka Monitor) devices
**
*/

"use strict";

module.exports = function (RED) {

    function OpenThingsMonitorNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var board = RED.nodes.getNode(config.board);
        var deviceId = Number(device.deviceId);

        // CHECK CONFIG
        if (!deviceId || device == null || !board || board == null) {
            this.status({ fill: "red", shape: "ring", text: "Not configured" });
            return false;
        } else {
            this.status({ fill: "grey", shape: "dot", text: "Waiting…" });

            board.events.on(deviceId, function (OTmsg) {
                // received event for me, update status and send monitor message to consuming downstream nodes
                if (OTmsg.SWITCH_STATE) {
                    node.status({ fill: "green", shape: "dot", text: "on" });
                } else if (OTmsg.SWITCH_STATE != null) {   // also checks for undefined, assume 0=off
                    node.status({ fill: "red", shape: "ring", text: "off" });
                } else if (OTmsg.TEMPERATURE) {
                    node.status({ fill: "grey", shape: "ring", text: "Temp " + OTmsg.TEMPERATURE });
                } else if (OTmsg.MOTION_DETECTOR == 1) {
                    var d = new Date(0);
                    d.setUTCSeconds(OTmsg.timestamp);
                    let timeStr = d.toTimeString();
                    node.status({ fill: "red", shape: "dot", text: `Motion at ${timeStr}` });
                } else if (OTmsg.MOTION_DETECTOR == 0) {
                    node.status({ fill: "grey", shape: "ring" });
                }
                // send on decoded OpenThings message as is
                node.send({ 'payload': OTmsg });
            });

            board.events.on('error', function () { node.error("Board event error") });

            this.on('close', function () {
                // tidy up state
            });
        }

    }
    RED.nodes.registerType("openThings-monitor", OpenThingsMonitorNode);
}
