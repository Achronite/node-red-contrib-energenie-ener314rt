/*
** Node-red module that supports transmitting any raw OOK or FSK based payload using the Energenie ENER314-RT raspberry pi board
** Use this node if you want to control devices other than those provided by energenie, if you have energenie devices use
** the other nodes in this module
**
** Author: Achronite, December 2018 - September 2019
**
** v0.3
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

var ener314rt = require('energenie-ener314rt');

module.exports = function (RED) {

    function Ener314RTrawNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;
        var board = RED.nodes.getNode(config.board);

        // CHECK CONFIG
        if (!board || board == null) {
            this.status({ fill: "red", shape: "ring", text: "Not configured" });
            return false;
        } else {

            node.on('input', function (msg) {
                // Check params
                if (msg.payload.raw === undefined || msg.payload.raw == null) {
                    // No payload!
                    this.error("Missing msg.payload.raw", msg);
                } else {
                    //var a = new IntArray(under);
                    var modulation = msg.payload.modulation || 0;  //OOK by default

                    this.status({ fill: "yellow", shape: "ring", text: "transmitting" });

                    if (typeof msg.payload.raw == "object") {
                        // This node transmits a raw byte array payload, it's up to the user to do any encoding first
                        var tPayload = Buffer.from(msg.payload.raw);

                        var ret = ener314rt.sendRadioMsg(modulation, 20, tPayload);
                        msg.payload.status = ret;
                        if (ret == 0) {
                            this.status({ fill: "green", shape: "ring", text: "sent "+tPayload.length+" bytes" });
                        } else {
                            this.status({ fill: "red", shape: "ring", text: `error ${ret}` });
                        }
                    } else {
                        this.error("msg.payload.raw is not an object");
                        this.status({ fill: "red", shape: "ring", text: `payload error` });
                    }
                }

                node.send(msg);
            });
        }
    }
    RED.nodes.registerType("ener314rt-raw", Ener314RTrawNode);
}
