var ref = require('ref');
var libradio = require('./libradio');
var async = require('async');
var inited = false;

/*
* The idea behind this node is to handle all the monitor and discover functions for the Energenie ENER314-RT Raspberry Pi adaptor
*
* I've no idea if this is how to achieve this, but it seems sort of right :)
*
* Copyright - Achronite, March 2019
*/
module.exports = function (RED) {
    function ener314rtAdaptor(config) {
        var events = require('events');

        this.devices = [];
        this.events = new events.EventEmitter();
        var scope = this;

        if (!inited) {
            inited = true;
            // initialise stuff - do once

            // TODO - Initialise radio - need to deal with unlock too
            //libradio.radio_init();
        };

        RED.nodes.createNode(this, config);

        // Monitor_loop: Receive radio transmissions for valid incoming OpenThings messages
        monitorLoop = async () => {
            do {
                let promiseOfData = getOpenThingsMessage();
                try {
                    var msg = await promiseOfData;
                    console.log(`### promise returned: ${msg.deviceId}`);

                    // inform the monitor devices that we have a message
                    // TODO: filter on deviceId here?
                    scope.events.emit('monitor', msg);
                } catch (err) {
                    console.log(`ENER314-RT: caught error ${err}`);
                }
            } while (scope.events.listenerCount('monitor') > 0)
            // all listeners gone, stop loop
            console.log(`ENER314-RT: monitoring stopped, no listeners`);
        }

        // receive an OpenThings radio message (called as an async function so we do not block the node.js event loop)
        getOpenThingsMessage = () => {
            return new Promise((resolve, reject) => {
                var buf = Buffer.alloc(500);
                //res =
                libradio.openThings_receive.async(20, buf, function (err, recs) {
                    if (err) {
                        console.log("!!! got message err= " + err);
                        reject(err);
                    } else {
                        console.log("### got message recs=" + recs);
                        var payload = ref.readCString(buf, 0);
                        resolve(JSON.parse(payload));
                    }
                });
            })
        }

        //only start the monitoring loop if we have listeners
        this.events.once('newListener', (event, listener) => {
            if (event === 'monitor') {
                console.log("ENER314-RT: monitor listener detected, starting monitor loop");
                // Do monitoring as we have listeners!
                monitorLoop();
            }
        });

        /*
                //only start monitor loop if we have listeners
                this.events.once('newListener', (event, listener) => {
                    if (event === 'monitor') {
                        console.log("### monitor listener detected: " + listener);
                        // Do monitoring as we have listeners!
                        //scope.monitorLoop();
        
                        console.log("### monitorLoop(): started");
                        scope.events.emit('monitor', "monitorLoop() started");
        
                        var count = 0;
                        var buf = Buffer.alloc(500);
                        // Using async.whilst() here, not 100% sure if this uses the node.js thread pool or not
        
                        async.whilst(       // test, fn, callback
                            function () {
                                //myEmitter.emit('monitor',count);
                                console.log("### monitorLoop(): check, count=" + count);
                                return count < 10;
                            },//check condition.
                            function (callback) {
                                console.log("### monitorLoop(): getting message " + count);
                                libradio.openThings_receive.async(20, buf, function (err, res) {
                                    if (err) {
                                        //RED.log.error("openThings_receive err: " + err);
                                        callback(err, null);
                                    } else {
                                        console.log("### monitorLoop(): got message " + count);
                                        //var payload = ref.readCString(buf, 0);
                                        //var msg = JSON.parse(payload);
                                        //console.log("### monitorLoop(): decoded\n" + msg);
        
                                        //console.log(msg);
                                        count++;
                                        //                    if ((listeners = this.events.listenerCount('monitor')) > 0){
                                        console.log("### monitorLoop(): emit");
                                        //scope.events.emit('monitor', "Mrs Squirrel");
                                    }
        
                                    callback(null, null);
                                });
                                //                    node.events.emit('monitor', "after cb()");
                            },
                            function (err, n) {    //final result
                                if (err) {
                                    console.log("Some error occured");
                                } else {
                                    console.log("All loops done");
                                }
                            }
                        )
                    }
                });
        */

        // register a local listener
        // console.log("###LOCAL: registering local listener");
        // this.events.on('monitor', function (msg) {
        //     console.log(`###LOCAL: got: ${msg}`);
        // });


        //        var recs;
        //        var listeners = 0;
        //        if ((listeners = this.events.listenerCount('monitor')) > 0) {
        //            console.log(`emitting to ${listeners} listeners`);
        // this.events.emit('monitor', "Testing local emit");

        // Interval and event emitter
        //this.myEmitter = new events.EventEmitter();

        /*
                function initialise() {
                    // Setting URL and headers for request
                    var buf = Buffer.alloc(500);
        
                    // Return new promise 
                    return new Promise(function (resolve, reject) {
                        // Do async job
                        libradio.openThings_receive.async(20, buf, function(err,res) {
                            //var msg = {};
                            // callback
                            RED.log.info("getMessage() async returned ");
                            if (err) {
                                RED.log.error("openThings_receive err: " + err);
                                reject(err);
                            } else {
                                //msg.payload = JSON.parse(ref.readCString(buf, 0));
                                var payload = ref.readCString(buf, 0);
                                RED.log.info("board receive async");
                                resolve(JSON.parse(payload));
                        
                                //res.end(JSON.stringify(devices));
                                //node.warn("board received: " +payload);
        
                                //this.events.emit(8294,"12345");
                                
                            }
                            //node.send(msg);
                
                            // lets go round again...
                            //this.getMessage();
                        })
                    })
                } //initialise
        */

        /*
                var initialisePromise = initialise();
                initialisePromise.then(function(result) {
                    userDetails = result;
                    console.log("Got message");
                    // Use user details from here
                    console.log(userDetails);
        
                }, function(err) {
                    console.log(err);
                })
                
                console.log("after promise code");
        */

        /* this works, but is blocking; it even stops node-red init
        **
        var count = 0;
        async.whilst(       // test, fn, callback
            function () {
                return count < 10;
            },//check condition.
            function (callback) {
                console.log("***Getting message "+count);
                var buf = Buffer.alloc(500);
                res = libradio.openThings_receive(20, buf );
                console.log("board receive async, res=",res);
                var payload = ref.readCString(buf, 0);
                var msg = JSON.parse(payload);

                console.log("***Got message "+count);
                // Use user details from here
                console.log(msg);
                count++;
                callback(null, count);
            },
            function (err, n) {    //final result
                if (err) {
                    console.log("Some error occured");
                } else {
                    console.log("All loops done");
                }
            }
        );
        */





        // start first get

        /*
                getMessage = function()
                {
                    var buf = Buffer.alloc(500);
                    RED.log.info("getMessage() called");
                    libradio.openThings_receive.async(20, buf, function(err,res) {
                        //var msg = {};
                        // callback
                        RED.log.info("getMessage() async returned ");
                        if (err) {
                            RED.log.error("openThings_receive err: " + err);
                        } else {
                            //msg.payload = JSON.parse(ref.readCString(buf, 0));
                            var payload = ref.readCString(buf, 0);
                            var msg = JSON.parse(payload);
                    
                            //res.end(JSON.stringify(devices));
                            //node.warn("board received: " +payload);
                            RED.log.info("board receive async");
                            //this.events.emit(8294,"12345");
                            if(scope.nodeActive == true) { setTimeout(function(){ scope.recheck(); }, config.interval); }
                        }
                        //node.send(msg);
            
                        // lets go round again...
                        //this.getMessage();
                    });
                    RED.log.info("async completed");
        
                }; // this.getMessage()
        
                // start first get
                getMessage();
                RED.log.info("line after getMessage");
        */

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


    /*
    function control(zone, switchNum, switchState, xmits){
        var y = libradio.OokSend(zone, switchNum, switchState, xmits);
        return y;
    }
    */
    /*
        var boardPool = (function() {
            var devices = {};
            return {
                get:function(boardConfig) {
                    // just return the connection object if already have one
                    var id = boardConfig.id;
                    if (devices[id]) { return devices[id]; }
    
                    devices[id] = (function() {
                        var obj = {
                            _emitter: new events.EventEmitter(),
                            board: null,
                            _closing: false,
                            tout: null,
                            queue: [],
                            on: function(a,b) { this._emitter.on(a,b); },
                            close: function(cb) { this.board.close(cb); },
                            write: function(m,cb) { this.board.write(m,cb); },
                            enqueue: function(zone, switchNum, switchState, xmits, cb) {
                                 var qobj = {
                                    mode: 0,  // Hardcode OOK for now
                                    zone: zone,
                                    switchNum: switchNum,
                                    switchState: switchState,
                                    xmits: xmits,
                                    cb: cb
                                }
                                this.queue.push(qobj);
                                // If we're enqueing the first message in line,
                                // we shall send it right away
                                if (this.queue.length === 1) {
                                    this.writehead();
                                }
                            },
                            writehead: function() {
                                if (!this.queue.length) { return; }
                                
                                var qobj = this.queue[0];
                                this.write(qobj.payload,qobj.cb);
                                var msg = qobj.msg;
                                var timeout = msg.timeout || responsetimeout;
                                this.tout = setTimeout(function () {
                                    this.tout = null;
                                    var msgout = obj.dequeue() || {};
                                    msgout.port = port;
                                    // if we have some leftover stuff, just send it
                                    if (i !== 0) {
                                        var m = buf.slice(0,i);
                                        m = Buffer.from(m);
                                        i = 0;
                                        if (binoutput !== "bin") { m = m.toString(); }
                                        msgout.payload = m;
                                    }
                                    // Notify the sender that a timeout occurred
                                    obj._emitter.emit('timeout',msgout,qobj.sender);
                                }, timeout);
                            },
                            dequeue: function() {
                                // if we are trying to dequeue stuff from an
                                // empty queue, that's an unsolicited message
                                if (!this.queue.length) { return null; }
                                var msg = Object.assign({}, this.queue[0].msg);
                                msg = Object.assign(msg, {
                                    request_payload: msg.payload,
                                    request_msgid: msg._msgid,
                                });
                                delete msg.payload;
                                if (this.tout) {
                                    clearTimeout(obj.tout);
                                    obj.tout = null;
                                }
                                this.queue.shift();
                                this.writehead();
                                return msg;
                            },
                        }
                        var olderr = "";
                        var setupboard = function() {
                            obj.board = new boardp(port,{
                                baudRate: baud,
                                dataBits: databits,
                                parity: parity,
                                stopBits: stopbits,
                                //parser: boardp.parsers.raw,
                                autoOpen: true
                            }, function(err, results) {
                                if (err) {
                                    if (err.toString() !== olderr) {
                                        olderr = err.toString();
                                        RED.log.error(RED._("board.errors.error",{port:port,error:olderr}));
                                    }
                                    obj.tout = setTimeout(function() {
                                        setupboard();
                                    }, settings.boardReconnectTime);
                                }
                            });
                            obj.board.on('error', function(err) {
                                RED.log.error(RED._("board.errors.error",{port:port,error:err.toString()}));
                                obj._emitter.emit('closed');
                                obj.tout = setTimeout(function() {
                                    setupboard();
                                }, settings.boardReconnectTime);
                            });
                            obj.board.on('close', function() {
                                if (!obj._closing) {
                                    RED.log.error(RED._("board.errors.unexpected-close",{port:port}));
                                    obj._emitter.emit('closed');
                                    obj.tout = setTimeout(function() {
                                        setupboard();
                                    }, settings.boardReconnectTime);
                                }
                            });
                            obj.board.on('open',function() {
                                olderr = "";
                                RED.log.info(RED._("board.onopen",{port:port,baud:baud,config: databits+""+parity.charAt(0).toUpperCase()+stopbits}));
                                if (obj.tout) { clearTimeout(obj.tout); obj.tout = null; }
                                //obj.board.flush();
                                obj._emitter.emit('ready');
                            });
    
                            obj.board.on('data',function(d) {
                                function emitData(data) {
                                    var m = Buffer.from(data);
                                    var last_sender = null;
                                    if (obj.queue.length) { last_sender = obj.queue[0].sender; }
                                    if (binoutput !== "bin") { m = m.toString(); }
                                    var msgout = obj.dequeue() || {};
                                    msgout.payload = m;
                                    msgout.port = port;
                                    obj._emitter.emit('data',
                                        msgout,
                                        last_sender);
                                }
    
                                for (var z=0; z<d.length; z++) {
                                    var c = d[z];
                                    // handle the trivial case first -- single char buffer
                                    if ((newline === 0)||(newline === "")) {
                                        emitData(new Buffer([c]));
                                        continue;
                                    }
    
                                    // save incoming data into local buffer
                                    buf[i] = c;
                                    i += 1;
    
                                    // do the timer thing
                                    if (spliton === "time" || spliton === "interbyte") {
                                        // start the timeout at the first character in case of regular timeout
                                        // restart it at the last character of the this event in case of interbyte timeout
                                        if ((spliton === "time" && i === 1) ||
                                            (spliton === "interbyte" && z === d.length-1)) {
                                            // if we had a response timeout set, clear it:
                                            // we'll emit at least 1 character at some point anyway
                                            if (obj.tout) {
                                                clearTimeout(obj.tout);
                                                obj.tout = null;
                                            }
                                            obj.tout = setTimeout(function () {
                                                obj.tout = null;
                                                emitData(buf.slice(0, i));
                                                i=0;
                                            }, newline);
                                        }
                                    }
                                    // count bytes into a buffer...
                                    else if (spliton === "count") {
                                        if ( i >= parseInt(newline)) {
                                            emitData(buf.slice(0,i));
                                            i=0;
                                        }
                                    }
                                    // look to match char...
                                    else if (spliton === "char") {
                                        if ((c === splitc[0]) || (i === bufMaxSize)) {
                                            emitData(buf.slice(0,i));
                                            i=0;
                                        }
                                    }
                                }
                            });
                            // obj.board.on("disconnect",function() {
                            //     RED.log.error(RED._("board.errors.disconnected",{port:port}));
                            // });
                        }
                        setupboard();
                        return obj;
                    }());
                    return devices[id];
                },
                close: function(port,done) {
                    if (devices[port]) {
                        if (devices[port].tout != null) {
                            clearTimeout(devices[port].tout);
                        }
                        devices[port]._closing = true;
                        try {
                            devices[port].close(function() {
                                RED.log.info(RED._("board.errors.closed",{port:port}));
                                done();
                            });
                        }
                        catch(err) { }
                        delete devices[port];
                    }
                    else {
                        done();
                    }
                }
            }
        });
    */

}