/*
** Node-red config node for Energenie ENER314-RT board
** Author: Achronite, March - August 2019
**
** v0.2 Alpha
**
** File: ener314-rt.js
** Purpose: The idea behind this node is to centrally handle all the monitor and discover functions for the Energenie ENER314-RT Raspberry Pi adaptor
**          Devices register to use this node, this node then emits monitor events for the devices to handle in their own nodes
**
** Copyright - Achronite 2019
*/
"use strict";

//var libradio = require('./libradio');
var ener314rt = require('/home/pi/development/node-red-contrib-energenie-ener314rt/build/Release/ener314rt');

//var async = require('async');
var inited = false;

module.exports = function (RED) {
    function ener314rtAdaptor(config) {
        var events = require('events');
        this.events = new events.EventEmitter();
        var scope = this;
        var myInterval, ret;

        this.nodeActive = true;

        if (!inited) {
            // initialise stuff - do once

            // Initialise radio
            //ret = libradio.init_ener314rt(false);
            //scope.log(`radio_init returned ${ret}`);

            ret = ener314rt.initEner314rt(false);
            console.log(`N-API radio_init returned ${ret}`);

            if (ret != 0){
                // can also happen if something else has beat us to it!
                scope.error(`Unable to initialise Energenie ENER314-RT board error: ${ret}`);
            } else {
                inited = true;
                RED.nodes.createNode(this, config);
            }
        };     

        // monitor mode - non-async version - this works, but does seem to use the main thread loop
        // TODO: async version
        function getMonitorMsg () {
            //const buf = Buffer.alloc(500);
            //res =
            /*
            var recs = ener314rt.openThingsReceive();
            if (recs > 0) {
                var payload = ref.readCString(buf, 0);
                var msg = JSON.parse(payload);
            */
            var msg = ener314rt.openThingsReceive();
            //scope.log(`received ${msg}`);
            
            // msg returns -ve int value if nothing received, or a string
            if (typeof(msg) === 'string' || msg instanceof String)  {
                // inform the monitor devices that we have a message
                var OTmsg = JSON.parse(msg);
                scope.events.emit('monitor', OTmsg);
            } else {
                // no message
            }
        };

        // start the monitoring loop when we have listeners
        this.events.once('newListener', (event, listener) => {
            if (event === 'monitor' && inited) {
                scope.log("Monitor listener detected, starting monitor loop, poll interval=" + config.interval + "ms");
                // Do monitoring as we have listeners!
                myInterval = setInterval(getMonitorMsg, config.interval);
            } else
                scope.error("Monitor unable to start, board not initialised");
        });

        // Close node, stop radio
        this.on('close', function (done) {
            clearInterval(myInterval);
            ener314rt.closeEner314rt();
            done();
        });

    }
    RED.nodes.registerType("energenie-board", ener314rtAdaptor);

    RED.httpAdmin.get("/board/devices", function (req, res) {

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        //var buf = Buffer.alloc(500);

        // Call discovery function
        var devices = ener314rt.openThingsDeviceList(false);
        //var devices = JSON.parse(ref.readCString(buf, 0));
        //res.end(JSON.stringify(devices));
        res.end(devices);
    });

    // learn/discovery mode for FSK monitor devices
    RED.httpAdmin.get("/board/learn", function (req, res) {

        // allocate the return buffer here for the JSON response, C routine does not do malloc()

        var iTimeOut = 10; // 10 seconds

        // Call discovery function - this time force a scan as button was pressed
        var devices = ener314rt.openThingsDeviceList(true);
        res.end(devices);
    });

}