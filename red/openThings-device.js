/*
** Node-red config node for OpenThings devices
** Author: Achronite, March 2019
**
** v0.1 Alpha
**
** File: OpenThings-device.js
**
*/
var ref = require('ref');
var libradio = require( './libradio');

module.exports = function(RED) {
    function openThingsDeviceNode(config) {
        RED.nodes.createNode(this,config);
        this.productId = config.productId;
        this.deviceId = config.deviceId;
    }
    RED.nodes.registerType("openThings-device",openThingsDeviceNode);
}