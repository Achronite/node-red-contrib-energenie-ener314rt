module.exports = function(RED) {
    function openThingsDeviceNode(config) {
        RED.nodes.createNode(this,config);
        this.productId = config.productId;
        this.deviceId = config.deviceId;
    }
    RED.nodes.registerType("openThings-device",openThingsDeviceNode);
}