// Defines and variables

const ipAddr = "ws://10.42.0.1:5000";
const networkProtocol = "network-protocol";
let connected = false;
var socket = new WebSocket(ipAddr, networkProtocol);
// Console log 

(function () {
        var old = console.log;
        var logger = document.getElementById('tofConsole');
        console.log = function (message) {
                if (typeof message == 'object') {
                        logger.innerHTML += Date.now() + ': ' + (JSON && JSON.stringify ? JSON.stringify(message) : message) + '<br />';
                } else {
                        logger.innerHTML += Date.now() + ': ' + message + '<br />';
                }
        }
})();

// Main Async function

(async function () {
        const refreshDeviceBtn = document.getElementById("refreshDevice");
        const setFrameTypeBtn = document.getElementById("setFrameType");

        window.addEventListener("load", (event) => {
                // Load protobuf structures
                protobuf.load("resources/buffer.proto");
                // Create WebSocket connection
                console.log("Connecting to server...");
                socket = new WebSocket(ipAddr, networkProtocol);
        });

        // Listen for possible errors
        socket.addEventListener("error", (event) => {
                if (connected == true) {
                        console.log("WebSocket error: ", event);
                }
        });

        socket.onopen = async function (event) {
                console.log("Connected to server");
                connected = true;
        }
        socket.onclose = async function (event) {
                if (connected == true) {
                        console.log("Disconnected from server");
                }
                connected = false;
        }

        socket.onmessage = async function (event) {

                console.log("Receiving message from server...");
        }


        refreshDeviceBtn.addEventListener("click", () => {
                if (connected == false) {
                        console.log("Connecting to server...");
                        socket = new WebSocket(ipAddr, networkProtocol);
                        connected = true;
                }
                else {
                        console.log("Already connected to server");
                }
        });


        setFrameTypeBtn.addEventListener("click", () => {
                // socket.send("setFrameType");
                console.log("setFrameType");
        });

        const urlCreator = window.URL || window.webkitURL;
        // socket.onmessage = async function (event) {
        //         const blob = await event.data.blob();
        //         var imageUrl = urlCreator.createObjectURL(blob);
        //         const imageViewer = document.getElementById("my-image");
        //         imageViewer.src = imageUrl;
        // };
})();
