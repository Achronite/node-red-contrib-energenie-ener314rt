/*
** Node-red node for monitor functions of Energenie ENER314-RT board, adapted specifically for MiHome Click
** This node is almost identical to the 'monitor' node
** Author: Achronite, February 2024
**
** v0.1 Alpha
**
** File: OpenThings-click.js
** Purpose: Node-Red wrapper for call to monitor MiHome click devices
**
*/

"use strict";

module.exports = function (RED) {

    function OpenThingsClickNode(config) {
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
                var d = new Date(0);
                d.setUTCSeconds(OTmsg.timestamp);
                let timeStr = d.toTimeString();
                // received event for me, update status and send monitor message to consuming downstream nodes
                switch (OTmsg.BUTTON) {
                    case 1:
                        node.status({ fill: "red", shape: "dot", text: `Click at ${timeStr}` });
                        break;
                    case 2:
                        node.status({ fill: "red", shape: "dot", text: `Double Click at ${timeStr}` });
                        break;
                    case 255:
                        node.status({ fill: "red", shape: "dot", text: `Long press at ${timeStr}` });
                        break;
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
    RED.nodes.registerType("openThings-click", OpenThingsClickNode);
}
