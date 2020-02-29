/*
** Node-red control of Energenie ENER314-RT board for remote control of radio sockets
** Author: Achronite, December 2018 - September 2019
**
** v0.3 Alpha
**
** File: ook-switch.js
** Purpose: Node-Red wrapper for call to switch node for ENER314-RT OOK device
**
*/
"use strict";

var ener314rt = require('energenie-ener314rt');

module.exports = function (RED) {

    function OokSwitchNode(config) {
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

                var zone = Number(msg.payload.zone) || Number(config.zone) || 0;
                var switchNum = Number(msg.payload.switchNum) || Number(msg.payload.unit) || Number(config.switchNum) || 1;
                var xmits = Number(msg.payload.repeat) || 20;

                // Check Switch State in message (default to off)
                var switchState = false;
                if (typeof msg.payload == typeof true)
                    switchState = msg.payload;
                else if (typeof msg.payload.powerOn == typeof true)
                    switchState = msg.payload.powerOn;
                else if (typeof msg.payload.state == typeof true)
                    switchState = msg.payload.state;
                else if (msg.payload === "on" || msg.payload.powerOn === "on" || msg.payload.state === "on")
                    switchState = true;

                // Check Switch Number
                if (switchNum < 0 || switchNum > 6 || isNaN(switchNum)) {
                    this.error("SwitchNum err: " + switchNum + " (" + typeof (switchNum) + ")");
                }

                //this.log(`${zone}:${switchNum} state=${switchState} xmits=${xmits}`);

                // Invoke C function to do the send
                if (ener314rt.ookSwitch(zone, switchNum, switchState, xmits) == 0) {
                    switch (switchState) {
                        case true:
                            node.status({ fill: "green", shape: "dot", text: "ON " + zone + ":" + switchNum });
                            break;
                        case false:
                            node.status({ fill: "red", shape: "ring", text: "OFF " + zone + ":" + switchNum });
                            break;
                    }
                    // return payload unchanged
                    node.send(msg);
                } else {
                    node.status({ fill: "grey", shape: "dot", text: "ERROR" });
                }

            });

            this.on('close', function () {
                // TODO: tidy up state
            });
        }
    }
    RED.nodes.registerType("ook-switch", OokSwitchNode);


    RED.httpAdmin.get("/ook/teach", function (req, res) {
        var zone = Number(req.query.zone) || 0;
        var switchNum = Number(req.query.switchNum);
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
        var switchNum = Number(req.query.switchNum);
        var ret = ener314rt.ookSwitch(zone, switchNum, false, 20);
        if (ret < 0) {
            console.error(`[ERROR] ener314rt - OOK off failed ${ret}`);
            res.sendStatus(500);
        } else {
            res.sendStatus(200);
        }
    });
}
