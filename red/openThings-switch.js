/*
** Node-red control of Energenie ENER314-RT board for remote control of radio sockets
** Author: Achronite, December 2019 - January 2019
**
** v0.1 Alpha
**
** File: OpenThings-switch.js
** Purpose: Node-Red wrapper for call to switch node for ENER314-RT FSK/OpenThings (aka Monitor) devices
**
*/
var libradio = require( './libradio');

module.exports = function(RED) {

    function OpenThingsSwitchNode(config) {
        RED.nodes.createNode(this,config);
        var node = this;

        // Initialise radio
        libradio.radio_init();

        node.on('input', function(msg) {
            this.status({fill:"yellow",shape:"ring",text:"Sending"});
            var deviceId = Number(config.deviceId) || 0;
            var xmits = 20;

            // Check all variables before we call the C routine to transmit, msg.payload overrides any defaults set in node

            // Check OpenThings deviceId
            if ( deviceId == 0 || isNaN(deviceId) ) {
                this.error("DeviceId err: " + deviceId + " (" + typeof(deviceId) + ")");
            }


            // Check Switch State (default to off=0)
            var switchState = 0;
            if (typeof msg.payload == typeof true)
                switchState = msg.payload ? 1 : 0;
            else if (typeof msg.payload.powerOn == typeof true)
                switchState = msg.payload.powerOn  ? 1 : 0;
            else if (msg.payload === "on" || msg.payload.powerOn === "on")
                switchState = 1;
            
             // Check xmit times (advanced), 26ms per payload transmission
            if (Number(msg.payload.repeat))
                xmits = Number(msg.payload.repeat);

            this.warn("Switching deviceId:" + deviceId + " switchState:" + switchState + " xmits:" + xmits);

            // Send payload
            var res = libradio.openThings_switch(2, deviceId, switchState, xmits);

            // Set the node status in the GUI
            switch (switchState) {
                case 1:
                    this.status({fill:"green",shape:"dot",text:"ON"});
                    break;
                case 0:            
                    this.status({fill:"red",shape:"ring",text:"OFF"});
                    break;
            }

            // return payload unchanged
            node.send(msg);
        });
    }
    RED.nodes.registerType("openThings-switch",OpenThingsSwitchNode);
}