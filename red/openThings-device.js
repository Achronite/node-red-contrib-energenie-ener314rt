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

        // Call discovery function
        var ret = libradio.openThings_discover(iTimeOut, buf );

        var devices = JSON.parse(ref.readCString(buf, 0));
        
        res.end(JSON.stringify(devices));
    });
}