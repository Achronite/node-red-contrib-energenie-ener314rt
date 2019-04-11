/*
** Node-red config node for Energenie ENER314-RT board
** Author: Achronite, March 2019
**
** v0.1 Alpha
**
** File: ener314-rt.js
** Purpose: The idea behind this node is to centrally handle all the monitor and discover functions for the Energenie ENER314-RT Raspberry Pi adaptor
**          Devices register to use this node, this node then emits monitor events for the devices to handle in their own nodes
**
** Copyright - Achronite, March 2019
*/

var ref = require('ref');
var libradio = require('./libradio');
//var async = require('async');
var inited = false;

module.exports = function (RED) {
    function ener314rtAdaptor(config) {
        var events = require('events');
        this.events = new events.EventEmitter();
        var scope = this;
        var myInterval;

        this.nodeActive = true;

        if (!inited) {
            inited = true;
            // initialise stuff - do once

            // Initialise radio
            let ret = libradio.init_ener314rt(false);

            if (ret != 0)
                // can also happen if something else has beat us to it!
                scope.error(`Unable to initialise Energenie ENER314-RT board error: ${ret}`);
        };

        RED.nodes.createNode(this, config);

        // monitor mode - non-async version - this works, but does seem to use the main thread loop
        getMonitorMsg = () => {
            const buf = Buffer.alloc(500);
            //res =
            var recs = libradio.openThings_receive(buf);
            if (recs > 0) {
                var payload = ref.readCString(buf, 0);
                var msg = JSON.parse(payload);

                // inform the monitor devices that we have a message
                scope.events.emit('monitor', msg);
            } else {
                // no message
            }
        }

        // start the monitoring loop when we have listeners
        this.events.once('newListener', (event, listener) => {
            if (event === 'monitor') {
                scope.log("Monitor listener detected, starting monitor loop");
                // Do monitoring as we have listeners!
                myInterval = setInterval(getMonitorMsg, config.interval);
            }
        });

        // Close node, stop radio
        this.on('close', function (done) {
            clearInterval(myInterval);
            libradio.close_ener314rt();
            done();
        });

    }
    RED.nodes.registerType("energenie-board", ener314rtAdaptor);

    // TODO: discovery stuff; will also need learn mode too
    RED.httpAdmin.get("/board/devices", function (req, res) {

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        var buf = Buffer.alloc(500);

        // Call discovery function
        var ret = libradio.openThings_deviceList(buf, false);
        var devices = JSON.parse(ref.readCString(buf, 0));
        res.end(JSON.stringify(devices));
    });

    // learn mode for monitor devices
    RED.httpAdmin.get("/board/learn", function (req, res) {

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        var buf = Buffer.alloc(500);

        var iTimeOut = 10; // 10 seconds

        // Call discovery function
        var ret = libradio.openThings_deviceList(buf, true);;
        var devices = JSON.parse(ref.readCString(buf, 0));
        res.end(JSON.stringify(devices));
    });

}