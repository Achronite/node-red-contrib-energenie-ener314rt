var ref = require('ref');
var libradio = require( './libradio');

module.exports = function(RED) {
    function openThingsDeviceNode(config) {
        RED.nodes.createNode(this,config);
        this.productId = config.productId;
        this.deviceId = config.deviceId;
    }
    RED.nodes.registerType("openThings-device",openThingsDeviceNode);

    RED.httpAdmin.get("/openThings/devices", function(req,res) {

        // allocate the return buffer here for the JSON response, C routine does not do malloc()
        var buf = Buffer.alloc(500);

        var iTimeOut = 10; // 10 seconds

        // Call discovery function (GUI is async anyway - so dont bother here)
        var ret = libradio.openThings_discover(iTimeOut, buf );
        var devices = JSON.parse(ref.readCString(buf, 0));        
        res.end(JSON.stringify(devices));

        /* async version
        libradio.openThings_discover.async(iTimeOut, buf, function(err,res) {
            // callback
            if (err) node.error("openThings_discover err: " + err);
            //this.warn("openThings_switch returned res:" + res);
            //.catch(error)
            var devices = JSON.parse(ref.readCString(buf, 0));        
            res.end(JSON.stringify(devices));
        });
        */

    });
}