/*
** Node-red config node for Energenie ENER314-RT board
** Author: Achronite, March - August 2019
**
** v0.3 Alpha
**
** File: ener314-rt.js
** Purpose: The idea behind this node is to centrally handle all the monitor and discover functions for the Energenie ENER314-RT Raspberry Pi adaptor
**          Devices register to use this node, this node then emits monitor events for the devices to handle in their own nodes
**
** Copyright - Achronite 2019
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
        var myInterval, ret;

        this.nodeActive = true;

        if (!inited) {
            // initialise stuff - do once

            // Initialise radio
            //ret = libradio.init_ener314rt(false);
            //scope.log(`radio_init returned ${ret}`);

            ret = ener314rt.initEner314rt(false);
            console.log(`N-API radio_init returned ${ret}`);

            if (ret != 0) {
                // can also happen if something else has beat us to it!
                scope.error(`Unable to initialise Energenie ENER314-RT board error: ${ret}`);
            } else {
                inited = true;
                RED.nodes.createNode(this, config);
            }
        };

        // monitor mode - non-async version - this works, but does seem to use the main thread loop
        // TODO: async version
        /*
        function getMonitorMsg() {
            //const buf = Buffer.alloc(500);
            //res =

            var recs = ener314rt.openThingsReceive();
            if (recs > 0) {
                var payload = ref.readCString(buf, 0);
                var msg = JSON.parse(payload);
            var msg = ener314rt.openThingsReceive();
            //scope.log(`received ${msg}`);

            // msg returns -ve int value if nothing received, or a string
            if (typeof (msg) === 'string' || msg instanceof String) {
                // inform the monitor devices that we have a message
                var OTmsg = JSON.parse(msg);
                scope.events.emit('monitor', OTmsg);
            } else {
                // no message
            }
        };
        */

        // async version in ener314rt uses a callback to return monitor messages directly (collected below), it needs the callback passing in
        // this async call has dynamic sleep function dependent on eTRV messages being sent
        function startMonitoringThread(timeout) {
            console.log(`startMonitorThread(${timeout}, cb)\n`);
            var ret = ener314rt.openThingsReceiveThread(timeout, (msg) => {
                //console.log(`asyncOpenThingsReceive ret=${ret}`);
                console.log(`ener314-rt cb: Rx=${msg}`);
                var OTmsg = JSON.parse(msg);
                scope.events.emit('monitor', OTmsg);
            });
        };


        // async version in ener314rt emits string monitor messages directly (collected below), it needs the emitter passing in
        // this async call does not have any sleep functions, so we need to deal with that in js code on RxMessage return
        /*
        function asyncGetMonitorMsg () {
            var ret = ener314rt.asyncOpenThingsReceive(scope.events.emit.bind(scope.events));  //emitter.emit.bind(emitter)
            //scope.log(`received ${msg}`);
            //scope.log(`asyncOpenThingsReceive ret=${ret}`);
            
            // msg returns -ve int value if nothing received, or a string
            if (ret != 0)  {
                // bad Rx command
                scope.error("Monitor unable to retrieve messages");
            }
        };
    
    
        // listen for valid OT messages to be sent from async Rx, and forward them on to the devices
        scope.events.on('RxMessage', function (msg) {
            // msg returns -ve int value if nothing received, or a string
            if (typeof(msg) === 'string' || msg instanceof String)  {
                // inform the monitor devices that we have a message
                var OTmsg = JSON.parse(msg);
                scope.events.emit('monitor', OTmsg);
            } else {
                // no message
            }
    
            // function returned, lets trigger getting next message
            //setImmediate(asyncGetMonitorMsg);
    
        });
    
    
        // start the monitoring loop when we have listeners
        this.events.once('newListener', (event, listener) => {
            if (event === 'monitor' && inited) {
                scope.log("Monitor listener detected, starting monitor loop, poll interval=" + config.interval + "ms");
                // Do monitoring as we have listeners!
                //myInterval = setInterval(getMonitorMsg, config.interval);
    
                // fixed Interval version
                myInterval = setInterval(asyncGetMonitorMsg, config.interval);                                     // TODO Dynamic sleep time here
    
                // do once version
                //setImmediate(asyncGetMonitorMsg);
            } else
                scope.error("Monitor unable to start, board not initialised");
        });
        */

        // start the monitoring loop when we have listeners
        this.events.once('newListener', (event, listener) => {
            if (event === 'monitor' && inited) {
                scope.log("Monitor listener detected, starting monitor loop, poll interval=" + config.interval + "ms");
                // Do monitoring as we have listeners!
                //myInterval = setInterval(getMonitorMsg, config.interval);

                // do once version
                //setImmediate(startMonitoringThread(config.interval));
                if (Number(config.interval) == NaN) {
                    config.interval = 5000;
                }
                startMonitoringThread(Number(config.interval));

                // TODO: Handle quit of monitor function (maybe ignore the once?)
            } else
                scope.error("Monitor unable to start, board not initialised");
        });



        // Close node, stop radio
        this.on('close', function (done) {
            clearInterval(myInterval);
            ener314rt.stopMonitoring();
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