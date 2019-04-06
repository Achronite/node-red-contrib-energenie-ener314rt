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
var async = require('async');
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
                node.error(`Unable to initialise Energenie ENER314-RT board error: ${ret}`);

        };

        RED.nodes.createNode(this, config);

        getMonitorMsg = () => {
            //console.log("ENER314-RT: getMonitorMsg()");
            process.stdout.write("\n<");
            const buf = Buffer.alloc(500);
            //res =
            process.stdout.write("{");
            var recs = libradio.openThings_receive(20, buf);
            console.log("}");
            if (recs > 0) {
                console.log("ENER314-RT: got message recs=" + recs);
                var payload = ref.readCString(buf, 0);
                var msg = JSON.parse(payload);

                // inform the monitor devices that we have a message
                // TODO: filter on deviceId here?
                scope.events.emit('monitor', msg);
            } else {
                // no messages
            }
            console.log(`ENER314-RT: getMonitorMsg() done`);

            // // RECHECK if we have listeners
            // if (scope.nodeActive) {
            //     console.log(`ENER314-RT: setTimeout(${config.interval})`);
            //     //setTimeout(function () { getMonitorMsg(); }, config.interval);
            // } else
            //     console.log("ENER314-RT: no listeners, monitoring stopped");
            // console.log(`ENER314-RT: done`);
            // return true;
        }

        // Monitor_loop: Receive radio transmissions for valid incoming OpenThings messages
        // TODO: this code does work, but has a tendancy to block things, replace with non-async version
        // monitorLoop = async () => {
        //     do {
        //         let promiseOfData = getOpenThingsMessage();
        //         try {
        //             var msg = await promiseOfData;
        //             console.log(`### promise returned: ${msg.deviceId}`);

        //             // inform the monitor devices that we have a message
        //             // TODO: filter on deviceId here?
        //             scope.events.emit('monitor', msg);

        //         } catch (err) {
        //             console.log(`ENER314-RT: caught error ${err}`);
        //         }
        //     } while (scope.events.listenerCount('monitor') > 0)

        //     // all listeners gone, stop loop
        //     console.log(`ENER314-RT: monitoring stopped, no listeners`);
        // }

        // // receive an OpenThings radio message (called as an async function so we do not block the node.js event loop)
        // getOpenThingsMessage = () => {
        //     return new Promise((resolve, reject) => {
        //         var buf = Buffer.alloc(500);
        //         //res =
        //         libradio.openThings_receive.async(20, buf, function (err, recs) {
        //             if (err) {
        //                 console.log("!!! got message err= " + err);
        //                 reject(err);
        //             } else {
        //                 console.log("### got message recs=" + recs);
        //                 var payload = ref.readCString(buf, 0);
        //                 resolve(JSON.parse(payload));
        //             }
        //         });
        //     })
        // }

        // start the monitoring loop if we have listeners
        this.events.once('newListener', (event, listener) => {
            if (event === 'monitor') {
                console.log("ENER314-RT: monitor listener detected, starting monitor loop");
                // Do monitoring as we have listeners!
                myInterval = setInterval(getMonitorMsg, config.interval);
            }
        });

        // Close node, stop radio
        this.on('close', function (done) {
            clearInterval(myInterval);
            libradio.close_ener314rt();            
            console.log("ENER314-RT: radio closed");
            done();
        });

    }
    RED.nodes.registerType("energenie-board", ener314rtAdaptor);

    // TODO: discovery stuff; will also need learn mode too
    RED.httpAdmin.get("/board/devices", function (req, res) {

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        var buf = Buffer.alloc(500);

        var iTimeOut = 10; // 10 seconds

        // Call discovery function
        var ret = libradio.openThings_deviceList(iTimeOut, buf);

        var devices = JSON.parse(ref.readCString(buf, 0));

        res.end(JSON.stringify(devices));
    });


}