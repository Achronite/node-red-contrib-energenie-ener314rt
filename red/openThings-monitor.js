/*
** Node-red node for monitor functions of Energenie ENER314-RT board
** Author: Achronite, March 2019
**
** v0.1 Alpha
**
** File: OpenThings-monitor.js
** Purpose: Node-Red wrapper for call to monitor node for ENER314-RT FSK/OpenThings (aka Monitor) devices
**
*/
var libradio = require( './libradio');

module.exports = function(RED) {

    function OpenThingsMonitorNode(config) {
        RED.nodes.createNode(this,config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var deviceId = Number(device.deviceId);

		// CHECK CONFIG
		if(!deviceId || device == null)
		{
			this.status({fill: "red", shape: "ring", text: "not configured"});
			return false;
		}


		//
		// UPDATE STATE
        this.status({fill: "grey", shape: "dot", text: "waiting…"});
/*
        device.events.on(deviceId, function(OTmsg)
        {
            // received event for me
            this.status({fill: "green", shape: "dot", text: "event"});

            msg.payload = "TO DO";

            // return payload
            node.send(msg);
        });
*/
    }
    RED.nodes.registerType("openThings-monitor",OpenThingsMonitorNode);
}
