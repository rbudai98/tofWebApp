(async function () {
        const refreshDeviceBtn = document.getElementById("refreshDevice");
        const setFrameTypeBtn = document.getElementById("setFrameType");

        // Create WebSocket connection
        var socket = new WebSocket("ws://10.42.0.1:5000", "network-protocol");
        // Listen for possible errors
        socket.addEventListener("error", (event) => {
                console.log("WebSocket error: ", event);
        });

        socket.onopen = async function(event) {
                console.log("Connected to server");
        }
        socket.onclose = async function(event){
                console.log("Disconnected from server");
        }

        socket.onmessage = async function (event) {

                console.log("Receiving message from server...");
        }

        window.addEventListener("load", (event) => {
                protobuf.load("resources/buffer.proto");
        });

        refreshDeviceBtn.addEventListener("click", () => {
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
