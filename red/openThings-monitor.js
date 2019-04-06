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
var ref = require('ref');
var libradio = require('./libradio');

module.exports = function (RED) {

    function OpenThingsMonitorNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var board = RED.nodes.getNode(config.board);
        var deviceId = Number(device.deviceId);

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        //var buf = Buffer.alloc(500);

        // CHECK CONFIG
        if (!deviceId || device == null || !board || board == null) {
            this.status({ fill: "red", shape: "ring", text: "not configured" });
            return false;
        } else {
            this.status({ fill: "grey", shape: "dot", text: "Waiting…" });

            board.events.on('monitor', function (OTmsg) {
                if (OTmsg.deviceId == deviceId) {
                    // received event for me
                    
                    //console.log("@@@OT-monitor: received monitor event for me! recs=" + OTmsg.recCount);

                    if (OTmsg.SWITCH_STATE) {
                        node.status({ fill: "green", shape: "dot", text: "ON" });
                    } else {
                        node.status({ fill: "red", shape: "ring", text: "OFF" });
                    }

                    // send on decoded OpenThings message as is
                    node.send({'payload':OTmsg});
                }
            });

            board.events.on('error', function () { node.error("Board event error")});

            this.on('close', function () {
                // tidy up state
            });
        }

    }
    RED.nodes.registerType("openThings-monitor", OpenThingsMonitorNode);
}
