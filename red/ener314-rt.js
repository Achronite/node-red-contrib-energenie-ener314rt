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
** Copyright - Achronite 2019, 2020
*/
"use strict";

var path = require('path');

// TODO: Release n-api functions as a separate npm module
var ener314rt = require(path.join(__dirname, '../build/Release/ener314rt'));

var inited = false;

module.exports = function (RED) {
    function ener314rtAdaptor(config) {
        var events = require('events');
        this.events = new events.EventEmitter();
        var scope = this;
        var ret;

        this.nodeActive = true;

        if (!inited) {
            // initialise stuff - do once
            ret = ener314rt.initEner314rt(false);

            if (ret != 0) {
                // can also happen if something else has beat us to it!
                scope.error(`Unable to initialise Energenie ENER314-RT board error: ${ret}`);
            } else {
                scope.log(`ener314-rt: radio initialised`);
                inited = true;
                RED.nodes.createNode(this, config);
            }
        };

        // async version in ener314rt uses a monitor thread that executes a callback when a message is received, it needs this callback passing in
        // this async call has dynamic sleep function dependent on eTRV messages being sent
        function startMonitoringThread(timeout) {
            scope.log(`starting monitor thread, timeout=${timeout}`);
            var ret = ener314rt.openThingsReceiveThread(timeout, (msg) => {
                //scope.log(`ener314-rt cb: Rx=${msg}`);
                var OTmsg = JSON.parse(msg);

                // TODO: make emits device specific
                scope.events.emit('monitor', OTmsg);
            });
        };

        // start the monitoring loop when we have listeners
        this.events.once('newListener', (event, listener) => {
            scope.log("newListener()");
            if (event === 'monitor' && inited) {
                if (isNaN(config.timeout) || config.timeout === undefined) {
                    config.timeout = 5000;
                }
                startMonitoringThread(Number(config.timeout));

                // TODO: Handle quit of monitor function (maybe ignore the once?)
            } else
                scope.error("Monitor unable to start, board not initialised");
        });

        // Close node, tidy-up and stop radio
        this.on('close', function (done) {
            this.log("ener314-rt: close triggered");
            ener314rt.stopMonitoring();

            // Allow time for monitor thread to complete after config.timeout and close properly, do this as a cb to not block main event loop
            setTimeout(function () {
                console.log("ener314-rt: finalizing close");
                ener314rt.closeEner314rt();
                inited = false;
                done();
            }, config.timeout);
        });


        this.on('input', function (msg) {
            this.log(`input triggered ${msg}`)
        });

    }
    RED.nodes.registerType("energenie-board", ener314rtAdaptor);

    // function called from the node-red GUI
    RED.httpAdmin.get("/board/devices", function (req, res) {

        // Call discovery function
        var devices = ener314rt.openThingsDeviceList(false);
        //var devices = JSON.parse(ref.readCString(buf, 0));
        //res.end(JSON.stringify(devices));
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