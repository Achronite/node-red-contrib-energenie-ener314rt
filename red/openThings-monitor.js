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
var ref = require('ref');
var libradio = require( './libradio');

module.exports = function(RED) {

    function OpenThingsMonitorNode(config) {
        RED.nodes.createNode(this,config);
        var node = this;

        var device = RED.nodes.getNode(config.device);
        var deviceId = Number(device.deviceId);

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        var buf = Buffer.alloc(500);

		// CHECK CONFIG
		if(!deviceId || device == null)
		{
			this.status({fill: "red", shape: "ring", text: "not configured"});
			return false;
		}


		//
		// UPDATE STATE
        this.status({fill: "grey", shape: "dot", text: "Waiting…"});

        // do a receive - MAKE ASYNC
        
        //var ret = libradio.openThings_receive(20, buf );
        //var OTmsg = JSON.parse(buf);

        libradio.openThings_receive.async(20, buf, function(err,res) {
            var msg = {};
            // callback
            if (err) {
                node.error("openThings_receive err: " + err);
                node.status({fill:"red",shape:"dot",text:"ERROR" });
            } else {
                //msg.payload = JSON.parse(ref.readCString(buf, 0));
                var payload = ref.readCString(buf, 0);
                msg.payload = JSON.parse(payload);
        
                //res.end(JSON.stringify(devices));
                node.warn("monitorNode: " +payload);
                if (msg.payload.deviceId != 0){
                    // valid record
                    node.status({fill: "green", shape: "dot", text: "devId="+msg.payload.deviceId});
                } else {
                    node.status({fill: "green", shape: "dot", text: "No Messages"});
                }
            }
            node.send(msg);
        });
        


        /*
        libradio.openThings_discover.async(iTimeOut, buf, function(err,res) {
            // callback
            if (err) node.error("openThings_discover err: " + err);
            //this.warn("openThings_switch returned res:" + res);
            //.catch(error)
            var devices = JSON.parse(ref.readCString(buf, 0));        
            res.end(JSON.stringify(devices));
        */

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
        this.on('close', function(){
            // tidy up state
            libradio.close_ener314rt();
        });
        
    }
    RED.nodes.registerType("openThings-monitor",OpenThingsMonitorNode);
}
