/*
** Node-red module that supports transmitting any raw OOK or FSK based payload using the Energenie ENER314-RT raspberry pi board
** Use this node if you want to control devices other than those provided by energenie, if you have energenie devices use
** the other nodes in this module
**
** Author: Achronite, December 2018 - April 2019
**
** v0.1
**
** File: ener314rt-raw.js
** Purpose: Node-Red wrapper for base ENER314-RT C calls
**
** Implemented:
**  OOK & FSK transmit of any payload
**  control of energenie OOK devices, including house code setting.  This allows 1048575*6 devices to be controlled from one Raspberry Pi
**
** To Do:
**  OpenThings protocol encode (used by MiHome transmitting devices)
**  OpenThings message body encryption
*/
"use strict";

var libradio = require( './libradio');

module.exports = function(RED) {

    function Ener314RTrawNode(config) {
        RED.nodes.createNode(this,config);
        var node = this;
        var board = RED.nodes.getNode(config.board);

        // initialise the radio as part of the node constructor
        libradio.radio_init();

        node.on('input', function(msg) {
            // Check params
            if ( msg.payload.raw === undefined || msg.payload.raw == null ) {
                // No payload!
                this.error("Missing msg.payload.raw", msg);
            } else {
                //var a = new IntArray(under);
                //this.warn("Setting modulation");
                var modulation = msg.payload.modulation || 0;  //OOK by default

                libradio.radio_modulation(modulation);  // 0=OOK, 1=FSK

                this.status({fill:"yellow",shape:"ring",text:"transmitting"});

                // This node transmits a raw payload, it's up to the user to do any encoding first
                var tPayload = msg.payload.raw;

                this.warn("Sending "+ tPayload);
                libradio.radio_transmit(tPayload,tPayload.length,40);
                this.status({fill:"green",shape:"ring",text:switchString});
            }

            node.send(msg);
        });
    }
    RED.nodes.registerType("ener314rt-raw",Ener314RTrawNode);
}
