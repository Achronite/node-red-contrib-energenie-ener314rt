/*
** Node-red config node for Energenie ENER314-RT board
** Author: Achronite, March 2019 - January 2020
**
** v0.3 Alpha
**
** File: ener314-rt.js
** Purpose: The idea behind this node is to centrally handle all the monitor and discover functions for the Energenie ENER314-RT Raspberry Pi adaptor
**          Devices register to use this node, this node then emits monitor events for the devices to handle in their own nodes
**
** Copyright - Achronite 2019 - 2021
*/
"use strict";

var ener314rt = require('energenie-ener314rt');

var inited = false;
var monitoring = false;

module.exports = function (RED) {
    function ener314rtAdaptor(config) {
        RED.nodes.createNode(this, config);
        var events = require('events');
        this.events = new events.EventEmitter();
        var ret;

        this.nodeActive = true;

        var scope = this;

        // initialise stuff - do once
        ret = ener314rt.initEner314rt(false);

        if (ret != 0) {
            // can also happen if something else has beat us to it!
            this.error(`Unable to initialise Energenie ENER314-RT board error: ${ret}`);
        } else {
            this.log(`ener314rt: radio initialised`);
            inited = true;
        }

        // async version in ener314rt uses a monitor thread that executes a callback when a message is received, it needs this callback passing in
        // this async call has dynamic sleep function dependent on eTRV messages being sent
        function startMonitoringThread(timeout) {
            scope.log(`starting monitor thread, timeout=${timeout}`);
            var ret = ener314rt.openThingsReceiveThread(timeout, (msg) => {
                var OTmsg = JSON.parse(msg);

                // notify the specific device node(s) of any msg received using events
                scope.events.emit(OTmsg.deviceId, OTmsg);
            });
        };

        // start the monitoring loop when we have openThings device listeners
        this.events.once('newListener', (event, listener) => {
            if (inited) {
                if (isNaN(config.timeout) || config.timeout === undefined) {
                    config.timeout = 5000;
                }
                monitoring = true;
                startMonitoringThread(Number(config.timeout));
            } else
                scope.error("Monitor unable to start, board not initialised");
        });

        // Close node, tidy-up and stop radio
        this.on('close', function (done) {
            if (monitoring) {
                scope.log("close triggered, waiting for monitor thread to complete");
                ener314rt.stopMonitoring();

                // Allow time for monitor thread to complete after config.timeout and close properly, do this as a cb to not block main event loop
                setTimeout(function () {
                    ener314rt.closeEner314rt();
                    inited = false;
                    monitoring = false;
                    done();
                }, config.timeout);
            } else {
                scope.log("close triggered, resetting device");
                ener314rt.closeEner314rt();
                inited = false;
                done();
            }
        });

    }
    RED.nodes.registerType("energenie-board", ener314rtAdaptor);

    // function called from the node-red GUI to return in memory list of OpenThings/FSK monitor devices
    RED.httpAdmin.get("/board/devices", function (req, res) {

        // Call discovery function
        var devices = ener314rt.openThingsDeviceList(false);
        res.end(devices);
    });

    // learn/discovery mode for OpenThings/FSK monitor devices
    RED.httpAdmin.get("/board/learn", function (req, res) {
        var iTimeOut = 10; // 10 seconds

        // Call discovery function - this time force a scan as button was pressed
        var devices = ener314rt.openThingsDeviceList(true);
        res.end(devices);
    });

}