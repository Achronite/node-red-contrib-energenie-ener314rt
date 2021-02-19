/*
** Node-red node for monitor functions of Energenie ENER314-RT board, adapted specifically for Door Sensor
** This node is almost identical to the 'monitor' node
** Author: Achronite, February 2021
**
** v0.1 Alpha
**
** File: OpenThings-door.js
** Purpose: Node-Red wrapper for call to monitor only node for ENER314-RT FSK/OpenThings (aka Monitor) devices
**
*/

"use strict";

module.exports = function (RED) {

    function OpenThingsDoorNode(config) {
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
                if (OTmsg.DOOR_SENSOR == 1) {
                    var d = new Date(0);
                    d.setUTCSeconds(OTmsg.timestamp);
                    let timeStr = d.toTimeString();
                    node.status({ fill: "grey", shape: "ring", text: `Closed at ${timeStr}` });
                } else if (OTmsg.DOOR_SENSOR == 0) {
                    var d = new Date(0);
                    d.setUTCSeconds(OTmsg.timestamp);
                    let timeStr = d.toTimeString();
                    node.status({ fill: "red", shape: "dot", text: `Open at ${timeStr}` });
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
    RED.nodes.registerType("openThings-door", OpenThingsDoorNode);
}
