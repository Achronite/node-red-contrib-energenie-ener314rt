/*
** Node-red control of Energenie ENER314-RT board for remote control of radio dimmer light switches
** Author: Achronite, February 2021
**
** v0.1 Alpha
**
** File: ook-dimmer.js
** Purpose: Node-Red wrapper for call to dimmer switch node for ENER314-RT OOK device
**
*/
"use strict";

var ener314rt = require('energenie-ener314rt');

module.exports = function (RED) {

    function OokDimmerNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;
        var board = RED.nodes.getNode(config.board);

        // CHECK CONFIG
        if (!board || board == null) {
            this.status({ fill: "red", shape: "ring", text: "Not configured" });
            return false;

        } else {

            this.on('input', function (msg) {

                this.status({ fill: "yellow", shape: "ring", text: "Sending" });

                //this.log(`IN ${msg.payload.zone}:${msg.payload.unit} state=${msg.payload.state}`);

                // dimmer uses switch number to set the light level
                var zone = Number(msg.payload.zone) || Number(config.zone) || 0;
                var xmits = Number(msg.payload.repeat) || 20;

                // Cater for other message types first (default to on)
                if (typeof msg.payload === 'number' || typeof msg.payload.brightness === 'number')
                    var brightness = msg.payload.brightness | msg.payload;
                else {
                    var brightness = 1;
                    if (typeof msg.payload == typeof true && !msg.payload)
                        brightness = 0
                    else if (typeof msg.payload.powerOn == typeof true && !msg.payload.powerOn)
                        brightness = 0
                    else if (typeof msg.payload.state == typeof true && !msg.payload.state)
                        brightness = 0
                    else if (typeof msg.payload.on == typeof true && !msg.payload.on)
                        brightness = 0
                    else if (msg.payload === "off" || msg.payload.powerOn === "off" || msg.payload.state === "off" || msg.payload.on === "off")
                        brightness = 0;
                }

                var switchNum = 1;
                var switchState = false;

                /*
                 translate the brightness level into a switch number with on/off value, where the dimmer switch expects the following inputs
                    '0': Channel 1 off: Turn dimmer Off
                    '1': Channel 1 on: Turn dimmer On at last light level set
                    '2': Channel 2 on: Set dimmer to 20% (turn on at 20% if off)
                    '3': Channel 3 on: Set dimmer to 30% (turn on at 30% if off)
                    '4': Channel 4 on: Set dimmer to 40% (turn on at 40% if off)
                    '5': Channel 2 off: Set dimmer to 60% (turn on at 60% if off)
                    '6': Channel 3 off: Set dimmer to 80% (turn on at 80% if off)
                    '7': Channel 4 off: Set dimmer to 100% (turn on at 100% if off)
                */
                switch (brightness) {
                    case 0:             // OFF (1 off)
                        node.status({ fill: "red", shape: "ring", text: "OFF " + zone });
                        break;
                    case 1:             // ON (1 on) at last light level
                        switchState = true;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone });
                        break;
                    case 2:
                    case 20:
                        switchNum = 2;
                        switchState = true;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":20%" });
                        break;
                    case 3:
                    case 30:
                        switchNum = 3;
                        switchState = true;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":30%" });
                        break;
                    case 4:
                    case 40:
                        switchNum = 4;
                        switchState = true;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":40%" });
                        break;
                    case 5:
                    case 60:
                        switchNum = 2;
                        switchState = false;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":60%" });
                        break;
                    case 6:
                    case 80:
                        switchNum = 3;
                        switchState = false;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":80%" });
                        break;
                    case 7:
                    case 100:
                        switchNum = 4;
                        switchState = false;
                        node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":100%" });
                        break;
                    default:
                        this.error(`Invalid brightness ${brightness}`);
                        node.status({ fill: "grey", shape: "dot", text: `ERROR: brightness=${brightness}` });
                        return;
                }


                this.log(`${zone}:${switchNum} state=${switchState} xmits=${xmits}`);

                // Invoke C function to do the send
                var ret = ener314rt.ookSwitch(zone, switchNum, switchState, xmits);
                if (ret >= 0) {
                    // return payload unchanged
                    node.send(msg);
                } else {
                    node.status({ fill: "grey", shape: "dot", text: `ERROR ${ret}` });
                }

            });

            this.on('close', function () {
                // TODO: tidy up state
            });
        }
    }
    RED.nodes.registerType("ook-dimmer", OokDimmerNode);


    RED.httpAdmin.get("/ook/teach", function (req, res) {
        var zone = Number(req.query.zone) || 0;
        var switchNum = 1;
        var ret = ener314rt.ookSwitch(zone, switchNum, true, 20);
        if (ret < 0) {
            console.error(`[ERROR] ener314rt - OOK teach failed ${ret}`);
            res.sendStatus(500);
        } else {
            res.sendStatus(200);
        }
    });

    RED.httpAdmin.get("/ook/off", function (req, res) {
        var zone = Number(req.query.zone) || 0;
        var switchNum = 1;
        var ret = ener314rt.ookSwitch(zone, switchNum, false, 20);
        if (ret < 0) {
            console.error(`[ERROR] ener314rt - OOK off failed ${ret}`);
            res.sendStatus(500);
        } else {
            res.sendStatus(200);
        }
    });
}
